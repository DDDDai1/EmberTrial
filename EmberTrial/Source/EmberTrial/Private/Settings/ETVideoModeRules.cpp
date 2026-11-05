#include "Settings/ETVideoModeRules.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogETVideoModes, Log, All);


EETVideoWindowMode FETVideoModeRules::FromNative(const EWindowMode::Type Mode)
{
    switch (Mode)
    {
    case EWindowMode::Windowed: return EETVideoWindowMode::Windowed;
    case EWindowMode::WindowedFullscreen: return EETVideoWindowMode::Borderless;
    case EWindowMode::Fullscreen: return EETVideoWindowMode::Fullscreen;
    default: return EETVideoWindowMode::Invalid;
    }
}

bool FETVideoModeRules::ToNative(const EETVideoWindowMode Mode, EWindowMode::Type& Out)
{
    Out = EWindowMode::Windowed;
    switch (Mode)
    {
    case EETVideoWindowMode::Windowed: Out = EWindowMode::Windowed; return true;
    case EETVideoWindowMode::Borderless: Out = EWindowMode::WindowedFullscreen; return true;
    case EETVideoWindowMode::Fullscreen: Out = EWindowMode::Fullscreen; return true;
    default: return false;
    }
}

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
    Out.Requested.Mode = FromNative(Settings->GetFullscreenMode());
    Out.LastConfirmed.Resolution = Settings->GetLastConfirmedScreenResolution();
    Out.LastConfirmed.Mode = FromNative(Settings->GetLastConfirmedFullscreenMode());
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