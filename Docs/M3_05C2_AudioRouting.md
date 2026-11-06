# M3-05C2：实际音量路由与原创试听声音

2026-11-06 19:45用户完成。**[本章验收通过](Reports/M3_05C2_Verification.md)，无待补，进入[C3共用音量设置卡](M3_05C3_AudioSettingsUI.md)。** 以下保留完整代码与资源步骤，6.2–6.4的助手实际操作记录保留，不要求重跑旧矩阵。

## 1. 玩家效果与职责

总音量、音乐、音效三项开始真正控制声音：总音量为0时两路静音，音乐为0不影响音效，音效为0不影响音乐。菜单→试炼→菜单时重新使用已保存偏好，全新进程同样恢复。当前用两段原创合成声音验证，不代表最终配乐、挥刀音效、3D衰减或完整音效系统。

本批不新增设置卡，C3再复用本批实际应用接口制作共用界面。开发控制台沿用`et.AudioSettings`，从C1的`store`迁移为`apply`；旧数据专用Setter移除，避免未来只保存却没混音。查询仍无参数。

| 所有者 | 职责 |
| --- | --- |
| GameUserSettings | 合法个人偏好、视频保护、明确应用成功后请求一次保存 |
| GI AudioSubsystem | 配置资源保活、当前World的Mix、切图清理 / 重建；不另存偏好 |
| 三个SoundClass / 一个SoundMix | 音乐 / 音效归类及乘法音量路由，不放业务蓝图 |
| 两段SoundWave | 单次2D开发试听，无循环或自动背景播放 |

本批三新 / 两旧Core、DefaultGame.ini、六新UAsset与两源WAV均已实施。实际228 Source / 130cpp（Core89 / UI38 / Editor3）、41自动化、308 Content / 来源91。代码60 / 资产30 / 阅读10 / 检查10，共110分钟、等待5；一专项、真实听感 / 切图与新进程恢复已验收。

## 2. 文件与准备

Save All并关闭UE。新文件都在**EmberTrial Core模块**，Tests也放Core自己的Tests；不要创建.generated.h。

```text
EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSubsystem.h        新
EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSubsystem.cpp     新
EmberTrial/Source/EmberTrial/Tests/ETAudioRoutingTests.cpp             新
EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h     旧
EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp   旧
EmberTrial/Config/DefaultGame.ini                                    旧配置
```

Build.cs / .uproject / UI源码 / 既有Widget / 地图不改。第3节只对三新文件用完整代码，第4节对旧文件逐块替换，保留其他输入 / 画质 / 视频实现。

## 3. 三份新文件完整代码

### 3.1 Public/Settings/ETAudioSubsystem.h

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Settings/ETAudioSettingsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ETAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundClass;
class USoundMix;
struct FSoundClassAdjuster;

/** 本机游戏音量路由；资产从共享配置读取，偏好由GameUserSettings持有。 */
UCLASS(Config = Game, DefaultConfig)
class EMBERTRIAL_API UETAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 成功表示混音请求已提交给引擎，不表示声卡已完成播放。
    bool ApplyRuntime(const FETAudioPreferences& Value, UWorld* World);
    bool IsRoutingApplied(const UWorld* World) const;
    void StopProbe();

#if !UE_BUILD_SHIPPING
    bool PlayProbe(bool bMusic, UWorld* World);
#endif

    // 无声音副作用；构造一次Master递归 + 两个叶子独立增益。
    static bool BuildAdjusters(const FETAudioPreferences& Value,
        USoundClass* Master, USoundClass* Music, USoundClass* Effects,
        USoundMix* Mix, TArray<FSoundClassAdjuster>& Out);

private:
    void HandlePostLoadMap(UWorld* World);
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    void ReleaseWorld();

    UPROPERTY(Config)
    TSoftObjectPtr<USoundClass> MasterClass;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundClass> MusicClass;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundClass> EffectsClass;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundMix> UserMix;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundBase> MusicProbe;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundBase> EffectsProbe;

    // 保活本服务资源；不修改资产默认Volume，不清除其他系统的Mix。
    UPROPERTY(Transient)
    TObjectPtr<USoundClass> LoadedMaster;
    UPROPERTY(Transient)
    TObjectPtr<USoundClass> LoadedMusic;
    UPROPERTY(Transient)
    TObjectPtr<USoundClass> LoadedEffects;
    UPROPERTY(Transient)
    TObjectPtr<USoundMix> LoadedMix;
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ProbeComponent;

    TWeakObjectPtr<UWorld> BoundWorld;
    FDelegateHandle PostMapHandle;
    FDelegateHandle CleanupHandle;
    bool bMixPushed = false;
};
```

### 3.2 Private/Settings/ETAudioSubsystem.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSubsystem.cpp`

