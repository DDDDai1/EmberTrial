# M3-05B1：视频模式值契约与原生快照

2026-11-05，UE **5.5.4**。[A3已验收](Reports/M3_05A3_Verification.md)，无待补。本章是手动实施指南，正式Source尚未修改，无本章Build / Success结论。

## 1. 本章交付与分批

显示设置按**B1快照 / 候选规则 → B2实际应用、确认 / 超时回退 → B3界面接入与分辨率实际核验**推进；画质 / 帧率按引擎非分辨率应用路径分批，音量仍后续。本章先分清“请求的配置”“上次确认配置”和“真实Viewport”，避免后续用尚未生效的请求值当回退基线。

新增纯值类型、原生只读快照与候选正规化。窗口 / 全屏只允许调用方提供目录里的正尺寸；无边框使用提供的实际桌面尺寸，不硬写1080p。目录来自后续平台适配，本章测试里的分辨率是夹具，不代表这台机器支持列表。候选合法不保证OS切换成功，实际应用 / 确认仍必须由B2处理。

`et.VideoMode`只读观察原生设置，不切屏、不修改 / 保存个人配置。Requested / Confirmed可为0，此时记录Valid=0，不悄悄补默认或把它当正在显示的真实像素。当前个人输入设置、页面与试炼业务保持。

预计代码25–40 / 阅读5–10 / 检查3–5分钟，构建等待另计。实际201 Source / 113cpp（Core78 / UI32 / Editor3）、34测试、300 Content / 来源81；完成后预计204 Source / 115cpp（Core80 / UI32 / Editor3）、35测试，资产数量保持。预计不计实耗。

## 2. 文件与准备

Save All并关闭UE，在CLion创建以下三个**普通Core源码文件**。它们没有UCLASS / USTRUCT / generated.h，不能用UE类向导生成另一套反射类；不改Build.cs、现有GameUserSettings、UI / Config或资产。

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Settings\ETVideoModeRules.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Settings\ETVideoModeRules.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETVideoModeRulesTests.cpp
```

下面按文件完整粘贴，三份各创建一份；UI模块同名Settings目录不用于本章。

## 3. 三份完整源码

### 3.1 Public/Settings/ETVideoModeRules.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

class UGameUserSettings;

// 原生值类型，不是UENUM / USTRUCT；UI后续用枚举映射显示文字。
enum class EETVideoWindowMode : uint8
{
    Windowed,
    Borderless,
    Fullscreen,
    Invalid
};

struct EMBERTRIAL_API FETVideoMode
{
    FIntPoint Resolution = FIntPoint::ZeroValue;
    EETVideoWindowMode Mode = EETVideoWindowMode::Invalid;

    bool IsWellFormed() const;
    bool operator==(const FETVideoMode& Other) const
    {
        return Resolution == Other.Resolution && Mode == Other.Mode;
    }
};

// Requested / LastConfirmed都是引擎配置状态，不能冒充当前Viewport实际像素。
struct EMBERTRIAL_API FETVideoModeSnapshot
{
    FETVideoMode Requested;
    FETVideoMode LastConfirmed;
    FIntPoint DesktopResolution = FIntPoint::ZeroValue;
};

class EMBERTRIAL_API FETVideoModeRules
{
public:
    // 只读已有引擎实例；未初始化尺寸保留0，由IsWellFormed表达不可用。
    static bool Capture(const UGameUserSettings* Settings, FETVideoModeSnapshot& Out);

    // AllowedSizes由后续平台适配提供；本函数不查询显卡、不切屏或保存。
    static bool TryResolve(const FETVideoMode& Request, FIntPoint Desktop,
        TConstArrayView<FIntPoint> AllowedSizes, FETVideoMode& Out);
};
```

### 3.2 Private/Settings/ETVideoModeRules.cpp

