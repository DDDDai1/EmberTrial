# M3-05A1：输入偏好、镜头灵敏度与本地保存

2026-11-04，UE **5.5.4**。[M3-04C2已验收](Reports/M3_04C2_Verification.md)，无待补。本章提供手动实施代码与步骤，正式Source / Config尚未修改，没有本章Build或Success结论。

## 1. 本章交付与后续安排

先让鼠标灵敏度与Y轴反转实际作用于自由镜头，并通过引擎GameUserSettings保存在本机；Stop、Retry、Continue或NewRun都不会重置个人偏好，完整退出编辑器后仍可读取。默认1倍 / 不反转保留当前手感，范围0.25–3倍，拒绝非法值。

这是M3-05的第一批：**A1输入偏好 / 保存 → A2正式设置面板、草稿 / 应用 / 取消 / 输入项默认与键盘路径 → B窗口 / 分辨率 / 画质及确认回退 → C音量实际路由 → 集成新包。**显示与音量会按实际依赖继续拆分，避免一次重做菜单、暂停和所有设置。完整6件遗物与500行列表压力仍在后续，不因本章开始而宣称M3整体完成。

本章用现成控制台入口观察，正式设置按钮留A2；不要求搭临时验收蓝图。`ApplyInputPreferences`只更新合法运行值，确认入口随后调用`SaveSettings`。下一批页面持有值副本，滑动 / 浏览 / 取消不会调用这两个方法；输入项默认也先改草稿。`SetToDefaults`是引擎级全设置重置，不能拿来实现页面的“仅输入默认”。

| 所有者 | 职责与边界 |
| --- | --- |
| `FETInputPreferences` | 有限数 / 范围校验、轴倍率与反转；值快照，不持有Pawn / UI |
| `UETGameUserSettings` | 引擎持有的设置实例、Config读取与合法输入提交；沿用原生本地保存 |
| Character原`Look` | 输入到Controller前变换一次；不改IA / IMC、不加Tick / Timer、不乘DeltaSeconds |
| 既有锁定相机 / Flow | 继续负责锁定构图、镜头输入接管与暂停；灵敏度不改变自动锁定跟随速度 |

个人偏好写入`GameUserSettings.ini`，**不是试炼磁盘存档**，不保存遗物、检查点或角色资源。多人分屏 / 手柄偏好不在当前范围。此阶段沿用现有鼠标平滑 / 模板输入倍率，本章倍数是当前手感的相对倍率。

预计：代码35–55分钟 / 配置与阅读5–10 / 检查5–10，另记构建与一次编辑器重启等待。实际起点193 Source / 108cpp（Core76 / UI29 / Editor3）、32自动化、299 Content / 来源80；完成后预计196 / 110cpp（Core78 / UI29 / Editor3）、33自动化，资产数量保持。以下预期不计入实际进度。

## 2. 实施前与文件位置

1. UE执行Save All，停止PIE并关闭编辑器；CLion可保留。不要使用Live Coding编译新增反射类型或切换GameUserSettings类。
2. 全部源码都属于**Core模块EmberTrial**，不是EmberTrialUI。若从CLion创建，按下表分别在Public / Private新建同名`Settings`目录；不要额外生成一个Settings模块。
3. 三份新文件粘贴下节完整内容，两个旧文件只改指定位置。无需修改Build.cs / .uproject / Target或创建Blueprint父类。

| 新旧 | 相对仓库根目录`E:\UE_Project\Demo`的完整层级 |
| --- | --- |
| 新 | `EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Tests/ETInputPreferencesTests.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp` |
| 旧配置 | `EmberTrial/Config/DefaultEngine.ini` |

## 3. 三份新源码：按文件完整粘贴

### 3.1 Public/Settings/ETGameUserSettings.h

`ETGameUserSettings.generated.h`是本文件最后一条include；由UHT生成，**不要手动创建generated.h**。如果用UE类向导创建过同名类，完整替换其模板，避免两份同名UCLASS。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "ETGameUserSettings.generated.h"

