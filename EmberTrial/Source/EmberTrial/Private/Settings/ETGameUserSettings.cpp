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