```cpp
#include "Settings/ETVideoModeRules.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogETVideoModes, Log, All);

namespace ETVideoModePrivate
{
EETVideoWindowMode FromNative(const EWindowMode::Type Mode)
{
    switch (Mode)
    {
    case EWindowMode::Windowed: return EETVideoWindowMode::Windowed;
    case EWindowMode::WindowedFullscreen: return EETVideoWindowMode::Borderless;
    case EWindowMode::Fullscreen: return EETVideoWindowMode::Fullscreen;
    default: return EETVideoWindowMode::Invalid;
    }
}
} // namespace ETVideoModePrivate

bool FETVideoMode::IsWellFormed() const
{
    const bool bKnownMode = Mode == EETVideoWindowMode::Windowed ||
        Mode == EETVideoWindowMode::Borderless || Mode == EETVideoWindowMode::Fullscreen;
    return bKnownMode && Resolution.X > 0 && Resolution.Y > 0;
}

// ---- 引擎状态快照：保留原值，不修配置或悄悄填1080p ----

bool FETVideoModeRules::Capture(const UGameUserSettings* Settings, FETVideoModeSnapshot& Out)
{
    Out = FETVideoModeSnapshot{};
    if (!IsValid(Settings))
    {
        return false;
    }
    Out.Requested.Resolution = Settings->GetScreenResolution();
    Out.Requested.Mode = ETVideoModePrivate::FromNative(Settings->GetFullscreenMode());
    Out.LastConfirmed.Resolution = Settings->GetLastConfirmedScreenResolution();
    Out.LastConfirmed.Mode = ETVideoModePrivate::FromNative(Settings->GetLastConfirmedFullscreenMode());
    Out.DesktopResolution = Settings->GetDesktopResolution();
    return true;
}

// ---- 候选正规化：无边框跟随桌面，其余只接受提供目录中的合法尺寸 ----

bool FETVideoModeRules::TryResolve(const FETVideoMode& Request, const FIntPoint Desktop,
    const TConstArrayView<FIntPoint> AllowedSizes, FETVideoMode& Out)
{
    // 先复制，允许调用者把同一变量用作输入和输出。
    const FETVideoMode Copy = Request;
    Out = FETVideoMode{};
    if (Copy.Mode == EETVideoWindowMode::Borderless)
    {
        if (Desktop.X <= 0 || Desktop.Y <= 0)
        {
            return false;
        }
        Out = Copy;
        Out.Resolution = Desktop;
        return true;
    }
    if (!Copy.IsWellFormed())
    {
        return false;
    }
    for (const FIntPoint Size : AllowedSizes)
    {
        if (Size.X > 0 && Size.Y > 0 && Size == Copy.Resolution)
        {
            Out = Copy;
            return true;
        }
    }
    return false;
}

#if !UE_BUILD_SHIPPING
namespace ETVideoModeConsole
{
void Query(const TArray<FString>& Args, UWorld* World)
{
    if (!Args.IsEmpty() || !World || !World->IsGameWorld())
    {
        UE_LOG(LogETVideoModes, Display, TEXT("[M3-05B1] Start PIE; query only: et.VideoMode"));
        return;
    }
    const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    FETVideoModeSnapshot Snapshot;
    if (!FETVideoModeRules::Capture(Settings, Snapshot))
    {
        UE_LOG(LogETVideoModes, Display, TEXT("[M3-05B1] Engine settings unavailable"));
        return;
    }
    UE_LOG(LogETVideoModes, Display,
        TEXT("[M3-05B1] Video Requested=%dx%d Mode=%d Valid=%d Confirmed=%dx%d Mode=%d Valid=%d Desktop=%dx%d"),
        Snapshot.Requested.Resolution.X, Snapshot.Requested.Resolution.Y,
        static_cast<int32>(Snapshot.Requested.Mode), Snapshot.Requested.IsWellFormed() ? 1 : 0,
        Snapshot.LastConfirmed.Resolution.X, Snapshot.LastConfirmed.Resolution.Y,
        static_cast<int32>(Snapshot.LastConfirmed.Mode), Snapshot.LastConfirmed.IsWellFormed() ? 1 : 0,
        Snapshot.DesktopResolution.X, Snapshot.DesktopResolution.Y);
}

FAutoConsoleCommandWithWorldAndArgs Command(TEXT("et.VideoMode"),
    TEXT("Read requested / confirmed video configuration and desktop size; does not apply."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Query), ECVF_Cheat);
} // namespace ETVideoModeConsole
#endif
```

