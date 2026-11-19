#pragma once

#include "Blueprint/UserWidget.h"
#include "ETMenuPagesWidget.generated.h"

class UButton;
class UWidgetSwitcher;

// Both game menus keep their original setting views alive while switching pages.
UCLASS(Abstract, meta=(DisableNativeTick))
class EMBERTRIALUI_API UETMenuPagesWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& DrawList, int32 LayerId, const FWidgetStyle& Style, bool bEnabled) const override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    void SetNavigationBusy(bool bBusy);
    void FocusButton(UButton* Button);
    void SetPrimary(UButton* Button, bool bPrimary);
    bool IsHome() const;
    bool IsSettingsPage() const;
    bool HasNavigationLayout(bool bNeedsRelics) const;
    void FocusCurrentCategory();
    void OpenAltarPage();
    void FocusRelicReturn();
    virtual void LeaveAltar() {}

private:
    UFUNCTION() void OpenSettings();
    UFUNCTION() void OpenRelics();
    UFUNCTION() void BackFromRelics();
    UFUNCTION() void BackFromSettings();
    UFUNCTION() void SelectInput();
    UFUNCTION() void SelectVideo();
    UFUNCTION() void SelectGraphics();
    UFUNCTION() void SelectAudio();
    void SelectCategory(int32 Index);
    void DiscardActiveDraft();
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidgetSwitcher> MenuPages;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidgetSwitcher> SettingsPages;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SettingsButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SettingsBack;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> InputTab;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> VideoTab;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> GraphicsTab;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> AudioTab;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> BrowseRelics;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> RelicsBack;
    bool bNavigationBusy = false;
    bool bAltarPage = false;
};
