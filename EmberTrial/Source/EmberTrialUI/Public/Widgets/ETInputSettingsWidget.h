#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/ETInputSettingsDraft.h"
#include "ETInputSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;

/** 主菜单 / Pause共用输入设置卡；不拥有世界暂停或输入模式。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETInputSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void SetHostBusy(bool bBusy);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    bool CanEdit() const;
    bool HasLiveHost() const;
    void ReadCurrent(const FText& Message);
    void Render();
    void FocusSlider();

    UFUNCTION()
    void HandleSensitivityChanged(float Value);

    UFUNCTION()
    void HandleInvertChanged(bool bChecked);

    UFUNCTION()
    void HandleApply();

    UFUNCTION()
    void HandleRevert();

    UFUNCTION()
    void HandleDefaults();

    // ---- 以下七个名称 / 类型是Designer装配契约 ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> SensitivitySlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SensitivityValueText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> InvertYCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DefaultsButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SettingsStatusText;

    TWeakObjectPtr<UETGameUserSettings> Settings;
    FETInputSettingsDraft Draft;
    FText Feedback;
    bool bViewActive = false;
    bool bSyncingControls = false;
    bool bHostBusy = false;
};