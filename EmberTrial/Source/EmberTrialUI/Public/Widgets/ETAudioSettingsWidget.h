#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/ETAudioSettingsDraft.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETAudioSettingsWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;
class UETGameUserSettings;
class UETAudioSubsystem;
class UETVideoSettingsSubsystem;

/** 共用音量卡；草稿归页面，实际路由与保存沿用Core唯一入口。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETAudioSettingsWidget : public UUserWidget
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
    void Render();
    void FocusMaster();
    void UpdateDraft(const FETAudioPreferences& Request);
    void HandleRoutingChanged();
    void HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot);

    UFUNCTION()
    void HandleMasterChanged(float Value);
    UFUNCTION()
    void HandleMusicChanged(float Value);
    UFUNCTION()
    void HandleEffectsChanged(float Value);
    UFUNCTION()
    void HandleApply();
    UFUNCTION()
    void HandleRevert();
    UFUNCTION()
    void HandleDefaults();

    // ---- 十一项绑定；没有属性Binding或蓝图点击链 ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> MasterSlider;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> MusicSlider;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> EffectsSlider;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MasterValueText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MusicValueText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EffectsValueText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DefaultsButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> AudioCurrentText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> AudioStatusText;

    TWeakObjectPtr<UETGameUserSettings> Settings;
    TWeakObjectPtr<UETAudioSubsystem> AudioService;
    TWeakObjectPtr<UETVideoSettingsSubsystem> PreviewService;
    FDelegateHandle RoutingChangedHandle;
    FDelegateHandle PreviewChangedHandle;
    FETAudioSettingsDraft Draft;
    FText Feedback;
    bool bViewActive = false;
    bool bHostBusy = false;
    bool bSyncingControls = false;
};