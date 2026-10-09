#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETUIBootstrapComponent.generated.h"

class UETLifecycleStatusWidget;
class UETHealthPresenter;
class UETHealthHUDWidget;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETUIBootstrapComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETUIBootstrapComponent();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildStatusWidget();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealthPresenter* GetHealthPresenter() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartHealthObservation();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildHealthWidget();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealthHUDWidget* GetHealthWidget() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RemoveStatusWidget();
    void RemoveHealthWidget();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETLifecycleStatusWidget> StatusWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UETLifecycleStatusWidget> StatusWidget;

    UPROPERTY(Transient)
    TObjectPtr<UETHealthPresenter> HealthPresenter;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETHealthHUDWidget> HealthWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UETHealthHUDWidget> HealthWidget;

    bool bUIShuttingDown = false;
};