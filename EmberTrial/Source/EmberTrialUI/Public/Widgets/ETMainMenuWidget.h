#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETMenuActionRequested);

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void ShowReady();
    void ShowPending(const FText& Message);
    void ShowError(const FText& Message);
    void FocusStartButton();

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Menu")
    FETMenuActionRequested OnStartRequested;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Menu")
    FETMenuActionRequested OnQuitRequested;

protected:
    virtual void NativeOnInitialized() override;

private:
    void ApplyState(bool bPending, const FText& Message);

    UFUNCTION()
    void HandleStartClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> StartButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ContinueButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    bool bBusy = false;
};