```cpp
#include "Settings/ETAudioSubsystem.h"
#include "Settings/ETGameUserSettings.h"
#include "Components/AudioComponent.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreMisc.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAudio, Log, All);

// ---- 资产合同：专用三类树 / 空的常驻Mix；只沿指定方向传播 ----

bool UETAudioSubsystem::BuildAdjusters(const FETAudioPreferences& Value,
    USoundClass* Master, USoundClass* Music, USoundClass* Effects,
    USoundMix* Mix, TArray<FSoundClassAdjuster>& Out)
{
    Out.Reset();
    if (!Value.IsValid() || !IsValid(Master) || !IsValid(Music) ||
        !IsValid(Effects) || !IsValid(Mix) ||
        Master == Music || Master == Effects || Music == Effects ||
        Master->ParentClass || Music->ParentClass != Master || Effects->ParentClass != Master ||
        Master->ChildClasses.Num() != 2 || !Master->ChildClasses.Contains(Music) ||
        !Master->ChildClasses.Contains(Effects) || !Music->ChildClasses.IsEmpty() ||
        !Effects->ChildClasses.IsEmpty() ||
        Mix->bApplyEQ || !Mix->SoundClassEffects.IsEmpty() ||
        !FMath::IsFinite(Mix->Duration) || Mix->Duration >= 0.0f ||
        !FMath::IsNearlyZero(Mix->InitialDelay) ||
        !FMath::IsNearlyZero(Mix->FadeInTime) || !FMath::IsNearlyZero(Mix->FadeOutTime))
    {
        return false;
    }
    USoundClass* Classes[] = { Master, Music, Effects };
    const float Volumes[] = { Value.MasterVolume, Value.MusicVolume, Value.EffectsVolume };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (!FMath::IsNearlyEqual(Classes[Index]->Properties.Volume, 1.0f) ||
            !FMath::IsNearlyEqual(Classes[Index]->Properties.Pitch, 1.0f) ||
            !Classes[Index]->PassiveSoundMixModifiers.IsEmpty())
        {
            Out.Reset();
            return false;
        }
        FSoundClassAdjuster Entry;
        Entry.SoundClassObject = Classes[Index];
        Entry.VolumeAdjuster = Volumes[Index];
        Entry.PitchAdjuster = 1.0f;
        // UE先解析类树再应用SoundMix。Master递归一次，两个叶子不递归。
        Entry.bApplyToChildren = Index == 0;
        Out.Add(Entry);
    }
    return true;
}

// ---- GI观察句柄；每个实际World仅持有本Mix一次，切图精确释放 ----

bool UETAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return Super::ShouldCreateSubsystem(Outer) && !GIsEditor && !IsRunningDedicatedServer();
}

void UETAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadedMaster = MasterClass.LoadSynchronous();
    LoadedMusic = MusicClass.LoadSynchronous();
    LoadedEffects = EffectsClass.LoadSynchronous();
    LoadedMix = UserMix.LoadSynchronous();
    PostMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &ThisClass::HandlePostLoadMap);
    CleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
        this, &ThisClass::HandleWorldCleanup);
}

void UETAudioSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostMapHandle);
    FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);
    PostMapHandle.Reset();
    CleanupHandle.Reset();
    ReleaseWorld();
    LoadedMaster = nullptr;
    LoadedMusic = nullptr;
    LoadedEffects = nullptr;
    LoadedMix = nullptr;
    Super::Deinitialize();
}

bool UETAudioSubsystem::ApplyRuntime(const FETAudioPreferences& Value, UWorld* World)
{
    TArray<FSoundClassAdjuster> Adjusters;
    if (!IsValid(World) || World->bIsTearingDown || World->WorldType != EWorldType::Game ||
        World->GetGameInstance() != GetGameInstance() || World->GetNetMode() != NM_Standalone ||
        !World->GetAudioDeviceRaw() ||
        !BuildAdjusters(Value, LoadedMaster, LoadedMusic, LoadedEffects, LoadedMix, Adjusters))
    {
        return false;
    }
    if (BoundWorld.Get() != World)
    {
        ReleaseWorld();
        BoundWorld = World;
    }
    for (const FSoundClassAdjuster& Entry : Adjusters)
    {
        UGameplayStatics::SetSoundMixClassOverride(World, LoadedMix, Entry.SoundClassObject,
            Entry.VolumeAdjuster, 1.0f, 0.05f, Entry.bApplyToChildren);
    }
    if (!bMixPushed)
    {
        UGameplayStatics::PushSoundMixModifier(World, LoadedMix);
        bMixPushed = true;
    }
    UE_LOG(LogETAudio, Display,
        TEXT("[M3-05C2] Routing applied World=%s Master=%.3f Music=%.3f Effects=%.3f"),
        *GetNameSafe(World), Value.MasterVolume, Value.MusicVolume, Value.EffectsVolume);
    return true;
}

bool UETAudioSubsystem::IsRoutingApplied(const UWorld* World) const
{
    return bMixPushed && World && BoundWorld.Get() == World;
}

void UETAudioSubsystem::HandlePostLoadMap(UWorld* World)
{
    if (!World || World->GetGameInstance() != GetGameInstance())
    {
        return;
    }
    const UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    const bool bApplied = Settings && ApplyRuntime(Settings->GetAudioPreferences(), World);
    UE_LOG(LogETAudio, Display, TEXT("[M3-05C2] World ready World=%s RoutingApplied=%d"),
        *GetNameSafe(World), bApplied ? 1 : 0);
}

void UETAudioSubsystem::HandleWorldCleanup(
    UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    if (BoundWorld.Get() == World)
    {
        ReleaseWorld();
    }
}

void UETAudioSubsystem::ReleaseWorld()
{
    StopProbe();
    if (UWorld* World = BoundWorld.Get(); World && bMixPushed && World->GetAudioDeviceRaw())
    {
        UGameplayStatics::ClearSoundMixClassOverride(World, LoadedMix, LoadedMaster, 0.0f);
        UGameplayStatics::ClearSoundMixClassOverride(World, LoadedMix, LoadedMusic, 0.0f);
        UGameplayStatics::ClearSoundMixClassOverride(World, LoadedMix, LoadedEffects, 0.0f);
        UGameplayStatics::PopSoundMixModifier(World, LoadedMix);
        UE_LOG(LogETAudio, Display, TEXT("[M3-05C2] Routing released World=%s"),
            *GetNameSafe(World));
    }
    bMixPushed = false;
    BoundWorld.Reset();
}

// ---- 开发探针单次播放；换图 / 停止 / GI结束都清理 ----

void UETAudioSubsystem::StopProbe()
{
    if (IsValid(ProbeComponent))
    {
        ProbeComponent->Stop();
    }
    ProbeComponent = nullptr;
}

#if !UE_BUILD_SHIPPING
bool UETAudioSubsystem::PlayProbe(bool bMusic, UWorld* World)
{
    if (!IsRoutingApplied(World))
    {
        return false;
    }
    USoundBase* Sound = bMusic ? MusicProbe.LoadSynchronous() : EffectsProbe.LoadSynchronous();
    USoundClass* Expected = bMusic ? LoadedMusic.Get() : LoadedEffects.Get();
    if (!IsValid(Sound) || Sound->GetSoundClass() != Expected)
    {
        return false;
    }
    StopProbe();
    // 2D试听在暂停下仍播放；未来战斗3D音效另按游戏时间与衰减设计。
    ProbeComponent = UGameplayStatics::SpawnSound2D(World, Sound,
        1.0f, 1.0f, 0.0f, nullptr, false, true);
    UE_LOG(LogETAudio, Display, TEXT("[M3-05C2] Probe Channel=%s Started=%d"),
        bMusic ? TEXT("Music") : TEXT("Effects"), IsValid(ProbeComponent) ? 1 : 0);
    return IsValid(ProbeComponent);
}

namespace ETAudioProbeConsole
{
void Execute(const TArray<FString>& Args, UWorld* World)
{
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UETAudioSubsystem* Audio = GI ? GI->GetSubsystem<UETAudioSubsystem>() : nullptr;
    if (!Audio || Args.Num() != 1)
    {
        UE_LOG(LogETAudio, Display,
            TEXT("[M3-05C2] Probe unavailable; use native -game: et.AudioProbe music|effects|stop"));
        return;
    }
    if (Args[0].Equals(TEXT("stop"), ESearchCase::IgnoreCase))
    {
        Audio->StopProbe();
    }
    else if (Args[0].Equals(TEXT("music"), ESearchCase::IgnoreCase) ||
        Args[0].Equals(TEXT("effects"), ESearchCase::IgnoreCase))
    {
        const bool bAccepted = Audio->PlayProbe(
            Args[0].Equals(TEXT("music"), ESearchCase::IgnoreCase), World);
        UE_LOG(LogETAudio, Display, TEXT("[M3-05C2] ProbeAccepted=%d"), bAccepted ? 1 : 0);
    }
}
FAutoConsoleCommandWithWorldAndArgs Command(
    TEXT("et.AudioProbe"), TEXT("Play one project routing probe: music|effects|stop"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Execute), ECVF_Cheat);
} // namespace ETAudioProbeConsole
#endif
```

