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