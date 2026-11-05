# M3-05B4-01：原生画质、帧率上限与垂直同步

2026-11-05，UE **5.5.4**。主体22:00、重启读取窄补验22:10完成，**本章验收通过，无待补**。[报告](Reports/M3_05B4_01_Verification.md)记录五源码 / 双Build / 一Success和Graphics03新进程读取0 / 30 / 0。下一章为[B4-02共享画质设置卡](M3_05B4_02_GraphicsSettingsUI.md)，以下保留已完成步骤供复查。

## 1. 本章做出的行为

引擎持有的同一个GameUserSettings新增非分辨率设置接口：统一十组画质、帧率上限、VSync；用户明确应用才改运行设置并请求保存。原显示预览 / 确认 / 回退、输入偏好和试炼会话继续各自负责原来的状态。

本批先完成可复用原生接口与一个开发观察入口，下批接正式共用卡。**本章没有新WBP、按钮、Blueprint变量或接线**，不要在P菜单临时添加逻辑；开发命令不代表最终玩家操作。

| 项目 | 契约 |
| --- | --- |
| 画质0 / 1 / 2 / 3 / 4 | 低 / 中 / 高 / 极高 / 影视级；这是可选择档位，不是当前设备性能推荐 |
| -1自定义 | 保留引擎当前十组混合状态；修改帧率 / VSync不强制改画质 |
| 帧率上限 | 有限非负数；0不设本设置上限，仍可能受VSync、驱动、平台限制。正式UI后续常用项0 / 30 / 60 / 120，读取时保留合法自定义值 |
| 浏览 / 纯规则 | 不更改运行、窗口或磁盘；非法值拒绝而不偷偷夹限 |
| 应用 | 仅本机独立Game World；PIE与视频预览保护期间拒绝，无排队补执行 |
| 读取 | 引擎当前设置快照，不等同GPU实际帧率、画面质量或磁盘成功 |

实际217 Source / 123cpp（Core85 / UI35 / Editor3）、38测试、301 Content / 来源82。三新 / 两旧Core已在本地d20de4e，资产 / 共享Config未改；代码50 / 阅读10 / 检查10加窄补验1，共71分钟，等待5。M3累计1643 / 等待111；项目5054 / 明确等待435分钟。

## 2. 文件与开始前准备

保存并关闭全部UE进程。工程根为`E:\UE_Project\Demo`；在CLion创建普通C++文件即可，本章不通过UE创建UCLASS，不手建generated.h。

```text
EmberTrial/Source/EmberTrial/Public/Settings/ETGraphicsSettingsTypes.h        新
EmberTrial/Source/EmberTrial/Private/Settings/ETGraphicsSettingsTypes.cpp     新
EmberTrial/Source/EmberTrial/Tests/ETGraphicsPreferencesTests.cpp            新
EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h             旧
EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp           旧
```

路径注意是Core的`EmberTrial`，不是UI；`Settings`两侧拼写一致。Build.cs已有所需依赖，不增加UI反向依赖或Slate模块。

## 3. 三份新文件完整代码

### 3.1 Public/Settings/ETGraphicsSettingsTypes.h

```cpp
#pragma once

#include "CoreMinimal.h"

/** 非分辨率设置的值快照；Custom表示保留当前混合画质，默认对象不可提交。 */
struct EMBERTRIAL_API FETGraphicsPreferences
{
    static constexpr int32 InvalidQuality = -2;
    static constexpr int32 CustomQuality = -1;
    static constexpr int32 MaxQuality = 4; // UE5.5：低 / 中 / 高 / 极高 / 影视级。

    int32 QualityLevel = InvalidQuality;
    float FrameRateLimit = 0.0f; // 0表示取消本设置的上限，仍可能受VSync / 平台限制。
    bool bUseVSync = false;

    bool IsValid() const;
};

/** 只检查值，不访问引擎实例 / 窗口 / 磁盘；不把分辨率比例混入整体画质标签。 */
struct EMBERTRIAL_API FETGraphicsSettingsRules
{
    static int32 FindUniformQuality(TConstArrayView<int32> Groups);
    static bool TryResolve(const FETGraphicsPreferences& Request,
        const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out);
};
```

### 3.2 Private/Settings/ETGraphicsSettingsTypes.cpp

