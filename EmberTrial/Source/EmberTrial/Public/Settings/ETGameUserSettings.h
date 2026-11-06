#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Settings/ETAudioSettingsTypes.h"
#include "ETGameUserSettings.generated.h"

struct FETGraphicsPreferences;
class UWorld;

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

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Settings")
    FETAudioPreferences GetAudioPreferences() const { return AudioPreferences; }

    /** 仅更新本实例偏好内存，不混音 / 保存；调用方显式确认后请求SaveSettings。 */
    bool SetAudioPreferences(const FETAudioPreferences& Preferences);

    /** 只提交合法值到运行内存；调用者在整页确认后显式SaveSettings，不逐次滑动写盘。 */
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Settings")
    bool ApplyInputPreferences(const FETInputPreferences& Preferences);

    /** 读取引擎保存的画质组 / 帧率 / VSync；不是GPU实际帧率或视觉达标证明。 */
    bool ReadGraphicsPreferences(FETGraphicsPreferences& Out) const;

    /** 仅独立游戏提交，视频预览保护期间拒绝；成功表示已应用并请求保存。 */
    bool ApplyGraphicsPreferences(const FETGraphicsPreferences& Request,
        const UWorld* ContextWorld);

    virtual void LoadSettings(bool bForceReload = false) override;
    virtual void SetToDefaults() override;
    virtual void ConfirmVideoMode() override;
    virtual void SaveSettings() override;

private:
    friend class UETVideoSettingsSubsystem;
    friend class FETVideoModeRulesTest;

    bool BeginVideoPreviewGuard();
    void RestoreGuardedVideo();
    void EndVideoPreviewGuard();
    void CommitVideoPreview(FIntPoint Resolution, EWindowMode::Type Mode);

    struct FVideoConfigState
    {
        FIntPoint Requested = FIntPoint::ZeroValue;
        FIntPoint Confirmed = FIntPoint::ZeroValue;
        int32 Mode = 0;
        int32 ConfirmedMode = 0;
        int32 PreferredMode = 0;
        float ResolutionQuality = 0.0f;
    };
    FVideoConfigState GuardedVideo;
    bool bVideoPreviewGuard = false; // 非Config，不跨进程保存。

    UPROPERTY(Config)
    FETInputPreferences InputPreferences;

    UPROPERTY(Config)
    FETAudioPreferences AudioPreferences;
};