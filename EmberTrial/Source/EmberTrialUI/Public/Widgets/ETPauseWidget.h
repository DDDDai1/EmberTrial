#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETPauseWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETPauseActionRequested);

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETPauseWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void ShowReady();
    void ShowPending(const FText& Message);
    void ShowError(const FText& Message);
    void FocusResume();

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Pause")
    FETPauseActionRequested OnResumeIntent;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Pause")
    FETPauseActionRequested OnMainMenuIntent;

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

private:
    void ApplyState(bool bPending, const FText& Message);

    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleMainMenuClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BTN_Resume;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainMenuButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    bool bBusy = false;
};