```cpp
#include "Settings/ETGraphicsSettingsTypes.h"

bool FETGraphicsPreferences::IsValid() const
{
    return QualityLevel >= CustomQuality && QualityLevel <= MaxQuality &&
        FMath::IsFinite(FrameRateLimit) && FrameRateLimit >= 0.0f;
}

int32 FETGraphicsSettingsRules::FindUniformQuality(const TConstArrayView<int32> Groups)
{
    if (Groups.IsEmpty())
    {
        return FETGraphicsPreferences::InvalidQuality;
    }

    const int32 First = Groups[0];
    bool bUniform = true;
    for (const int32 Value : Groups)
    {
        if (Value < 0 || Value > FETGraphicsPreferences::MaxQuality)
        {
            return FETGraphicsPreferences::InvalidQuality;
        }
        bUniform &= Value == First;
    }
    return bUniform ? First : FETGraphicsPreferences::CustomQuality;
}

bool FETGraphicsSettingsRules::TryResolve(const FETGraphicsPreferences& Request,
    const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out)
{
    // 输入可与输出别名；失败输出保持Invalid，不夹限后伪装为接受。
    const FETGraphicsPreferences RequestCopy = Request;
    const FETGraphicsPreferences CurrentCopy = Current;
    Out = FETGraphicsPreferences{};
    if (!RequestCopy.IsValid() || !CurrentCopy.IsValid())
    {
        return false;
    }

    Out = RequestCopy;
    if (Out.QualityLevel == FETGraphicsPreferences::CustomQuality)
    {
        Out.QualityLevel = CurrentCopy.QualityLevel;
    }
    return true;
}
```

