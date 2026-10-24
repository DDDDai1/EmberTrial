#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETTargetLockMarkerComponent.generated.h"

class APlayerController;
class AEmberTrialCharacter;
class UETTargetLockComponent;
class UETTargetLockMarkerWidget;
class UUserWidget;
enum class ESlateVisibility : uint8;

// UI表现组件：订阅Core唯一目标，拥有一份View及必要的投影更新。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETTargetLockMarkerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UETTargetLockMarkerComponent();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Target Marker")
    void RebuildMarkerWidget();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Marker")
    TSubclassOf<UETTargetLockMarkerWidget> MarkerWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Marker",
              meta = (ClampMin = "0.0", ClampMax = "200.0", Units = "cm"))
    float HeightOffset = 24.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Marker",
              meta = (ClampMin = "12.0", ClampMax = "200.0"))
    float ScreenInset = 24.0f;
    UPROPERTY(Transient)
    TObjectPtr<UETTargetLockMarkerWidget> MarkerWidget;

    TWeakObjectPtr<APlayerController> BoundController;
    TWeakObjectPtr<UETTargetLockComponent> BoundLock;
    FDelegateHandle TargetChangedHandle;
    FDelegateHandle VisibilityChangedHandle;
    FDelegateHandle DestructHandle;
    bool bViewAttached = false;
    bool bEndingPlay = false;

    bool HasValidSettings() const;
    bool CanRefresh() const;
    void RefreshPresentation();
    void SetRefreshEnabled(bool bEnabled);
    void UpdateMarkerFrame();
    void RemoveMarkerWidget();
    void HandleTargetChanged(AEmberTrialCharacter* Previous, AEmberTrialCharacter* Current);
    void HandleWidgetVisibility(ESlateVisibility Visibility);
    void HandleWidgetDestruct(UUserWidget* Widget);
};