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