### 3.3 Tests/ETGraphicsPreferencesTests.cpp

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETGraphicsSettingsTypes.h"
#include "Misc/AutomationTest.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGraphicsPreferencesTest,
    "EmberTrial.M3.Settings.GraphicsPreferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGraphicsPreferencesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const int32 Uniform[] = {2, 2, 2};
    const int32 Mixed[] = {2, 1, 2};
    const int32 Broken[] = {2, 1, 5};
    const int32 Cinematic[] = {4, 4};
    TestEqual(TEXT("Uniform native groups retain their tier"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Uniform)), 2);
    TestEqual(TEXT("Mixed groups are custom"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Mixed)), -1);
    TestEqual(TEXT("Invalid group is rejected even after mixed values"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Broken)), -2);
    TestEqual(TEXT("Empty groups are unavailable"),
        FETGraphicsSettingsRules::FindUniformQuality(TConstArrayView<int32>()), -2);
    TestEqual(TEXT("Stored cinematic tier is readable"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Cinematic)), 4);

    FETGraphicsPreferences Current;
    Current.QualityLevel = 2;
    Current.FrameRateLimit = 60.0f;
    FETGraphicsPreferences Request = Current;
    Request.QualityLevel = 0;
    Request.FrameRateLimit = 30.0f;
    Request.bUseVSync = true;
    FETGraphicsPreferences Out;
    TestTrue(TEXT("Legal request resolves"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestEqual(TEXT("Requested quality retained"), Out.QualityLevel, 0);
    TestEqual(TEXT("Requested cap retained"), Out.FrameRateLimit, 30.0f);
    TestTrue(TEXT("Requested VSync retained"), Out.bUseVSync);
    TestEqual(TEXT("Current snapshot unchanged"), Current.QualityLevel, 2);

    Request.QualityLevel = -1;
    Request.FrameRateLimit = 0.0f;
    TestTrue(TEXT("Custom request keeps existing tier and allows unlimited"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestEqual(TEXT("Current tier preserved"), Out.QualityLevel, 2);
    TestEqual(TEXT("Zero means no settings cap"), Out.FrameRateLimit, 0.0f);
    Current.QualityLevel = -1;
    TestTrue(TEXT("Mixed native state remains mixed"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestEqual(TEXT("Mixed groups will not be overwritten"), Out.QualityLevel, -1);
    TestTrue(TEXT("Output may alias request"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Request));

    Request.FrameRateLimit = -1.0f;
    TestFalse(TEXT("Negative cap rejected"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestFalse(TEXT("Rejected output invalidated"), Out.IsValid());
    Request.FrameRateLimit = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN cap rejected"), Request.IsValid());
    Request.FrameRateLimit = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Infinite cap rejected"), Request.IsValid());
    Request.FrameRateLimit = 59.94f;
    TestTrue(TEXT("Finite custom cap can be preserved"), Request.IsValid());
    Request.QualityLevel = 5;
    TestFalse(TEXT("Unknown quality rejected"), Request.IsValid());
    Request.QualityLevel = 1;
    TestFalse(TEXT("Unavailable current state rejected"),
        FETGraphicsSettingsRules::TryResolve(Request, FETGraphicsPreferences{}, Out));
    return true;
}
#endif
```

## 4. 两份既有GameUserSettings的增量

**以下均为增加，不替换原声明 / 函数。**特别保留输入偏好、LoadSettings / SetToDefaults、视频保护结构与所有Confirm / Save重写，不把整个private区替换掉。

### 4.1 头文件

1. 在generated.h之后、现有`/** 玩家输入偏好`注释之前增加两个前置声明。generated.h仍为最后一条include；本章普通类型头没有生成头。

```cpp
struct FETGraphicsPreferences;
class UWorld;
```

2. 在**UETGameUserSettings类的public区**，现有`ApplyInputPreferences`之后、`virtual void LoadSettings`之前插入以下声明。不要放进上方FETInputPreferences结构体。

```cpp
    /** 读取引擎保存的画质组 / 帧率 / VSync；不是GPU实际帧率或视觉达标证明。 */
    bool ReadGraphicsPreferences(FETGraphicsPreferences& Out) const;

    /** 仅独立游戏提交，视频预览保护期间拒绝；成功表示已应用并请求保存。 */
    bool ApplyGraphicsPreferences(const FETGraphicsPreferences& Request,
        const UWorld* ContextWorld);
```

### 4.2 实现文件

1. 顶部原ETGameUserSettings.h之后增加：

```cpp
#include "Settings/ETGraphicsSettingsTypes.h"
```

2. 找到现有`#if !UE_BUILD_SHIPPING`，在它**之前**粘贴以下两个完整函数。正式接口也要在Shipping存在，所以不能放进这个宏内部。

```cpp
// ---- 非分辨率设置；不切窗口、不改输入偏好或预览事务 ----

bool UETGameUserSettings::ReadGraphicsPreferences(FETGraphicsPreferences& Out) const
{
    Out = FETGraphicsPreferences{};
    if (this != GetETGameUserSettings())
    {
        return false;
    }

    // GetOverallScalabilityLevel还比较ResolutionQuality；此处明确只汇总十组画质。
    const int32 Groups[] = {
        GetViewDistanceQuality(), GetAntiAliasingQuality(), GetShadowQuality(),
        GetGlobalIlluminationQuality(), GetReflectionQuality(), GetPostProcessingQuality(),
        GetTextureQuality(), GetVisualEffectQuality(), GetFoliageQuality(), GetShadingQuality()
    };
    FETGraphicsPreferences Value;
    Value.QualityLevel = FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Groups));
    Value.FrameRateLimit = GetFrameRateLimit();
    Value.bUseVSync = IsVSyncEnabled();
    if (!Value.IsValid())
    {
        return false;
    }
    Out = Value;
    return true;
}

bool UETGameUserSettings::ApplyGraphicsPreferences(
    const FETGraphicsPreferences& Request, const UWorld* ContextWorld)
{
    // PIE不能全局更改编辑器画质；预览中保存也不能泄漏候选视频配置。
    if (this != GetETGameUserSettings() || GIsEditor || !IsValid(ContextWorld) ||
        ContextWorld->bIsTearingDown || ContextWorld->WorldType != EWorldType::Game ||
        ContextWorld->GetNetMode() != NM_Standalone || bVideoPreviewGuard)
    {
        return false;
    }

    FETGraphicsPreferences Current;
    FETGraphicsPreferences Next;
    if (!ReadGraphicsPreferences(Current) ||
        !FETGraphicsSettingsRules::TryResolve(Request, Current, Next))
    {
        return false;
    }

    if (Next.QualityLevel != FETGraphicsPreferences::CustomQuality)
    {
        // 不调用SetOverallScalabilityLevel，保留当前ResolutionQuality与窗口配置。
        SetViewDistanceQuality(Next.QualityLevel);
        SetAntiAliasingQuality(Next.QualityLevel);
        SetShadowQuality(Next.QualityLevel);
        SetGlobalIlluminationQuality(Next.QualityLevel);
        SetReflectionQuality(Next.QualityLevel);
        SetPostProcessingQuality(Next.QualityLevel);
        SetTextureQuality(Next.QualityLevel);
        SetVisualEffectQuality(Next.QualityLevel);
        SetFoliageQuality(Next.QualityLevel);
        SetShadingQuality(Next.QualityLevel);
    }
    SetFrameRateLimit(Next.FrameRateLimit);
    SetVSyncEnabled(Next.bUseVSync);
    ApplyNonResolutionSettings();
    SaveSettings();
    UE_LOG(LogETUserSettings, Display,
        TEXT("[M3-05B4-01] Graphics applied Quality=%d Cap=%.2f VSync=%d SaveRequested=1"),
        Next.QualityLevel, Next.FrameRateLimit, Next.bUseVSync ? 1 : 0);
    return true;
}

```

3. 找到文件底部现有`} // namespace ETInputPreferencesConsole`和最后的`#endif`，在这两者**之间**插入下列独立命名空间；保留原输入命令。不多加或删除#if / #endif。

```cpp
// ---- 开发观察入口；后续正式卡片复用上面的原生接口 ----
namespace ETGraphicsPreferencesConsole
{
void Execute(const TArray<FString>& Args, UWorld* World)
{
    UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    if (!Settings || !World || !World->IsGameWorld())
    {
        UE_LOG(LogETUserSettings, Display, TEXT("[M3-05B4-01] Start a game world first"));
        return;
    }

    if (!Args.IsEmpty())
    {
        FETGraphicsPreferences Request;
        if (Args.Num() != 4 || !Args[0].Equals(TEXT("apply"), ESearchCase::IgnoreCase) ||
            !FDefaultValueHelper::ParseInt(Args[1], Request.QualityLevel) ||
            !FDefaultValueHelper::ParseFloat(Args[2], Request.FrameRateLimit) ||
            (Args[3] != TEXT("0") && Args[3] != TEXT("1")))
        {
            UE_LOG(LogETUserSettings, Display,
                TEXT("[M3-05B4-01] Usage: et.GraphicsSettings [apply <-1..4 quality> <cap;0=unlimited> <vsync:0|1>]"));
            return;
        }
        Request.bUseVSync = Args[3] == TEXT("1");
        const bool bAccepted = Settings->ApplyGraphicsPreferences(Request, World);
        UE_LOG(LogETUserSettings, Display,
            TEXT("[M3-05B4-01] ApplyAccepted=%d"), bAccepted ? 1 : 0);
    }

    FETGraphicsPreferences Value;
    const bool bValid = Settings->ReadGraphicsPreferences(Value);
    const IConsoleVariable* Cap = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
    const IConsoleVariable* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
    UE_LOG(LogETUserSettings, Display,
        TEXT("[M3-05B4-01] Graphics Valid=%d Quality=%d Cap=%.2f VSync=%d EffectiveCap=%.2f EffectiveVSync=%d Ini=%s"),
        bValid ? 1 : 0, Value.QualityLevel, Value.FrameRateLimit, Value.bUseVSync ? 1 : 0,
        Cap ? Cap->GetFloat() : -1.0f, VSync ? VSync->GetInt() : -1, *GGameUserSettingsIni);
}

FAutoConsoleCommandWithWorldAndArgs Command(
    TEXT("et.GraphicsSettings"),
    TEXT("Graphics preferences: query; apply <-1..4 quality> <cap> <vsync:0|1>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Execute), ECVF_Cheat);
} // namespace ETGraphicsPreferencesConsole
```

## 5. 为什么这样接入

- 不调用SetOverallScalabilityLevel / ApplySettings：前者同时改变分辨率比例，后者会请求窗口变化。这里仅逐组设画质并调用ApplyNonResolutionSettings，保留原ResolutionQuality与窗口配置。
- UE5.5的GetOverallScalabilityLevel还比较ResolutionQuality；本项目的画质标签只汇总十个组，避免保留分辨率比例后错误显示“自定义”。混合组确实显示Custom；不会把非法组误判为混合。
- bVideoPreviewGuard检查位于任何写入之前。显示候选未确认时，本接口直接拒绝，不能在调整画质时提交候选分辨率；窗口实际事务仍由旧服务控制。
- 明确确认后调用SaveSettings仅表示请求保存。读取有效CVar辅助辨认命令行 / 更高优先级覆盖，实际帧率受GPU耗时和VSync等共同影响，不把EffectiveCap当测得FPS。
- 无新Subsystem、常驻扫描、Tick、Timer或委托；纯规则测试不创建World / 窗口、不Apply / Save，不替代真实应用与重启读取。

## 6. 正常构建与索引

UE保持关闭，完整复制下列命令（保留反斜杠）：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor成功后：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload，预计Core85 / UI35 / Editor3。确认三个新文件属于Core CodeModel，普通类型头可导航；不清缓存或手改生成CMake。报错先发送第一条真实error与附近10–20行。

## 7. 一项自动化与PIE拒绝（约2分钟）

打开UE，在Output Log执行一次：

```text
Automation RunTests EmberTrial.M3.Settings.GraphicsPreferences
```

应Success；覆盖统一 / 混合 / 非法组，合法档位、保留混合状态、取消本设置帧率上限、非法值 / NaN / Inf、别名和失败输出。此测试不验证驱动或磁盘。

正式地图Play → Output Log输入`et.GraphicsSettings`查询；再输入`et.GraphicsSettings apply 0 30 0`，应ApplyAccepted=0且快照未变。P打开原输入 / 显示卡仍存在，正常继续与移动转向后Stop。不改蓝图，不跑旧全组 / 新Cook。

## 8. 独立游戏真实应用与重启（约6–10分钟）

### 8.1 专用窗口启动

保存关闭UE后运行脚本。必须是-game，不选编辑器Play下的Standalone替代；独立日志明确写入本批Runtime，避免只剩编辑器日志。首次专用ini不存在时由引擎创建，不复制到共享Config。

```powershell
Set-Location 'E:\UE_Project\Demo'
$graphicsRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05B4_01\Runtime'
New-Item -ItemType Directory -Path $graphicsRuntime -Force | Out-Null
$graphicsProfile = Join-Path $graphicsRuntime 'GraphicsUser.ini'
$graphicsLog = Join-Path $graphicsRuntime 'Graphics01.log'
$graphicsArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '/Game/EmberTrial/Maps/L_ET_MainMenu'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    ('-GameUserSettingsINI="{0}"' -f $graphicsProfile)
    ('-abslog="{0}"' -f $graphicsLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $graphicsArgs -WindowStyle Normal
```

### 8.2 查询、应用与非法拒绝

1. 首屏显示卡应自动就绪。按键盘`~`打开游戏控制台；若只见小输入条也能输入。以下命令**逐条Enter执行**，不是在PowerShell输入。长日志由助手读取，用户只核对界面和Accepted。
2. 输入`et.GraphicsSettings`记录原快照。输入`et.GraphicsSettings apply 0 30 0`，应ApplyAccepted=1，Quality0 / Cap30 / VSync0，EffectiveCap约30。关闭控制台开始试炼，可用`stat fps`看到本机实际上限表现；GPU瓶颈允许更低，不要求精确30.00。
3. 输入`et.GraphicsSettings apply 5 60 0`：ApplyAccepted=0，仍保持上一组。再输入`et.GraphicsSettings apply -1 -10 0`同样拒绝。不要用ApplyAccepted=1但视觉看不出差异作为错误证据，灰盒难呈现所有画质组差异。
4. P暂停后输入`et.GraphicsSettings apply 1 60 1`，应接受，Quality1 / Cap60 / VSync1；暂停允许设置。输入`et.GraphicsSettings apply -1 60 0`，应保留Quality1、只关VSync。窗口尺寸 / 模式不因这些操作改变，原输入卡仍1 / 不反转。

### 8.3 显示预览期间拒绝

在P显示卡选择实际目录中的另一窗口尺寸 → 预览。在倒计时还在时打开控制台执行`et.GraphicsSettings apply 0 30 0`，应ApplyAccepted=0；关闭控制台，点取消预览（若已超时恢复也可，只需日志能确认命令发生在保护期间）。查询应保持预览开始前最后一次成功应用值（按原顺序是Quality1 / Cap60 / VSync0），不补执行被拒绝的设置。不要在预览前为了预填命令先按Enter，那会真实应用该命令。若来不及输入，保留结果，不为此反复整套重跑；可发消息由助手接手此单项。

### 8.4 重启读取与收尾

1. 正常返回菜单并退出。脚本就是8.1的整段PowerShell，不是另存的.ps1文件。把其中日志名改为Graphics02.log再启动，沿用同一GraphicsUser.ini；执行`et.GraphicsSettings`，应读取**上次退出前最后一次成功应用的值**。若严格按前述顺序，是Quality1 / Cap60 / VSync0；若之后额外成功应用其他合法值，以该值为准。存在专用ini不等于已证明重启读取。
2. 新进程进入试炼，P一种方式继续、移动与转向短查，正常返回菜单退出。个人测试配置留本批Artifacts；无需改共享Config、恢复所有机器原值或重存资产。本章未改B3原专用ini。

### 8.5 重启读取窄补验（已完成，保留复查）

**22:10结论：**Graphics03已确认22:09:51.724全新-game只读查询Valid1 / Quality0 / Cap30 / VSync0，无前置apply；正常退出22:10:01。用户额外手动1分钟，待补项关闭，下面无需重复执行。

**为什么补：**Graphics01日志中21:56:25在预览开始前又成功应用Quality0 / Cap30 / VSync0；21:56:34预览期间的同值请求正确拒绝。最终GraphicsUser.ini十组画质都是0、FrameRateLimit=30、VSync=False，这属于合法已保存状态，不是预览中错误提交。Graphics02有正确-game / 同一ini启动和退出，但没有`et.GraphicsSettings`查询输出。只补重启读取，不改代码、不重Build或重做前面检查，也不强制把合法0 / 30 / 0改回1 / 60 / 0。

1. 确保上一轮测试游戏已退出。将下列**整段**复制到PowerShell执行；沿用现有GraphicsUser.ini，只创建新的Graphics03.log。

```powershell
Set-Location 'E:\UE_Project\Demo'
$graphicsRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05B4_01\Runtime'
$graphicsProfile = Join-Path $graphicsRuntime 'GraphicsUser.ini'
$graphicsLog = Join-Path $graphicsRuntime 'Graphics03.log'
$graphicsArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '/Game/EmberTrial/Maps/L_ET_MainMenu'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    ('-GameUserSettingsINI="{0}"' -f $graphicsProfile)
    ('-abslog="{0}"' -f $graphicsLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $graphicsArgs -WindowStyle Normal
```

2. 新游戏主菜单出现后，按`~`打开**游戏控制台**，只输入下面这条查询并按Enter。不要在PowerShell或编辑器Output Log输入，也不要先执行apply。

```text
et.GraphicsSettings
```

3. 应输出`Valid=1 Quality=0 Cap=30.00 VSync=0`，有效CVar一般为EffectiveCap30 / EffectiveVSync0。若不同，保留实际输出并反馈，不先改值覆盖现场。助手读取长日志，你不用抄整行。
4. 收起控制台，从主菜单正常退出；无需再进试炼或切分辨率。可反馈：

```text
M3-05B4-01 重启读取窄补验
同一GraphicsUser.ini / Graphics03.log / 未先执行apply：
全新-game查询Valid1 / Quality0 / Cap30 / VSync0（不符写实际）：
正常退出：
完成时分 / 额外手动用时：
```

## 9. 完成后反馈

```text
M3-05B4-01 完成
三新 / 两旧Core；关闭UE后Editor Build / Core85-UI35-Editor3索引 / Game Build：
GraphicsPreferences一次 / PIE应用拒绝：
独立窗口Quality0-Cap30 / 非法拒绝、窗口与输入隔离：
暂停Quality1-Cap60-VSync1，随后保持画质关闭VSync：
视频预览保护期间ApplyAccepted0、取消后无补执行：
独立重启读取最后已保存值（本次0-30-0）/ 日志 / 原P与移动短查：
Source已保存，资产 / 共享Config未改；失败或不理解处（无填无）：
完成时分 / 手动代码 / 阅读 / 检查 / 构建启动等待：
```

本批没有正式画质控件符合预期。后续B4-02再实现共用草稿卡、应用 / 取消与键盘导航；不直接在Blueprint调用引擎散落Setter。

## 10. Commit / Push（固定放在最后）

B4-01五份Core实现已在本地`d20de4e`。当前Stage其验收报告 / 指南收尾、新B4-02指南与实际更新的共享状态文档；02五新 / 四旧UI及一新 / 两旧Widget尚未实施，不提前Stage候选、空报告或来源行。

- 英文文档建议：`docs: verify graphics preferences and prepare shared graphics settings UI`
- 专业中文：`docs: 完成画质偏好验收并准备共享画质设置界面`

02未来源码 / 资产提交建议见新指南末尾。Artifacts证据 / 个人ini、候选与缓存留本地。Stage后继续修改需重新Stage → Review → Commit / Push由用户操作，助手未核验远端。
