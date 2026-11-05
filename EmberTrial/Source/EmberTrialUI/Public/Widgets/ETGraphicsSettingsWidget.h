#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Settings/ETGraphicsSettingsDraft.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETGraphicsSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UTextBlock;
class UETGameUserSettings;
class UETVideoSettingsSubsystem;

/** 共用画质卡；View拥有草稿，原生GameUserSettings负责应用 / 保存。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETGraphicsSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void SetHostBusy(bool bBusy);
    void CloseView();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    bool HasLiveHost() const;
    bool IsPreviewBusy() const;
    bool CanEdit() const;
    void ReadCurrent(const FText& Message);
    void Render(bool bRebuildChoices = false);
    void FocusQuality();
    void HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot);

    UFUNCTION()
    void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleFrameRateChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleVSyncChanged(bool bChecked);
    UFUNCTION()
    void HandleApply();
    UFUNCTION()
    void HandleRevert();
    UFUNCTION()
    void HandleDefaults();

    // ---- 八项必需绑定；控件程序同步不回写，不使用属性Binding ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> QualityCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> FrameRateCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> VSyncCheckBox;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DefaultsButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GraphicsCurrentText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GraphicsStatusText;

    TWeakObjectPtr<UETGameUserSettings> Settings;
    TWeakObjectPtr<UETVideoSettingsSubsystem> PreviewService;
    FDelegateHandle PreviewChangedHandle;
    FETGraphicsSettingsDraft Draft;
    TArray<int32> QualityLevels;
    TArray<float> FrameRateLimits;
    FText Feedback;
    bool bViewActive = false;
    bool bHostBusy = false;
    bool bSyncingControls = false;
};