/** 玩家输入偏好的值快照；设置页面可持有副本，浏览 / 取消不改变已应用值。 */
USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETInputPreferences
{
    GENERATED_BODY()

    static constexpr float MinSensitivity = 0.25f;
    static constexpr float MaxSensitivity = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input",
        meta = (ClampMin = "0.25", ClampMax = "3.0", UIMin = "0.25", UIMax = "3.0"))
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bInvertMouseY = false;

    bool IsValid() const;
    FVector2D TransformLook(const FVector2D& RawLook) const;
};

/** 机器上的玩家偏好，跨地图 / 进程保存；不属于试炼存档或身体资源。 */
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class EMBERTRIAL_API UETGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    // 取引擎持有的实例；不创建第二个全局单例，也不把CDO当运行设置使用。
    static UETGameUserSettings* GetETGameUserSettings();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Settings")
    FETInputPreferences GetInputPreferences() const { return InputPreferences; }

    /** 只提交合法值到运行内存；调用者在整页确认后显式SaveSettings，不逐次滑动写盘。 */
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Settings")
    bool ApplyInputPreferences(const FETInputPreferences& Preferences);

    virtual void LoadSettings(bool bForceReload = false) override;
    virtual void SetToDefaults() override;

private:
    UPROPERTY(Config)
    FETInputPreferences InputPreferences;
};
```

### 3.2 Private/Settings/ETGameUserSettings.cpp

只创建这一份cpp。开发命令限定游戏World且Shipping排除；查询不保存，合法`apply` / `defaults`才显式保存。`SaveSettings`返回void，因此日志只报告Save requested；实际落盘由第7节重启读取验证。

```cpp
#include "Settings/ETGameUserSettings.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CoreGlobals.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DefaultValueHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUserSettings, Log, All);

// ---- 合法值与输入变换：保持模板轴向，只在最终Look输入上乘一次 ----

bool FETInputPreferences::IsValid() const
{
    return FMath::IsFinite(MouseSensitivity) &&
        MouseSensitivity >= MinSensitivity && MouseSensitivity <= MaxSensitivity;
}

FVector2D FETInputPreferences::TransformLook(const FVector2D& RawLook) const
{
    if (RawLook.ContainsNaN())
    {
        return FVector2D::ZeroVector;
    }

    // 运行时防线：损坏的配置不允许给Controller注入NaN / Inf。
    const FETInputPreferences Safe = IsValid() ? *this : FETInputPreferences{};
    const double PitchScale = Safe.bInvertMouseY ? -1.0 : 1.0;
    const FVector2D Result(RawLook.X * Safe.MouseSensitivity,
        RawLook.Y * Safe.MouseSensitivity * PitchScale);
    return Result.ContainsNaN() ? FVector2D::ZeroVector : Result;
}

// ---- 引擎实例 / 配置生命周期；不绑定Pawn，不增加Tick或Timer ----

UETGameUserSettings* UETGameUserSettings::GetETGameUserSettings()
{
    return GEngine ? Cast<UETGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

bool UETGameUserSettings::ApplyInputPreferences(const FETInputPreferences& Preferences)
{
    if (!Preferences.IsValid())
    {
        return false;
    }

    InputPreferences = Preferences;
    return true;
}

void UETGameUserSettings::LoadSettings(bool bForceReload)
{
    Super::LoadSettings(bForceReload);
    if (!InputPreferences.IsValid())
    {
        InputPreferences = FETInputPreferences{};
        UE_LOG(LogETUserSettings, Warning,
            TEXT("[M3-05A1] Invalid stored input preferences; restored input defaults"));
    }
}

void UETGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    InputPreferences = FETInputPreferences{};
}

#if !UE_BUILD_SHIPPING
// ---- 开发观察入口；正式设置页将复用同一接口，不把控制台作为产品UI ----

