#include "Settings/ETGameUserSettings.h"
#include "Settings/ETGraphicsSettingsTypes.h"
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
{
    Super::LoadSettings(bForceReload);
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
}

void UETGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    InputPreferences = FETInputPreferences{};
    AudioPreferences = FETAudioPreferences{};
}

// ---- 预览配置保护：UE窗口变化会自动SetScreenResolution + ConfirmVideoMode ----

bool UETGameUserSettings::BeginVideoPreviewGuard()
{
    if (bVideoPreviewGuard)
    {
        return false;
    }
    GuardedVideo.Requested = GetScreenResolution();
    GuardedVideo.Confirmed = GetLastConfirmedScreenResolution();
    GuardedVideo.Mode = FullscreenMode;
    GuardedVideo.ConfirmedMode = LastConfirmedFullscreenMode;
    GuardedVideo.PreferredMode = PreferredFullscreenMode;
    GuardedVideo.ResolutionQuality = ScalabilityQuality.ResolutionQuality;
    bVideoPreviewGuard = true;
    return true;
}

void UETGameUserSettings::RestoreGuardedVideo()
{
    if (bVideoPreviewGuard)
    {
        ResolutionSizeX = GuardedVideo.Requested.X;
        ResolutionSizeY = GuardedVideo.Requested.Y;
        LastUserConfirmedResolutionSizeX = GuardedVideo.Confirmed.X;
        LastUserConfirmedResolutionSizeY = GuardedVideo.Confirmed.Y;
        FullscreenMode = GuardedVideo.Mode;
        LastConfirmedFullscreenMode = GuardedVideo.ConfirmedMode;
        PreferredFullscreenMode = GuardedVideo.PreferredMode;
        ScalabilityQuality.ResolutionQuality = GuardedVideo.ResolutionQuality;
    }
}

void UETGameUserSettings::EndVideoPreviewGuard()
{
    RestoreGuardedVideo();
    bVideoPreviewGuard = false;
}

void UETGameUserSettings::ConfirmVideoMode()
{
    if (bVideoPreviewGuard)
    {
        RestoreGuardedVideo();
        return;
    }
    Super::ConfirmVideoMode();
}

void UETGameUserSettings::SaveSettings()
{
    RestoreGuardedVideo();
    Super::SaveSettings(); // 输入等非视频偏好照常保存，候选视频值不入盘。
}

void UETGameUserSettings::CommitVideoPreview(
    const FIntPoint Resolution, const EWindowMode::Type Mode)
{
    EndVideoPreviewGuard();
    SetScreenResolution(Resolution);
    SetFullscreenMode(Mode);
    if (Mode != EWindowMode::Windowed)
    {
        // UE5.5的SetPreferredFullscreenMode是private；在派生类同步受保护字段与同名CVar。
        PreferredFullscreenMode = Mode == EWindowMode::Fullscreen ? 0 : 1;
        if (IConsoleVariable* Fullscreen = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode")))
        {
            Fullscreen->Set(PreferredFullscreenMode, ECVF_SetByGameSetting);
        }
    }
    Super::ConfirmVideoMode();
    Super::SaveSettings();
}

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