#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETUIBootstrapComponent.generated.h"

class APlayerController;
class UETLifecycleStatusWidget;
class UETHealthPresenter;
class UETHealthHUDWidget;
class UETStaminaPresenter;
class UETStaminaHUDWidget;
class UETHealingPresenter;
class UETHealingHUDWidget;
class UETBreakPulsePresenter;
class UETBreakPulseHUDWidget;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETUIBootstrapComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETUIBootstrapComponent();

    // 重建只替换View，不重建Presenter，也不初始化玩法属性。
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildStatusWidget();

    // ---- 生命：Presenter持有数据订阅，Widget持有展示订阅 ----
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealthPresenter* GetHealthPresenter() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartHealthObservation();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildHealthWidget();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealthHUDWidget* GetHealthWidget() const;

    // ---- 体力：独立对象与配置，共用相同的本地入口规则 ----
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETStaminaPresenter* GetStaminaPresenter() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartStaminaObservation();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildStaminaWidget();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETStaminaHUDWidget* GetStaminaWidget() const;

    // ---- 治疗次数：只读离散资源，不改变次数或生命 ----
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealingPresenter* GetHealingPresenter() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartHealingObservation();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildHealingWidget();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealingHUDWidget* GetHealingWidget() const;

    // ---- 破势冷却HUD：View重建与数据观察分离 ----
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETBreakPulsePresenter* GetBreakPulsePresenter() const;
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartBreakPulseObservation();
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildBreakPulseWidget();
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETBreakPulseHUDWidget* GetBreakPulseWidget() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // 每次查询当前Owner；不缓存Controller / LocalPlayer，也不参与输入模式切换。
    APlayerController* ResolveLocalUIController() const;

    void RemoveStatusWidget();
    void RemoveHealthWidget();
    void RemoveStaminaWidget();
    void RemoveHealingWidget();
    void RemoveBreakPulseWidget();

    // ---- 蓝图配置与运行期所有权；Transient对象不写回资产 ----
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

    UPROPERTY(Transient)
    TObjectPtr<UETStaminaPresenter> StaminaPresenter;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETStaminaHUDWidget> StaminaWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UETStaminaHUDWidget> StaminaWidget;

    UPROPERTY(Transient)
    TObjectPtr<UETHealingPresenter> HealingPresenter;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETHealingHUDWidget> HealingWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UETHealingHUDWidget> HealingWidget;

    UPROPERTY(Transient)
    TObjectPtr<UETBreakPulsePresenter> BreakPulsePresenter;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETBreakPulseHUDWidget> BreakPulseWidgetClass;
    UPROPERTY(Transient)
    TObjectPtr<UETBreakPulseHUDWidget> BreakPulseWidget;

    // EndPlay先封闭入口，再移除View，最后停止Presenter。
    bool bUIShuttingDown = false;
};