namespace ETInputPreferencesConsole
{
void Execute(const TArray<FString>& Args, UWorld* World)
{
    if (!World || !World->IsGameWorld())
    {
        UE_LOG(LogETUserSettings, Display, TEXT("[M3-05A1] Start PIE or a game world first"));
        return;
    }

    UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    if (!Settings)
    {
        UE_LOG(LogETUserSettings, Warning,
            TEXT("[M3-05A1] Settings class missing; check DefaultEngine.ini and restart editor"));
        return;
    }

    if (!Args.IsEmpty())
    {
        FETInputPreferences Next;
        if (Args.Num() == 1 && Args[0].Equals(TEXT("defaults"), ESearchCase::IgnoreCase))
        {
            // 仅重置输入偏好；不调用全局SetToDefaults去重置画质 / 窗口。
        }
        else if (Args.Num() == 3 &&
            Args[0].Equals(TEXT("apply"), ESearchCase::IgnoreCase) &&
            FDefaultValueHelper::ParseFloat(Args[1], Next.MouseSensitivity) &&
            (Args[2] == TEXT("0") || Args[2] == TEXT("1")))
        {
            Next.bInvertMouseY = Args[2] == TEXT("1");
        }
        else
        {
            UE_LOG(LogETUserSettings, Display,
                TEXT("[M3-05A1] Usage: et.InputSettings [apply <0.25..3> <InvertY:0|1> | defaults]"));
            return;
        }

        if (!Settings->ApplyInputPreferences(Next))
        {
            UE_LOG(LogETUserSettings, Display,
                TEXT("[M3-05A1] Rejected input preferences; current values unchanged"));
            return;
        }

        // 只在明确确认时保存。SaveSettings返回void，不能把调用写成已验证磁盘成功。
        Settings->SaveSettings();
        UE_LOG(LogETUserSettings, Display, TEXT("[M3-05A1] Save requested"));
    }

    const FETInputPreferences Current = Settings->GetInputPreferences();
    UE_LOG(LogETUserSettings, Display,
        TEXT("[M3-05A1] Input Class=%s Sensitivity=%.2f InvertY=%d Ini=%s"),
        *Settings->GetClass()->GetPathName(), Current.MouseSensitivity,
        Current.bInvertMouseY ? 1 : 0, *GGameUserSettingsIni);
}

FAutoConsoleCommandWithWorldAndArgs Command(
    TEXT("et.InputSettings"),
    TEXT("Input preferences: query; apply <0.25..3> <InvertY:0|1>; defaults"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Execute), ECVF_Cheat);
} // namespace ETInputPreferencesConsole
#endif
```

### 3.3 Tests/ETInputPreferencesTests.cpp

测试使用独立设置对象，只更改内存；不会保存或重载你的个人配置，不改变全局分辨率 / 画质。首项会检查引擎是否选中了新类，因此必须完成配置并**重启编辑器**再运行。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/ETGameUserSettings.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETInputPreferencesTest,
    "EmberTrial.M3.Settings.InputPreferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETInputPreferencesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    TestNotNull(TEXT("Engine selects the configured project settings class"),
        UETGameUserSettings::GetETGameUserSettings());

    // 独立对象只改内存，不调用SaveSettings / LoadSettings或全局ApplySettings。
    UETGameUserSettings* Settings = NewObject<UETGameUserSettings>();
    if (!TestNotNull(TEXT("Isolated settings object"), Settings))
    {
        return false;
    }
    Settings->SetToDefaults();
    const FETInputPreferences Defaults = Settings->GetInputPreferences();
    TestTrue(TEXT("Input defaults are legal"), Defaults.IsValid());
    TestEqual(TEXT("Default scale preserves template feel"), Defaults.MouseSensitivity, 1.0f);
    TestFalse(TEXT("Default pitch is not inverted"), Defaults.bInvertMouseY);

    // 草稿是副本；构造 / 修改副本不会偷偷提交，更不需要模拟Widget来验证。
    FETInputPreferences Draft = Defaults;
    Draft.MouseSensitivity = 0.5f;
    Draft.bInvertMouseY = true;
    TestEqual(TEXT("Editing a copy does not apply it"),
        Settings->GetInputPreferences().MouseSensitivity, 1.0f);
    TestTrue(TEXT("Valid input commit accepted"), Settings->ApplyInputPreferences(Draft));
    TestTrue(TEXT("Applied multiplier and pitch inversion both take effect"),
        Settings->GetInputPreferences().TransformLook(FVector2D(4.0, 2.0)).Equals(
            FVector2D(2.0, -1.0)));
    TestTrue(TEXT("Repeated confirmation is accepted"), Settings->ApplyInputPreferences(Draft));

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    for (const float Bad : {0.0f, -1.0f, 0.24f, 3.01f, NaN, Infinity})
    {
        FETInputPreferences Invalid = Draft;
        Invalid.MouseSensitivity = Bad;
        TestFalse(TEXT("Illegal multiplier is rejected"), Settings->ApplyInputPreferences(Invalid));
        TestEqual(TEXT("Rejection preserves applied value"),
            Settings->GetInputPreferences().MouseSensitivity, 0.5f);
        TestTrue(TEXT("Rejection preserves applied inversion"),
            Settings->GetInputPreferences().bInvertMouseY);
    }
    for (const float Limit : {FETInputPreferences::MinSensitivity,
        FETInputPreferences::MaxSensitivity})
    {
        FETInputPreferences Boundary;
        Boundary.MouseSensitivity = Limit;
        TestTrue(TEXT("Inclusive boundary accepted"), Settings->ApplyInputPreferences(Boundary));
    }

    TestTrue(TEXT("Default transform preserves both signs"),
        Defaults.TransformLook(FVector2D(-3.0, 2.0)).Equals(FVector2D(-3.0, 2.0)));
    TestTrue(TEXT("Zero input stays zero"), Draft.TransformLook(FVector2D::ZeroVector).IsZero());
    TestTrue(TEXT("Nonfinite source input is suppressed"),
        Draft.TransformLook(FVector2D(NaN, 1.0)).IsZero());
    TestTrue(TEXT("Infinite source input is suppressed"),
        Draft.TransformLook(FVector2D(1.0, Infinity)).IsZero());
    FETInputPreferences Corrupt;
    Corrupt.MouseSensitivity = NaN;
    Corrupt.bInvertMouseY = true;
    TestTrue(TEXT("Corrupt preferences fall back to complete input defaults"),
        Corrupt.TransformLook(FVector2D(2.0, -3.0)).Equals(FVector2D(2.0, -3.0)));
    TestTrue(TEXT("Finite input that overflows after scaling is suppressed"),
        Settings->GetInputPreferences().TransformLook(
            FVector2D(std::numeric_limits<double>::max(), 1.0)).IsZero());

    TestTrue(TEXT("Input-only defaults can be committed"),
        Settings->ApplyInputPreferences(FETInputPreferences{}));
    TestTrue(TEXT("Input-only reset restores template look"),
        Settings->GetInputPreferences().TransformLook(FVector2D(4.0, -2.0)).Equals(
            FVector2D(4.0, -2.0)));
    return true;
}
#endif
```