### 3.3 Tests/ETVideoModeRulesTests.cpp

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETVideoModeRules.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETVideoModeRulesTest,
    "EmberTrial.M3.Settings.VideoModeRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETVideoModeRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FIntPoint Desktop(2560, 1440);
    const TArray<FIntPoint> Sizes = {FIntPoint::ZeroValue, FIntPoint(1280, 720),
        FIntPoint(1920, 1080), FIntPoint(1920, 1080)};
    FETVideoMode Request;
    Request.Mode = EETVideoWindowMode::Windowed;
    Request.Resolution = FIntPoint(1280, 720);
    FETVideoMode Out;
    TestTrue(TEXT("Windowed catalogue entry accepted"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    TestTrue(TEXT("Resolution and mode preserved"), Out == Request);
    Request.Mode = EETVideoWindowMode::Fullscreen;
    TestTrue(TEXT("Fullscreen catalogue entry accepted"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    Request.Resolution = FIntPoint(1500, 900);
    TestFalse(TEXT("Unlisted resolution rejected"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    TestFalse(TEXT("Failure clears candidate"), Out.IsWellFormed());
    Request.Mode = static_cast<EETVideoWindowMode>(255);
    TestFalse(TEXT("Unknown mode rejected"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));

    Request.Mode = EETVideoWindowMode::Borderless;
    Request.Resolution = FIntPoint::ZeroValue;
    TestTrue(TEXT("Borderless uses desktop even without separate resolution choice"),
        FETVideoModeRules::TryResolve(Request, Desktop, {}, Out));
    TestTrue(TEXT("Borderless normalizes to actual supplied desktop"), Out.Resolution == Desktop);
    TestFalse(TEXT("Unknown desktop cannot be invented"),
        FETVideoModeRules::TryResolve(Request, FIntPoint::ZeroValue, Sizes, Out));
    Request.Mode = EETVideoWindowMode::Windowed;
    Request.Resolution = FIntPoint(-1, 720);
    TestFalse(TEXT("Negative dimensions rejected"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    Request.Resolution = FIntPoint(1280, 720);
    TestTrue(TEXT("Aliased input and output supported"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Request));
    TestTrue(TEXT("Aliased result is intact"), Request.Resolution == FIntPoint(1280, 720));

    // 独立对象只改配置内存；ConfirmVideoMode只登记确认值，不调用Apply / Save切屏。
    UGameUserSettings* Local = NewObject<UGameUserSettings>();
    if (!TestNotNull(TEXT("Isolated settings object"), Local))
    {
        return false;
    }
    Local->SetScreenResolution(FIntPoint(1600, 900));
    Local->SetFullscreenMode(EWindowMode::Windowed);
    Local->ConfirmVideoMode();
    Local->SetScreenResolution(FIntPoint(1280, 720));
    Local->SetFullscreenMode(EWindowMode::Fullscreen);
    FETVideoModeSnapshot Snapshot;
    TestTrue(TEXT("Native snapshot captured"), FETVideoModeRules::Capture(Local, Snapshot));
    TestTrue(TEXT("Requested differs from last confirmed"),
        Snapshot.Requested.Resolution == FIntPoint(1280, 720) &&
        Snapshot.Requested.Mode == EETVideoWindowMode::Fullscreen &&
        Snapshot.LastConfirmed.Resolution == FIntPoint(1600, 900) &&
        Snapshot.LastConfirmed.Mode == EETVideoWindowMode::Windowed);
    TestTrue(TEXT("Capture does not confirm the pending native request"),
        Local->GetLastConfirmedScreenResolution() == FIntPoint(1600, 900));
    TestFalse(TEXT("Missing settings rejected"), FETVideoModeRules::Capture(nullptr, Snapshot));
    TestTrue(TEXT("Rejected snapshot is cleared"),
        !Snapshot.Requested.IsWellFormed() && Snapshot.DesktopResolution == FIntPoint::ZeroValue);
    return true;
}
#endif
```


## 4. 正常Build与索引

UE关闭、文件保存。PowerShell先Editor Build / 三索引：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload后预计Core80 / UI32 / Editor3；新文件属于EmberTrialCodeModel，GameUserSettings.h能导航。遇错贴第一条error及前后10–20行，exit code 6只是汇总；不删缓存、也不手写generated.h。

## 5. 最小检查（3–5分钟）

1. 重开UE，停止PIE，从Output Log的Cmd运行一次：

```text
Automation RunTests EmberTrial.M3.Settings.VideoModeRules
```

2. 一次Success即可。该专项覆盖窗口 / 全屏目录资格、无边框桌面正规化、非法模式 / 尺寸、失败输出清空、输入输出别名与原生请求 / 确认分离；不切真实屏幕或写个人配置，不是显示事务实机。
3. 常用正式地图Play，游戏视口`~`控制台执行**不带参数**的查询：

```text
et.VideoMode
```

4. 看见一条`[M3-05B1] Video Requested=... Confirmed=... Desktop=...`即可，**不用修改显示设置**。Mode项目枚举0=窗口、1=无边框、2=全屏、3=未知；这不是UE内部枚举数值。Requested / Confirmed为0或Valid=0如实反馈，不当作编译 / 功能失败。日志不宣称此值等于实际PIE窗口。
5. P开关一次，确认原输入卡为1 / 不反转；主菜单或祭坛本批未改，不重跑其矩阵。移动 / 转向一次、Stop，保持原地图 / 配置，源码保存。无需新蓝图、全35项、重启持久化或新Cook。

## 6. 设计边界与后续接入

- 不复制引擎的持久化字段，本章值对象只是读快照 / 候选；真正运行设置仍由引擎持有。
- `Capture`读取GetScreenResolution / GetLastConfirmedScreenResolution及对应模式；桌面查询在无显示环境允许0。接口已按本机5.5 `Engine/Classes/GameFramework/GameUserSettings.h`与其Private实现核对。
- B2必须取得实际运行窗口 / Viewport基线再预览，确认前不Save；倒计时不能依赖已暂停的World Timer，关闭 / 切图 / 失效时需恢复。PIE与独立游戏模式边界另外核验，本章不承诺已经实现回退。
- AllowedSizes由平台适配提供并含明确允许的窗口尺寸，不能拿测试常数代替用户显示器目录；无边框使用桌面像素，不假装任意分辨率都已生效。
- 无Tick / Timer、ASC或UI反向依赖；开发查询仅明确调用时读一次原生状态。面试重点是配置请求 / 已确认 / 当前显示三者的区别。

## 7. 完成后反馈

```text
M3-05B1 完成
三新Core；Editor Build / Core80-UI32-Editor3索引 / Game Build：
VideoModeRules一次：
et.VideoMode已输出（0 / Valid0如实保留，长日志由助手读取）：
原P输入卡默认1 / 不反转与移动转向短查：
源码保存，资产 / 配置未改；失败或不理解处（无填无）：
完成时分 / 手动代码 / 阅读 / 检查 / 构建等待：
```

## 8. Commit / Push（固定放在最后）

**当前文档收尾**：A3实现在本地`63ff171`，当前Stage A3验收 / 指南收尾、两资产来源更新、状态与本B1指南；B1正式源码尚未实施，候选 / 日志不Stage。英文`docs: verify main menu settings and prepare video mode rules`；中文`docs: 完成主菜单设置验收并准备视频模式规则`。

**本章实施后**，CLion检查第2节三份新Core源码与实际修改的本指南 / Reports / Progress等文档；本章不改既有Source、Config、Build.cs或Content，不强制重存资产。未生成报告不创建空文件；个人ini / Saved / Intermediate / Binaries / .idea / CMake生成 / Artifacts留本地。

英文实现建议：`feat: add video mode snapshots and candidate validation`。

专业中文：`feat: 新增视频模式快照与候选配置校验`。

Stage后又修改需重新Stage最新版本 → Review → Commit / Push由用户操作。