### 3.3 Tests/ETAudioRoutingTests.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETAudioRoutingTests.cpp`

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETAudioSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAudioRoutingTest,
    "EmberTrial.M3.Settings.AudioRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAudioRoutingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    TStrongObjectPtr<USoundClass> Master(NewObject<USoundClass>());
    TStrongObjectPtr<USoundClass> Music(NewObject<USoundClass>());
    TStrongObjectPtr<USoundClass> Effects(NewObject<USoundClass>());
    TStrongObjectPtr<USoundMix> Mix(NewObject<USoundMix>());
    Music->ParentClass = Effects->ParentClass = Master.Get();
    Master->ChildClasses = { Music.Get(), Effects.Get() };
    Mix->Duration = -1.0f;
    Mix->InitialDelay = Mix->FadeInTime = Mix->FadeOutTime = 0.0f;
    FETAudioPreferences Value;
    Value.MasterVolume = 0.5f;
    Value.MusicVolume = 0.4f;
    Value.EffectsVolume = 0.2f;
    TArray<FSoundClassAdjuster> Entries;
    auto Build = [&]()
    {
        return UETAudioSubsystem::BuildAdjusters(Value, Master.Get(),
            Music.Get(), Effects.Get(), Mix.Get(), Entries);
    };
    if (!TestTrue(TEXT("Dedicated three-class tree accepted"), Build()) ||
        !TestEqual(TEXT("Three independent overrides"), Entries.Num(), 3))
    {
        return false;
    }
    TestTrue(TEXT("Master propagates exactly once"), Entries[0].bApplyToChildren != 0);
    TestFalse(TEXT("Music remains a leaf override"), Entries[1].bApplyToChildren != 0);
    TestFalse(TEXT("Effects remains a leaf override"), Entries[2].bApplyToChildren != 0);
    TestEqual(TEXT("Raw music avoids multiplying master twice"), Entries[1].VolumeAdjuster, 0.4f);
    TestEqual(TEXT("Raw effects retained"), Entries[2].VolumeAdjuster, 0.2f);

    Effects->ParentClass = nullptr;
    TestFalse(TEXT("Unrelated effects class rejected"), Build());
    TestEqual(TEXT("Failure publishes no partial entries"), Entries.Num(), 0);
    Effects->ParentClass = Master.Get();
    Master->Properties.Volume = 0.5f;
    TestFalse(TEXT("Non-unity asset gain rejected"), Build());
    Master->Properties.Volume = 1.0f;
    Mix->SoundClassEffects.AddDefaulted();
    TestFalse(TEXT("Static mix cannot double-adjust volume"), Build());
    Mix->SoundClassEffects.Reset();
    Mix->Duration = 1.0f;
    TestFalse(TEXT("Expiring user mix rejected"), Build());
    Mix->Duration = -1.0f;
    Value.EffectsVolume = -1.0f;
    TestFalse(TEXT("Invalid request has no partial route"), Build());
    Value.EffectsVolume = 0.2f;
    Value.MasterVolume = 0.0f;
    if (!TestTrue(TEXT("Mute retains route"), Build()))
    {
        return false;
    }
    TestEqual(TEXT("Muted master is zero"), Entries[0].VolumeAdjuster, 0.0f);
    TestEqual(TEXT("Music preference survives mute"), Entries[1].VolumeAdjuster, 0.4f);
    return true;
}
#endif
```

## 4. 两个旧源码与共享配置增量

本节有六处：4.1修改ETGameUserSettings.h；4.2–4.4修改ETGameUserSettings.cpp；4.5–4.6修改DefaultGame.ini。每处只用“替换为”覆盖“原文”对应部分，不能两段都保留。不要用候选文件整体覆盖你现有工程。

### 4.1 头文件：迁移数据Setter到实际应用接口

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h`。