## 4. 两个旧文件：精确修改

### 4.1 Character.cpp加入一个include

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp`。在现有include组最后一条`#include "Engine/GameInstance.h"`之后增加：

```cpp
#include "Settings/ETGameUserSettings.h"
```

只加一次，不在函数中include。

### 4.2 完整替换Look函数

在同文件定位`void AEmberTrialCharacter::Look(const FInputActionValue& Value)`，替换从该签名开始到其匹配右花括号的**整个函数**，不要替换后面的治疗逻辑或保留两份Look：

```cpp
void AEmberTrialCharacter::Look(const FInputActionValue& Value)
{
    if (!Controller)
    {
        return;
    }

    // 只变换收到的轴，不乘DeltaSeconds；锁定 / 暂停仍由既有Controller输入门控接管。
    const UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    const FETInputPreferences Preferences = Settings
        ? Settings->GetInputPreferences() : FETInputPreferences{};
    const FVector2D LookAxis = Preferences.TransformLook(Value.Get<FVector2D>());
    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}
```

原`SetupPlayerInputComponent`的Look绑定、Controller、IA / IMC都保留。轴只乘一次灵敏度；锁定中的鼠标拒绝和P恢复继续走既有链。没有设置实例时保留模板默认输入，避免因配置尚未生效而失去镜头；测试 / 查询会明确指出类配置问题。

### 4.3 DefaultEngine.ini指定引擎设置类

