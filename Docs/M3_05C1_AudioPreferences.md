# M3-05C1：音量偏好、合法值与本机保存

2026-11-06 18:55完成。**[本章验收通过](Reports/M3_05C1_Verification.md)，无待补，进入[C2实际声音路由](M3_05C2_AudioRouting.md)。** 以下保留原C1实施步骤；C1的store数据入口会在C2迁移为apply，不要求重跑本章。

## 1. 本批交付与后续声音效果

本批建立总音量、音乐、音效三项线性偏好，取值0–1，默认均1。合法值存进引擎现有的`UETGameUserSettings`，显式提交后保存到本机ini；独立游戏重启仍能读回。非法值拒绝，不偷偷夹成另一组设置。三项与试炼装备 / 检查点无关，不写进GI试炼状态。

**本批只完成数据与保存，不产生声音，也不改变实际混音。** 日志固定输出`RuntimeAudioApplied=0`。`CombinedMusic`与`CombinedEffects`只是总音量乘对应通道的数学诊断；数值变零不能当作“游戏已静音”的验收证据。

| 分批 | 交付与通过标准 |
| --- | --- |
| 当前C1 | 三项偏好 / 合法值 / 本机保存，一项自动化与两轮专用-game读取 |
| C2 | 实际SoundClass / SoundMix路由及来源明确的零成本可听探针；通过实际播放验证各路音量与切图恢复 |
| C3 | 主菜单 / Pause复用音量草稿卡，应用 / 取消 / 默认与焦点；复用C2声音入口 |

工程当前尚未建立本项目正式声音路由，本批不购买素材，不在蓝图中搭临时验收系统。后续父子SoundClass分别设置**原始Master / Music / Effects**，由层级合成；不能把已相乘的Combined值再次乘Master，否则总音量会重复衰减。

本批三新 / 两旧Core均已实施。实际225 Source / 128cpp（Core87 / UI38 / Editor3）、40自动化、302 Content / 来源83。代码40 / 阅读10 / 检查10，共60分钟、构建启动等待5；一专项与两轮独立保存 / 重启读取通过。

## 2. 文件、准备与修改边界

先Save All，关闭所有UE窗口。普通C++仍由你手动输入。新建以下三文件，路径中的模块是**EmberTrial**；不要放进EmberTrialUI，也不要手建generated.h。

```text
EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSettingsTypes.h        新
EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSettingsTypes.cpp     新
EmberTrial/Source/EmberTrial/Tests/ETAudioPreferencesTests.cpp            新
EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h         旧
EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp       旧
```

仅第3节三新文件使用完整代码；第4节只对两个旧文件做定位替换，保留输入、显示预览保护、画质与既有控制台命令。`Build.cs`、`.uproject`、共享Config、所有Widget / Blueprint / 地图都不改。第4节代码块中的“原文”用于定位，**只粘贴“替换为”**，不要把两段一起留在源码中。

## 3. 三份新文件完整代码

### 3.1 Public/Settings/ETAudioSettingsTypes.h

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSettingsTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "ETAudioSettingsTypes.generated.h"

/** 独立线性音量偏好；不保存SoundClass / World，不代表声音已经应用。 */
USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETAudioPreferences
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float MasterVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float MusicVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float EffectsVolume = 1.0f;

    bool IsValid() const;

    // 仅诊断数学组合；后续父子SoundClass分别设置原始值，不能再乘一次Master。
    bool TryGetEffectiveVolumes(float& OutMusic, float& OutEffects) const;
};
```

### 3.2 Private/Settings/ETAudioSettingsTypes.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSettingsTypes.cpp`

```cpp
#include "Settings/ETAudioSettingsTypes.h"

namespace ETAudioPreferencesPrivate
{
bool IsValidVolume(const float Value)
{
    return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
}
} // namespace ETAudioPreferencesPrivate

bool FETAudioPreferences::IsValid() const
{
    return ETAudioPreferencesPrivate::IsValidVolume(MasterVolume) &&
        ETAudioPreferencesPrivate::IsValidVolume(MusicVolume) &&
        ETAudioPreferencesPrivate::IsValidVolume(EffectsVolume);
}

bool FETAudioPreferences::TryGetEffectiveVolumes(float& OutMusic, float& OutEffects) const
{
    const FETAudioPreferences Copy = *this; // 输出可引用自身字段，先保存输入。
    OutMusic = OutEffects = 0.0f;
    if (!Copy.IsValid())
    {
        return false;
    }
    OutMusic = Copy.MasterVolume * Copy.MusicVolume;
    OutEffects = Copy.MasterVolume * Copy.EffectsVolume;
    return true;
}
```

