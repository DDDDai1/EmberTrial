#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Settings/ETVideoSettingsDraft.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETVideoSettingsWidget.generated.h"

class UButton;
class UTextBlock;
class UETVideoSettingsSubsystem;

/** 主菜单 / Pause共用显示卡；Core拥有真实预览，本View拥有草稿与本次Serial。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETVideoSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void SetHostBusy(bool bBusy);
    void CloseView(); // 宿主移除时立即封口；子控件稍后Destruct可重复调用。

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;

private:
    bool HasLiveHost() const;
    bool CanEdit() const;
    bool Capture(FETVideoMode& Actual, FETVideoResolutionCatalogue& Available) const;
    void ReadCurrent(const FText& Message);
    void Render(bool bRebuildChoices = false);
    void FocusMode();
    void CancelOwnedPreview();
    void HandleSnapshot(const FETVideoPreviewSnapshot& Snapshot);

    UFUNCTION()
    void HandleModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandlePreview();
    UFUNCTION()
    void HandleRevert();
    UFUNCTION()
    void HandleConfirm();
    UFUNCTION()
    void HandleCancel();

    // ---- 八项Designer必需绑定；选项由C++生成，无属性Binding ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ModeCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ResolutionCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> PreviewButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConfirmButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CancelPreviewButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CurrentModeText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> VideoStatusText;

    TWeakObjectPtr<UETVideoSettingsSubsystem> Service;
    FDelegateHandle PreviewChangedHandle;
    FETVideoSettingsDraft Draft;
    TArray<EETVideoWindowMode> Modes;
    TArray<FIntPoint> Resolutions;
    FText Feedback;
    uint64 PreviewSerial = 0;
    bool bViewActive = false;
    bool bHostBusy = false;
    bool bSyncing = false;
    bool bStartingPreview = false;
};