打开`E:\UE_Project\Demo\EmberTrial\Config\DefaultEngine.ini`，搜索现有节标题`[/Script/Engine.Engine]`。**在已有节中增加这一行**，保留原有ActiveGameNameRedirects / ActiveClassRedirects等条目；不要重复新建同名节，不改GameInstance或GameMode：

```ini
[/Script/Engine.Engine]
GameUserSettingsClassName=/Script/EmberTrial.ETGameUserSettings
```

上面两行用于说明节与键的对应关系：节标题原本已有，实际只新增键。脚本路径没有C++类前缀U。全项目该键应只有一处。无需手动创建`DefaultGameUserSettings.ini`或把Saved中的个人文件复制到共享Config。

## 5. 正常Build与索引

确认UE仍关闭，所有文件保存。PowerShell依次运行，第一条脚本会做Editor正常构建并刷新三个模块索引：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后再执行Game Development构建：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion如弹出CMake Reload则选择Reload；确认三份新文件属于EmberTrialCodeModel、Ctrl+B能进入GameUserSettings.h。预计Core78 / UI29 / Editor3 cpp。**CLion CodeModel本身不替代UBT正常构建**。报错请贴第一条真实error及其前后10–20行，exit code 6仅是汇总。

重新启动`EmberTrial.uproject`，打开正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。不需要保存 / 修改任何角色或Widget资产。

## 6. 自动化：仅两个受影响专项

未开始PIE时，从编辑器底部`Cmd` / Output Log命令框分别执行；看到前一条完成后再输入后一条，不连写：

```text
Automation RunTests EmberTrial.M3.Settings.InputPreferences
```

```text
Automation RunTests EmberTrial.M2.TargetLock.CameraRules
```

各一次Success即可。新专项覆盖配置类选择、默认手感、值副本与真正应用、拒绝不改旧值、边界 / NaN / Inf及轴变换；原CameraRules回归既有锁定构图。自动化不冒充实际鼠标或本机磁盘读写，实机只查下一节。不要跑全33项或新增临时蓝图图表。

## 7. 实机分组短查（约5–10分钟，重启等待另记）

### 7.1 查询与实际轴变换

1. 正式地图Play。按`~`打开**游戏视口内**控制台，输入`et.InputSettings`并回车；也可在编辑器Output Log的Cmd输入，若提示Start PIE则改用视口控制台。无须自己指定World。
2. 应看到`Class=/Script/EmberTrial.ETGameUserSettings Sensitivity=1.00 InvertY=0`与实际`Ini=...`。若此前已保存过本章偏好，以实际值为准；首次没有设置文件时才承诺默认值。
3. 关闭控制台，Tab保持未锁定，移动鼠标看自由镜头。重新打开控制台执行下条后关闭控制台：

```text
et.InputSettings apply 0.5 1
```

4. 应输出Save requested与`.50 / 1`。相同鼠标位移的转角更小，上下方向相对原来反转。别用锁定中静止镜头判断倍率：锁定跟随不受此倍数控制。
5. 再执行下条，应Rejected且查询仍为`.50 / 1`；不用故意输入NaN或改坏个人ini，非法边界已自动化覆盖。

```text
et.InputSettings apply 0 0
```

### 7.2 一次真实重新读取与恢复默认

1. 保留`.50 / 1`，Stop后重新Play，查询仍是`.50 / 1`。这一步只证明同一引擎实例保持，**还不证明磁盘读取**。
2. Stop、Save All并完整关闭UE编辑器。重新打开工程与正式地图、Play，再执行`et.InputSettings`。仍为`.50 / 1`才是此次真实持久化检查；不需要新Cook或打包。
3. 如果没有恢复，先保留日志中的完整Ini路径告诉我，别删除缓存 / 个人文件。不同Editor / 独立包的配置目录可以不同，本章不要求旧M2包识别新设置。
4. 执行下面命令恢复本章最终默认值，随后查询确认`1.00 / 0`。该命令只恢复输入项，不改窗口 / 分辨率 / 画质。

```text
et.InputSettings defaults
```

### 7.3 受影响镜头与最短旧功能检查