在public区替换。GetAudioPreferences与其他接口保留；旧SetAudioPreferences不再保留另一份声明。

原文：

```cpp
    /** 仅更新本实例偏好内存，不混音 / 保存；调用方显式确认后请求SaveSettings。 */
    bool SetAudioPreferences(const FETAudioPreferences& Preferences);
```

替换为：

```cpp
    /** 仅独立游戏提交实际路由并请求保存；视频保护或路由未就绪时拒绝。 */
    bool ApplyAudioPreferences(const FETAudioPreferences& Preferences, UWorld* ContextWorld);
```

### 4.2 实现：补充路由与GameInstance包含

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp`。

顶部包含增加两行，原其余include保留。头文件的generated.h仍是最后include。

原文：

```cpp
#include "Settings/ETGraphicsSettingsTypes.h"
```

替换为：

```cpp
#include "Settings/ETGraphicsSettingsTypes.h"
#include "Settings/ETAudioSubsystem.h"
#include "Engine/GameInstance.h"
```

### 4.3 实现：完整替换原SetAudioPreferences函数

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp`。

函数位置不变。只替换此函数，不改其后的LoadSettings；校验与路由成功后才更新偏好并请求保存。

原文：

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
```

替换为：

```cpp
bool UETGameUserSettings::ApplyAudioPreferences(
    const FETAudioPreferences& Preferences, UWorld* ContextWorld)
{
    if (this != GetETGameUserSettings() || bVideoPreviewGuard || !Preferences.IsValid() ||
        GIsEditor || !IsValid(ContextWorld) || ContextWorld->bIsTearingDown ||
        ContextWorld->WorldType != EWorldType::Game || ContextWorld->GetNetMode() != NM_Standalone)
    {
        return false;
    }
    UGameInstance* GI = ContextWorld->GetGameInstance();
    UETAudioSubsystem* Audio = GI ? GI->GetSubsystem<UETAudioSubsystem>() : nullptr;
    if (!Audio || !Audio->ApplyRuntime(Preferences, ContextWorld))
    {
        return false;
    }
    AudioPreferences = Preferences;
    SaveSettings();
    UE_LOG(LogETUserSettings, Display,
        TEXT("[M3-05C2] Audio applied SaveRequested=1 RoutingApplied=1"));
    return true;
}
```

### 4.4 实现：完整替换原音量命令命名空间

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp`。

