#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETFailureWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETFailureActionRequested);

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETFailureWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void ShowReady();
    void ShowPending(const FText& Message);
    void ShowError(const FText& Message);
    void FocusRetryButton();

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Failure")
    FETFailureActionRequested OnRetryRequested;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Failure")
    FETFailureActionRequested OnMainMenuRequested;

protected:
    virtual void NativeOnInitialized() override;

private:
    void ApplyState(bool bPending, const FText& Message);

    UFUNCTION()
    void HandleRetryClicked();

    UFUNCTION()
    void HandleMainMenuClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RetryButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainMenuButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    bool bBusy = false;
};