1. 未锁定时恢复当前正常鼠标手感；Tab锁定敌人，确认原后侧俯视构图仍跟随、鼠标不争抢，再Tab解除后鼠标可转动。
2. P打开原页，鼠标点击继续；只查恢复后无需点视口即可移动 / 转向，无积攒镜头输入。祭坛E打开原确认页再继续一次，确认页面入口仍正常。
3. 左键一次 / 2一次 / 移动跳跃，确认HUD正常；Stop结束。不重复整套装卸、Retry / Continue、死亡矩阵或旧能力测试。

## 8. 问题定位、性能与学习要点

- `Settings class missing` / 首项测试Fail：先看第4.3节键值和大小写，再确认已完整退出并重开编辑器。不要NewObject第二个实例掩盖配置问题，镜头回退能动也不代表接入正确。
- include无法找到：检查Public/Settings与Private/Settings层级、文件名完全一致；generated.h只放头文件最后一条include，不手写、不从别的cpp复制。
- 相机不随鼠标变化：先退出控制台 / P并Tab解除锁定。锁定跟随仍由TargetLockCamera控制，自动跟随速度不乘鼠标灵敏度；原动作门控不变。
- 运行成本只增加原Look输入回调内一次实例查询、值拷贝与轴运算，不增加每帧扫描 / 磁盘读取。显式确认才保存，无Notify、GE、ASC或UI反向依赖。
- 个人配置在Saved / 平台用户目录，已受gitignore覆盖；共享代码 / DefaultEngine键才入库。本章不把个人偏好扩散到每个角色默认值或试炼会话。

对照本机5.5引擎`GameFramework/GameUserSettings.h`与`Engine/Private/GameUserSettings.cpp`的LoadSettings / SaveSettings；[官方5.5 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/UGameUserSettings?application_version=5.5)说明其持久化职责。面试重点是“设置草稿、运行应用、落盘”三个时机，以及个人偏好与游戏进度不同的生命周期；不要将纯规则测试写成磁盘或真实Viewport测试。

## 9. 完成后反馈模板

```text
M3-05A1 完成
三新Core / Character.cpp / Engine.ini；Editor Build / 三索引 / Game Build：
引擎Class正确 / InputPreferences与CameraRules各一次：
.50与反转实际生效 / 非法0拒绝保持原值：
完整关闭重开UE后仍.50 / 1；最后defaults恢复1 / 0：
自由-锁定-解锁 / P继续 / 祭坛E与最短旧功能检查：
Source / Config已保存；未改资产；失败或手感问题（无填无）：
完成时分 / 手动代码 / 阅读配置 / 检查 / 构建与重启等待：
```

不必贴长日志或记测试秒点；我读取本轮日志。出错贴真实error文字；Designer / 焦点等视觉问题附完整窗口截图和发生步骤，复杂问题再按当次授权直接排查。

## 10. Commit / Push（固定放在最后）

**当前收尾文档**：C2代码 / 两资产已在本地`f0f4e76`，当前只Stage C2验收报告、C2收尾 / 来源清单 / 状态文档及本指南。建议`docs: verify relic confirmation UI and prepare input settings`；中文`docs: 完成遗物确认界面验收并准备输入设置`。不重复Stage未变化源码 / 资产，未联网核验远端。

**本章手动实施完成后**，CLion检查以下差异再Stage：

- 三新：第2节Settings头 / 实现与InputPreferences测试。
- 两旧：Character.cpp的单include与Look替换、DefaultEngine.ini的单键。
- 实际有改的本指南 / 进度 / 报告等共享文档；尚未生成的验收报告不要创建空文件。
- 不Stage个人GameUserSettings.ini、Saved、Intermediate、Binaries、.idea / CMake生成、日志 / Artifacts候选或旧包；本章无Content改动。

实施建议：`feat: persist input preferences and apply mouse sensitivity`。

专业中文：`feat: 持久化输入偏好并应用鼠标镜头灵敏度`。

Stage前查看最终差异；暂存后又修改需重新Stage最新版本，再Commit / Push。Git由用户操作，助手未提交或推送。