从原“本批只保存偏好”注释起，到ETAudioPreferencesConsole的命名空间闭合为止，整体替换。文件末尾原#endif保留；Input / Graphics命令保留。旧store不再使用。

原文：

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
```

替换为：

```cpp
// ---- 音量入口复用实际应用合同；C1的store数据入口已迁移 ----
namespace ETAudioPreferencesConsole
{
void Execute(const TArray<FString>& Args, UWorld* World)
{
    UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    if (!Settings || !World || !World->IsGameWorld())
    {
        return;
    }
    if (!Args.IsEmpty())
    {
        FETAudioPreferences Request;
        if (Args.Num() != 4 || !Args[0].Equals(TEXT("apply"), ESearchCase::IgnoreCase) ||
            !FDefaultValueHelper::ParseFloat(Args[1], Request.MasterVolume) ||
            !FDefaultValueHelper::ParseFloat(Args[2], Request.MusicVolume) ||
            !FDefaultValueHelper::ParseFloat(Args[3], Request.EffectsVolume))
        {
            UE_LOG(LogETUserSettings, Display,
                TEXT("[M3-05C2] Usage: et.AudioSettings [apply <master> <music> <effects>]"));
            return;
        }
        const bool bAccepted = Settings->ApplyAudioPreferences(Request, World);
        UE_LOG(LogETUserSettings, Display, TEXT("[M3-05C2] ApplyAccepted=%d"), bAccepted ? 1 : 0);
    }
    const FETAudioPreferences Value = Settings->GetAudioPreferences();
    UGameInstance* GI = World->GetGameInstance();
    UETAudioSubsystem* Audio = GI ? GI->GetSubsystem<UETAudioSubsystem>() : nullptr;
    UE_LOG(LogETUserSettings, Display,
        TEXT("[M3-05C2] Audio Valid=%d Master=%.3f Music=%.3f Effects=%.3f RoutingApplied=%d"),
        Value.IsValid() ? 1 : 0, Value.MasterVolume, Value.MusicVolume, Value.EffectsVolume,
        Audio && Audio->IsRoutingApplied(World) ? 1 : 0);
}
FAutoConsoleCommandWithWorldAndArgs Command(
    TEXT("et.AudioSettings"), TEXT("Audio preferences: query; apply <master> <music> <effects>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Execute), ECVF_Cheat);
} // namespace ETAudioPreferencesConsole
```

### 4.5 共享DefaultGame.ini：增加音频目录Cook规则

文件：`E:/UE_Project/Demo/EmberTrial/Config/DefaultGame.ini`。

在已有ProjectPackagingSettings段原NNEDenoiser行下加一行，不能删除原行或另改CookAll。Config软路径与实际Cook可达性分开处理。

原文：

```ini
+DirectoriesToAlwaysCook=(Path="/NNEDenoiser")
```

替换为：

```ini
+DirectoriesToAlwaysCook=(Path="/NNEDenoiser")
+DirectoriesToAlwaysCook=(Path="/Game/EmberTrial/Audio")
```

### 4.6 共享DefaultGame.ini：在文件末尾追加路由资源路径

文件：`E:/UE_Project/Demo/EmberTrial/Config/DefaultGame.ini`。

原文是当前最后一条StartingRelics；保留它，后面另起新配置段。不要把音频字段写进遗物Subsystem段或个人ini。

原文：

```ini
+StartingRelics=ETRelic:keen_edge
```

替换为：

```ini
+StartingRelics=ETRelic:keen_edge

[/Script/EmberTrial.ETAudioSubsystem]
MasterClass=/Game/EmberTrial/Audio/Classes/SC_ET_Master.SC_ET_Master
MusicClass=/Game/EmberTrial/Audio/Classes/SC_ET_Music.SC_ET_Music
EffectsClass=/Game/EmberTrial/Audio/Classes/SC_ET_Effects.SC_ET_Effects
UserMix=/Game/EmberTrial/Audio/Mixes/SM_ET_User.SM_ET_User
MusicProbe=/Game/EmberTrial/Audio/Probes/SW_ET_MusicProbe.SW_ET_MusicProbe
EffectsProbe=/Game/EmberTrial/Audio/Probes/SW_ET_EffectsProbe.SW_ET_EffectsProbe
```

## 5. 正常构建与一次自动化

### 5.1 双Build / 索引

保持UE关闭，PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor Build成功后刷新CLion既有CMake工程，确认新文件可见、Settings/ETAudioSubsystem.h与Sound/SoundMix.h可导航。预期Core89 / UI38 / Editor3个cpp。再执行：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

若失败发第一条真实error和附近10–20行。此时资源还没有创建；Build只验证代码，不能证明音量已能听见。

### 5.2 专项一次与PIE短查

打开工程，非Play的Output Log执行：

```text
Automation RunTests EmberTrial.M3.Settings.AudioRouting
```

只需一次Success。它用临时原生SoundClass / Mix验证三类树、Master只传播一次、叶子用原始值、非法 / 非单位默认增益与重复静态覆盖拒绝；不启动音频设备、不声称实际听感通过。原C1纯值未改，不再跑旧40项或Cook。

正式地图PIE游戏控制台执行一次`et.AudioSettings apply 0.5 0.4 0.2`应ApplyAccepted0，查询前后偏好不变、RoutingApplied0。本服务仅在原生-game / 独立游戏创建，避免PIE与编辑器共用声音设备时相互污染。P继续与移动转向短查后Stop，不搭蓝图验收入口。

## 6. 创建六个真实声音资源

### 6.1 先生成两份原创WAV

以下是本文内的生成脚本，使用本机已有Python标准库，不下载包。它只在**SourceAssets/Audio/Probes**生成两份WAV；不要先去资源网站找音效。声音是柔和短和弦 / 短铃声，峰值约0.12，均无循环，不是最终配乐。

在PowerShell整段执行一次：

```powershell
Set-Location 'E:\UE_Project\Demo'
@'
from pathlib import Path
import math
import struct
import wave

output = Path(r'E:\UE_Project\Demo\SourceAssets\Audio\Probes')
output.mkdir(parents=True, exist_ok=True)
sample_rate = 44100
def save(name, seconds, sample):
    target = output / name
    if target.exists():
        raise SystemExit(f'Already exists: {target}; keep the existing source file.')
    data = bytearray()
    for frame in range(round(seconds * sample_rate)):
        t = frame / sample_rate
        envelope = min(1.0, t / 0.025, (seconds - t) / 0.08)
        value = 0.12 * max(0.0, envelope) * sample(t)
        data.extend(struct.pack('<h', round(32767 * max(-1.0, min(1.0, value)))))
    with wave.open(str(target), 'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(data)
    print(target)

save('SW_ET_MusicProbe.wav', 3.0,
     lambda t: sum(math.sin(2 * math.pi * hz * t) for hz in (261.63, 311.13, 392.0)) / 3)
save('SW_ET_EffectsProbe.wav', 0.65,
     lambda t: math.sin(2 * math.pi * 740.0 * t) * math.exp(-4.0 * t))
'@ | python -
```

完成后两个文件应各约264644 / 57374字节（PCM 16-bit、单声道、44100Hz；时长3.0 / .65秒）。已生成就不重复执行。源WAV保留，用于复现与资源来源记录，现有.gitattributes已经对.wav启用LFS。

### 6.2 创建三个SoundClass并连接父子关系

在Content Browser建立：

```text
/Game/EmberTrial/Audio/Classes
/Game/EmberTrial/Audio/Mixes
/Game/EmberTrial/Audio/Probes
```

1. 在Classes目录空白右键，搜索**Sound Class**并创建`SC_ET_Master`、`SC_ET_Music`、`SC_ET_Effects`。这是声音资源类型，不是新Blueprint类；菜单可能位于Audio / Sounds → Classes。
2. 双击SC_ET_Master打开Sound Class图。把Content Browser中的两个子类资源拖进这张图，形成三个节点。若拖入操作不可用，在图空白右键找添加已有Sound Class，选择这两个资产；不要再次创建同名副本。
3. Master放左，两子类放右。由Master的**Children输出引脚**分别连到Music、Effects的**左侧Parent输入引脚**。两个子类不能互相串联，也不要连到引擎Master。图应为一根分两支：

```text
SC_ET_Master ──→ SC_ET_Music
             └→ SC_ET_Effects
```

4. 逐个选中节点看Details：三个资源的**Volume=1.0、Pitch=1.0、Passive Sound Mix Modifiers数组为空**。本批不要在资源Volume里填写个人的0.6 / 0.3 / 0.8，也不要添加被动Mix。
5. Master无父；Master的Child Classes恰好Music / Effects两项；两子类无子项。Parent Class可能是只读字段，关系通过图连线建立，不需要手输父路径。
6. Save All。重新打开Master图确认两条线仍在，三个资产都保存。后面若RoutingApplied0，优先检查连线是否真的保存；仅有三个同名文件不等于树已建立。

**6.2 实际操作记录：** 助手通过 UE 5.5 编辑器完成第 2–6 点。SC_ET_Master 的 Child Classes 中依次选择了 SC_ET_Effects 和 SC_ET_Music，编辑器自动建立两条父子连线；三个类的 Volume / Pitch 均为 1，Passive Sound Mix Modifiers 均为空，两个子类的 Child Classes 均为空。已保存三个资产，关闭并重新打开 SC_ET_Master 后，两条连接仍然存在，编辑器显示 All Saved。子项顺序不影响本章路由。

### 6.3 创建专用常驻SoundMix

在Mixes目录空白右键搜索**Sound Mix**，创建`SM_ET_User`并打开，在Details逐项设置：

| 属性 | 值 |
| --- | --- |
| Apply EQ | False |
| Sound Class Effects | 空数组，0项 |
| Initial Delay | 0 |
| Fade In Time | 0 |
| Duration | -1 |
| Fade Out Time | 0 |

保存。这里空数组是刻意设计：运行时由服务添加三条覆盖；不要再在资源里填三条同样的Volume，否则会重复调节。类覆盖自身用0.05秒渐变降低点击声；Mix本身不延迟进入或退出。Duration负数表示持续有效，不能填0或1秒。

**6.3 实际操作记录：** 助手在 UE 5.5 的 Audio → Classes → Sound Class Mix 菜单创建并保存 SM_ET_User。已核对 Apply EQ 关闭、Sound Class Effects 为空、Initial Delay / Fade In / Fade Out 均为 0、Duration 为 -1。

### 6.4 导入两份SoundWave并分流

1. 回Probes目录，点Import，选择SourceAssets/Audio/Probes下两个.wav导入。资产应为**Sound Wave**，名为`SW_ET_MusicProbe` / `SW_ET_EffectsProbe`，不是Sound Cue / MetaSound。
2. 双击MusicProbe，在Details搜索**Sound Class**，Sound Class Object设为`SC_ET_Music`；EffectsProbe同字段设为`SC_ET_Effects`。
3. 两个SoundWave的Volume=1、Pitch=1、Looping=False；不要设置并发限制 / Attenuation /额外调制器。若Details搜索仍是Sound Class过滤，清空搜索后再检查其他项。
4. 分别点编辑器内预览播放，应能听到和弦 / 铃声，停止后保存。这一步只验证资源可解码，不用于判断运行时个人音量；编辑器预览没有本章游戏服务。
5. Save All；Content Browser确认三个Class / 一个Mix / 两个SoundWave共六个，目录名 / 类型与第4.6节配置路径一致。所有已有Widget父类、地图与Controller配置保持。

**6.4 实际操作记录：** 助手导入两份现有 WAV，创建 SW_ET_MusicProbe 和 SW_ET_EffectsProbe，分别绑定 SC_ET_Music、SC_ET_Effects；两者 Volume / Pitch 均为 1，Looping 关闭、单声道 / 44100 Hz。已保存，编辑器显示 All Saved。当次只确认配置；用户随后已完成第7节实际听感 / 路由与重启验收，结果见本章报告。

## 7. 实际声音、自有Mix与切图

### 7.1 第一轮原生窗口

Save All关闭UE。以下脚本就在本文；用新的专用AudioRoutingUser.ini，不覆盖C1证据或共享Config：

```powershell
Set-Location 'E:\UE_Project\Demo'
$routingRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05C2\Runtime'
New-Item -ItemType Directory -Path $routingRuntime -Force | Out-Null
$routingIni = Join-Path $routingRuntime 'AudioRoutingUser.ini'
$routingLog = Join-Path $routingRuntime 'AudioRouting01.log'
$routingArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    '-NoSplash'
    ('-GameUserSettingsINI="{0}"' -f $routingIni)
    ('-abslog="{0}"' -f $routingLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $routingArgs -WindowStyle Normal
```

菜单就绪，游戏控制台`~`执行`et.AudioSettings`。应Valid1、RoutingApplied1；新ini首次默认1 / 1 / 1。若RoutingApplied0，先发日志给我，不继续把后面的ApplyAccepted0当音量效果。

### 7.2 一组音乐 / 音效真实分流

一次执行一行，等上一段结束后再播放下一段（新探针会主动停止上一段，最多一份）。每条probe后直接听即可，不要求复制长日志。

| 操作顺序 | 预期听感 |
| --- | --- |
| `et.AudioSettings apply 1 1 1`，分别`et.AudioProbe music` / `et.AudioProbe effects` | 两种声音均可听见 |
| `et.AudioSettings apply 1 0 1`，再分别播放两种 | 音乐静音、音效仍可听见 |
| `et.AudioSettings apply 1 1 0`，再分别播放两种 | 音乐可听见、音效静音 |
| `et.AudioSettings apply 0 1 1`，再分别播放两种 | 两路都静音，子通道偏好仍1 / 1 |
| `et.AudioSettings apply 0.5 1 1`，分别试听，再回`apply 1 1 1`试听 | 两路均随总音量变小 / 恢复；不要求用耳朵证明精确一半声压 |

在同一设备音量下比较，不需要分贝测量。静音时ProbeAccepted1也正常，表示播放请求创建成功；听感才证明没有漏路。`RoutingApplied1`只证明请求提交给引擎，不代表扬声器实际输出已经测量。

然后执行一次`et.AudioSettings apply -0.1 1 1`，应ApplyAccepted0、仍1 / 1 / 1且试听正常；无需重复C1所有非法样本。本批没有新增自动循环背景音乐。

### 7.3 暂停试听与切图恢复

1. 开始试炼 → P暂停，在游戏控制台明确执行`et.AudioSettings apply 0.6 0.3 0.8`，应Accepted1。分别试听music / effects，应可听见。这两种2D设置试听按UI声音在暂停时播放；不是宣称未来战斗音效也要在暂停时继续。
2. 收起控制台，鼠标或键盘继续，移动转向正常。P → 返回主菜单：若有尚未结束的试听，旧声音随换图清理；菜单无旧音延续或叠加。
3. 主菜单只查询`et.AudioSettings`，应同组0.6 / 0.3 / 0.8、RoutingApplied1；再试听一次effects。服务应有旧World released / 新World ready，不能每次apply不断Push同一个Mix。
4. 主菜单再开始试炼一次，查询与试听music应同组。最后回菜单正常退出。不重复战斗、装卸、检查点旧矩阵；HUD默认与原P短观察即可。

日志由我检查每个World一次持有、释放顺序与新World自动应用；不要求你抢时间截取精确声波结束帧。服务仅管理自己的UserMix，不使用ClearSoundMixModifiers清除整个音频设备。

### 7.4 新进程恢复并试听

第一轮正常退出后，再复制7.1整段脚本，只把日志名改为`AudioRouting02.log`，同一`AudioRoutingUser.ini`保持。新进程主菜单先**只查询**`et.AudioSettings`，应0.6 / 0.3 / 0.8、RoutingApplied1；不先apply。

执行一次`et.AudioProbe music`与`et.AudioProbe effects`确认声音仍按保存偏好播放，正常退出。新进程日志和你的实际听感共同构成本批结论；不把只读取ini当作声音路由成功。

## 8. 已知边界、排错与完成反馈

本批两段是原创技术占位。后续正式音乐 / 动作音效只需分配同一Music / Effects类并按自身场景播放；重要的战斗时机、衰减、混响、并发与响度统一另开发。声音源、所属类与运行混音的音量因子各有职责，不把所有资产Volume改成玩家设置值。

服务启动同步加载四个很小的路由资源一次，探针按需载入，没加Tick / 轮询Timer；应用每次三条类覆盖、Mix只Push一次。切图先释放本World三覆盖 / Mix和旧试听，再用同一偏好初始化新World。无音频设备 / 路径错误 / 拓扑不合法时应用拒绝且不保存新偏好；关闭视频预览保护后也不会补执行旧请求。

资源配置报错优先发AudioRouting01.log路径与Root图 / Mix Details截图；编译错误复制第一条error；听不到时说明哪组命令、音乐还是音效、编辑器预览是否可听。自动化不通过先停止实机步骤；不要为解决音量问题删除其他系统的Mix或把PIE设为共享音频试验场。

```text
M3-05C2 完成
三新 / 两旧Core与DefaultGame.ini；Editor Build / Core89-UI38-Editor3索引 / Game Build：
AudioRouting一次；PIE应用拒绝 / 原P移动短查：
三个Class父子与单位默认 / 常驻空Mix / 两Wave分流；六资产Save All：
两源WAV保留；独立首屏RoutingApplied1：
音乐0只静音乐 / 音效0只静音效 / 总0两路静音 / 总音量变小：
一次非法拒绝不变 / 暂停试听 / 切图释放与自动恢复：
新进程同ini只读0.6-0.3-0.8、实际两路试听 / 两轮正常退出：
Source / Config / 六资产保存；原HUD-P-移动短查；失败或听感问题（无填无）：
完成时分 / 手动代码 / 资产配置 / 阅读 / 检查 / 构建启动等待：
```

## 9. Commit / Push（固定放在最后）

本章源码 / Config、六UAsset / 两WAV已在本地`1a2a81b`。历史实现建议：英文`feat: route audio preferences through project sound classes`；专业中文`feat: 通过项目声音类应用音量偏好并支持切图恢复`。已提交内容不重复Stage，远端状态由你确认。

当前Stage本指南收尾（包括授权编辑器操作记录）、[验收报告](Reports/M3_05C2_Verification.md)、AssetManifest八新行、[C3新指南](M3_05C3_AudioSettingsUI.md)及实际修改的状态文档；准确清单见C3第10.1节。

- 英文：`docs: verify audio routing and prepare shared audio settings UI`
- 专业中文：`docs: 完成声音路由验收并准备共享音量设置界面`

C3源码 / 新WBP尚未实施，不Stage候选或空报告；Artifacts证据 / 个人ini与缓存留本地。Stage后继续修改需重新Stage → Review → Commit / Push由用户操作。