### 3.3 Tests/ETAudioPreferencesTests.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETAudioPreferencesTests.cpp`

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETAudioSettingsTypes.h"
#include "Misc/AutomationTest.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAudioPreferencesTest,
    "EmberTrial.M3.Settings.AudioPreferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAudioPreferencesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETAudioPreferences Value;
    float Music = -1.0f;
    float Effects = -1.0f;
    TestTrue(TEXT("Defaults are valid"), Value.IsValid());
    TestTrue(TEXT("Defaults produce unity gains"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Default music"), Music, 1.0f);
    TestEqual(TEXT("Default effects"), Effects, 1.0f);

    Value.MasterVolume = 0.5f;
    Value.MusicVolume = 0.4f;
    Value.EffectsVolume = 0.2f;
    TestTrue(TEXT("Independent channel values accepted"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Master applied once to music"), Music, 0.2f);
    TestEqual(TEXT("Master applied once to effects"), Effects, 0.1f);
    Value.MasterVolume = 0.0f;
    TestTrue(TEXT("Mute is a valid preference"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Muted music gain"), Music, 0.0f);
    TestEqual(TEXT("Muted effects gain"), Effects, 0.0f);
    TestEqual(TEXT("Mute retains channel preference"), Value.MusicVolume, 0.4f);
    Value.MasterVolume = 1.0f;
    TestTrue(TEXT("Unmute retains independent values"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Music restored from preference"), Music, 0.4f);

    Value.MusicVolume = -0.1f;
    TestFalse(TEXT("Negative volume rejected"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Failure clears music output"), Music, 0.0f);
    TestEqual(TEXT("Failure clears effects output"), Effects, 0.0f);
    Value.MusicVolume = 1.1f;
    TestFalse(TEXT("Above unity rejected without clamping"), Value.IsValid());
    Value.MusicVolume = 0.4f;
    Value.MasterVolume = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN master rejected"), Value.IsValid());
    Value.MasterVolume = 1.0f;
    Value.EffectsVolume = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Infinite effects rejected"), Value.IsValid());
    Value.EffectsVolume = 0.2f;
    Value.MasterVolume = 0.5f;
    TestTrue(TEXT("Self-field output alias supported"),
        Value.TryGetEffectiveVolumes(Value.MusicVolume, Value.EffectsVolume));
    TestEqual(TEXT("Alias uses original music input"), Value.MusicVolume, 0.2f);
    TestEqual(TEXT("Alias uses original effects input"), Value.EffectsVolume, 0.1f);
    return true;
}
#endif
```

## 4. 两个旧文件的七处增量修改

前三处在`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h`；后四处在对应`Private/Settings/ETGameUserSettings.cpp`。按顺序操作，不用本节覆盖整个旧文件。

### 4.1 头文件：在generated.h之前包含音量类型

本文件最后一条include仍须是`#include "ETGameUserSettings.generated.h"`，不能把音量头放到它后面。

原文：

```cpp
#include "GameFramework/GameUserSettings.h"
```

替换为：

```cpp
#include "GameFramework/GameUserSettings.h"
#include "Settings/ETAudioSettingsTypes.h"
```

### 4.2 头文件：保留输入Getter并增加音量接口

上方既有UFUNCTION仍归原输入Getter；新增UFUNCTION归音量Getter。Setter是原生接口，本批不暴露蓝图，不混音、不保存。

原文：

```cpp
    FETInputPreferences GetInputPreferences() const { return InputPreferences; }
```

替换为：

```cpp
    FETInputPreferences GetInputPreferences() const { return InputPreferences; }

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Settings")
    FETAudioPreferences GetAudioPreferences() const { return AudioPreferences; }

    /** 仅更新本实例偏好内存，不混音 / 保存；调用方显式确认后请求SaveSettings。 */
    bool SetAudioPreferences(const FETAudioPreferences& Preferences);
```

### 4.3 头文件：在private区保留输入Config并增加音量Config

原InputPreferences上方的`UPROPERTY(Config)`保留；AudioPreferences另有自己的UPROPERTY，不共用宏。

原文：

```cpp
    FETInputPreferences InputPreferences;
```

替换为：

```cpp
    FETInputPreferences InputPreferences;

    UPROPERTY(Config)
    FETAudioPreferences AudioPreferences;
```

### 4.4 实现：在LoadSettings函数之前增加Setter

替换的是原函数签名这一行。其后LoadSettings原函数体保留；最终Setter是一个完整独立函数，不要嵌进LoadSettings内部。Setter只允许引擎持有的实际实例，并拒绝视频保护期间修改。

原文：

```cpp
void UETGameUserSettings::LoadSettings(bool bForceReload)
```

替换为：

```cpp
bool UETGameUserSettings::SetAudioPreferences(const FETAudioPreferences& Preferences)
{
    // 数据提交不混音；视频保护期间拒绝，不把本批开发入口当成设置权限绕过。
    if (this != GetETGameUserSettings() || bVideoPreviewGuard || !Preferences.IsValid())
    {
        return false;
    }
    AudioPreferences = Preferences;
    return true;
}

void UETGameUserSettings::LoadSettings(bool bForceReload)
```

### 4.5 实现：LoadSettings保留输入校验并追加音量校验

这一段仍在LoadSettings的原函数大括号内、Super之后。损坏的音量只恢复音量默认，不顺带重置输入 / 画质或立刻写盘。

原文：

```cpp
    if (!InputPreferences.IsValid())
    {
        InputPreferences = FETInputPreferences{};
        UE_LOG(LogETUserSettings, Warning,
            TEXT("[M3-05A1] Invalid stored input preferences; restored input defaults"));
    }
```

替换为：

```cpp
    if (!InputPreferences.IsValid())
    {
        InputPreferences = FETInputPreferences{};
        UE_LOG(LogETUserSettings, Warning,
            TEXT("[M3-05A1] Invalid stored input preferences; restored input defaults"));
    }
    if (!AudioPreferences.IsValid())
    {
        AudioPreferences = FETAudioPreferences{};
        UE_LOG(LogETUserSettings, Warning,
            TEXT("[M3-05C1] Invalid stored audio preferences; restored audio defaults"));
    }
```

### 4.6 实现：全局默认补上音量默认

此处是引擎全局默认流程。未来音量页的“恢复默认”只改本卡草稿，不调用全局SetToDefaults。

原文：

```cpp
void UETGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    InputPreferences = FETInputPreferences{};
}
```

替换为：

```cpp
void UETGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    InputPreferences = FETInputPreferences{};
    AudioPreferences = FETAudioPreferences{};
}
```

### 4.7 实现：在文件末尾开发命令区追加音量入口

定位文件**最末尾**、与既有`#if !UE_BUILD_SHIPPING`配对的`#endif`；当前文件只有这一条endif。保留此前Input与Graphics两个命令命名空间，把以下整段放在其后。命令只在非Shipping构建注册；不是正式音量界面。

原文：

```cpp
#endif
```

替换为：

```cpp
// ---- 本批只保存偏好；C2再把同一数据接入实际声音路由 ----
namespace ETAudioPreferencesConsole
{
void Execute(const TArray<FString>& Args, UWorld* World)
{
    UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    if (!Settings || !World || !World->IsGameWorld())
    {
        UE_LOG(LogETUserSettings, Display, TEXT("[M3-05C1] Start a game world first"));
        return;
    }

    if (!Args.IsEmpty())
    {
        FETAudioPreferences Request;
        if (Args.Num() != 4 || !Args[0].Equals(TEXT("store"), ESearchCase::IgnoreCase) ||
            !FDefaultValueHelper::ParseFloat(Args[1], Request.MasterVolume) ||
            !FDefaultValueHelper::ParseFloat(Args[2], Request.MusicVolume) ||
            !FDefaultValueHelper::ParseFloat(Args[3], Request.EffectsVolume))
        {
            UE_LOG(LogETUserSettings, Display,
                TEXT("[M3-05C1] Usage: et.AudioSettings [store <master:0..1> <music:0..1> <effects:0..1>]"));
            return;
        }
        const bool bAllowed = !GIsEditor && !World->bIsTearingDown &&
            World->WorldType == EWorldType::Game && World->GetNetMode() == NM_Standalone;
        const bool bStored = bAllowed && Settings->SetAudioPreferences(Request);
        if (bStored)
        {
            Settings->SaveSettings();
            UE_LOG(LogETUserSettings, Display,
                TEXT("[M3-05C1] Audio preferences stored SaveRequested=1 RuntimeAudioApplied=0"));
        }
        UE_LOG(LogETUserSettings, Display, TEXT("[M3-05C1] StoreAccepted=%d"), bStored ? 1 : 0);
    }

    const FETAudioPreferences Value = Settings->GetAudioPreferences();
    float Music = 0.0f;
    float Effects = 0.0f;
    const bool bValid = Value.TryGetEffectiveVolumes(Music, Effects);
    UE_LOG(LogETUserSettings, Display,
        TEXT("[M3-05C1] Audio Valid=%d Master=%.3f Music=%.3f Effects=%.3f CombinedMusic=%.3f CombinedEffects=%.3f RuntimeAudioApplied=0 Ini=%s"),
        bValid ? 1 : 0, Value.MasterVolume, Value.MusicVolume, Value.EffectsVolume,
        Music, Effects, *GGameUserSettingsIni);
}

FAutoConsoleCommandWithWorldAndArgs Command(
    TEXT("et.AudioSettings"),
    TEXT("Audio preferences only: query; store <master> <music> <effects>; runtime routing follows in C2"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Execute), ECVF_Cheat);
} // namespace ETAudioPreferencesConsole
#endif
```

## 5. 接口分工与本批自查

| 接口 | 负责什么 | 不负责什么 |
| --- | --- | --- |
| FETAudioPreferences | 三个独立值、有限值 / 范围校验、组合增益诊断 | World / SoundClass / 文件读写 |
| GetAudioPreferences | 读取当前偏好副本 | 混音、保存、创建第二个设置实例 |
| SetAudioPreferences | 校验请求与引擎实例、预览保护；成功更新内存 | 保存、播放声音、恢复全局默认 |
| et.AudioSettings store | 开发期独立单机入口；Setter成功后显式请求一次保存 | PIE修改、正式音量UI、实际音量应用 |

`UPROPERTY(Config)`使三项随现有GameUserSettings保存；不新增设置服务、音频Subsystem或业务Tick。三值保持线性，0合法；这是音量百分比的基础，不是分贝标尺。总音量0时保留Music / Effects偏好，恢复总音量后仍能沿用原通道值。

保存前只做四项自查：新头generated.h为最后include；两个旧文件七处都在正确作用域；原Input / Graphics命令及视频Guard保留；没有改UI / 共享Config。不要重新设计蓝图测试函数。

## 6. 正常Build、索引和一次自动化

### 6.1 关闭UE后的正常构建

在PowerShell逐段执行。路径保留`\`，不要把复制结果改成`D:UE5...`或`.ToolsRefresh...`。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先构建Editor再刷新索引；失败时停在第一条真实error，不继续Game Build。成功后在CLion刷新CMake / 重新载入既有工程，可见三新文件且头文件可导航。预期Core87 / UI38 / Editor3个cpp，不能把generated.cpp混进统计。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

### 6.2 自动化一次

打开工程，保持非Play，Output Log执行：

```text
Automation RunTests EmberTrial.M3.Settings.AudioPreferences
```

只需一次Success。它检查默认、独立通道、总音量0保留子通道、非法 / 非有限值拒绝及失败输出；不冒充实际音频设备测试，不跑全40项或Cook。

### 6.3 PIE只读拒绝与最短基线

正式地图Play，在**游戏控制台**（`~`）依次执行：

```text
et.AudioSettings
et.AudioSettings store 0.5 0.4 0.2
et.AudioSettings
```

PIE默认应Valid1、三项均1；store应`StoreAccepted=0`，前后值一致。若个人配置原本不是默认，按实际基线比较，不用改共享ini。收起控制台，P打开看原设置卡仍正常，继续后移动转向；Stop。没有新音量卡、没有声音变化符合本批范围。

## 7. 一组独立窗口保存检查

两次启动只用于证明保存与新进程读取。无需搭Blueprint测试入口、跑战斗 / 装备旧矩阵或抢倒计时输入。

### 7.1 第一轮启动脚本就在这里

Save All并关闭UE。以下是**本文代码块**，无需寻找其他.ps1；整段复制到PowerShell。单独ini隔离个人测试值，不能换成项目DefaultGame.ini。主菜单已有显示 / 画质卡可能读到此专用文件的默认值，这不改变共享配置。

```powershell
Set-Location 'E:\UE_Project\Demo'
$audioRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05C1\Runtime'
New-Item -ItemType Directory -Path $audioRuntime -Force | Out-Null
$audioIni = Join-Path $audioRuntime 'AudioUser.ini'
$audioLog = Join-Path $audioRuntime 'Audio02.log'
$audioArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    '-NoSplash'
    ('-GameUserSettingsINI="{0}"' -f $audioIni)
    ('-abslog="{0}"' -f $audioLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $audioArgs -WindowStyle Normal
```

等待主菜单自行就绪。第一次打开新文件默认三项应1；若目录中已做过本批测试，以实际值为准并保留日志，不靠删除文件制造“新进程”。

### 7.2 只读、合法保存与非法拒绝

按`~`打开游戏控制台，一次执行一行：

```text
et.AudioSettings
et.AudioSettings store 0.5 0.4 0.2
et.AudioSettings store -0.1 0.4 0.2
et.AudioSettings store 0.5 1.1 0.2
et.AudioSettings
```

预期：第一条store Accepted1 / SaveRequested1，原始值0.5 / 0.4 / 0.2；诊断组合0.2 / 0.1。两条非法store均Accepted0，最后查询仍为该合法组。无需复制长日志，我会从Audio01.log读取。始终`RuntimeAudioApplied=0`，本章不以听感判定。

### 7.3 总音量零与最终保存值

继续执行：

```text
et.AudioSettings store 0 0.4 0.2
et.AudioSettings
et.AudioSettings store 0.6 0.3 0.8
et.AudioSettings
```

第一组组合增益为0 / 0，Music / Effects原始偏好仍0.4 / 0.2。最后明确保存**0.6 / 0.3 / 0.8**，组合0.18 / 0.48，Accepted1。选这个与全1不同的值，是为了区分读回数据与重新默认；不要求现在实际静音或改变听感。

收起控制台，开始试炼 → P → 原输入 / 显示 / 画质卡仍可见，继续后移动转向一次。没有新增音量UI、没有改变HUD资源符合预期。正常回菜单退出，不用测试旧能力矩阵。

### 7.4 第二轮只查询，不能先store

再次运行**7.1整段脚本**，只把`$audioLog = Join-Path $audioRuntime 'Audio01.log'`改成：

```powershell
$audioLog = Join-Path $audioRuntime 'Audio02.log'
```

其余内容尤其`AudioUser.ini`保持相同。不要在第一轮还运行时启动第二轮。全新进程主菜单就绪后，游戏控制台**仅执行**：

```text
et.AudioSettings
```

预期Valid1、Master0.600 / Music0.300 / Effects0.800、Combined0.180 / 0.480，且本进程此前没有store。正常退出。这是保存通过的关键证据；ini有这些值或第一轮查询成功不能单独替代新进程读取。

本轮专用配置保留在Artifacts，不改成全1也不上传。两轮日志与ini路径由我读取；以后用新配置启动正式开发时仍采用各自设置，不混用测试专用ini。

## 8. 问题定位与完成反馈

若generated.h缺失，先核对第2节目录与头文件名，不自己创建.generated.h。若Setter未声明，核对4.2是否放在类内public区；若原显示或画质编译失败，发送第一条error与附近代码，不用先删除缓存。

自动化失败请发送测试全名、Expected行和文件行号；UI / 窗口现象发可见截图，日志报错优先复制文本。出现非预期声音变化先说明实际启动的是哪个包 / -game与是否有额外资源；本批不承诺声音路由已生效。

```text
M3-05C1 完成
三新 / 两旧Core；关闭UE后Editor Build / Core87-UI38-Editor3索引 / Game Build：
AudioPreferences一次：
PIE只读 / store拒绝；原P与移动转向短查：
独立0.5-0.4-0.2保存 / 两非法拒绝不变 / 总0保留子通道：
最终0.6-0.3-0.8；新进程同ini只查询读回 / RuntimeAudioApplied0：
两轮日志 / 专用ini保持 / 正常退出：
Source已保存，资产 / 共享Config未改；失败或不理解处（无填无）：
完成时分 / 手动代码 / 阅读 / 检查 / 构建启动等待：
```

## 9. Commit / Push（固定放在最后）

C1五源码已在本地`99b8146`。历史实现建议：英文`feat: add validated audio preferences with local persistence`；专业中文`feat: 新增带合法值校验与本机保存的音量偏好`。已提交实现不重复Stage，远端状态由你确认。

当前Stage本指南收尾、[验收报告](Reports/M3_05C1_Verification.md)、[C2新指南](M3_05C2_AudioRouting.md)与实际修改的状态文档；具体当前清单见C2第9.1节。

- 英文：`docs: verify audio preferences and prepare runtime audio routing`
- 专业中文：`docs: 完成音量偏好验收并准备运行时声音路由`

来源83保持。C2尚未实施，不Stage候选 / 空报告 / 未来资源行；Artifacts证据 / 个人ini与缓存留本地。Stage后继续修改需重新Stage → Review → Commit / Push由用户操作。
