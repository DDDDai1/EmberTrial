#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETFailureFlowComponent.generated.h"

class APawn;
class AETPlayerController;
class UWorld;
class UETCombatLifeComponent;
class UETFailureWidget;
class UETPauseWidget;
class UETRunSessionSubsystem;
class UETUILocalPlayerSubsystem;

// Preserve the existing native class and serialized component configuration.
// This single Controller component now coordinates both pause and failure UI.
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETFailureFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETFailureFlowComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // ---- 当前Pawn / View意图的引擎事件适配 ----
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleRetryRequested();

    UFUNCTION()
    void HandleMainMenuRequested();

    UFUNCTION()
    void HandleResumeRequested();

    void HandlePauseRequested();
    void PresentPause();
    void RemovePauseWidget();

    // 身体变化要成组撤销旧观察 / 延迟呈现 / View，再接入新Life。
    void BindPawn(APawn* CurrentPawn);
    void StopObservingLife();
    void HandleDeathSealed(UETCombatLifeComponent* SourceLife);

    // 身份查询只读取当前快照；Ready / Life归属 / 预期死亡状态不能省略。
    bool HasCurrentContext() const;
    bool HasCurrentBody(bool bExpectedDead) const;
    bool HasCurrentDeadBody() const;

    // 普通死亡NextTick；已拥有暂停时立即替换View，避免等不到世界Tick。
    void QueueFailure();
    void PresentFailure();
    void RecoverActivePanel(const FText& Message);
    void HandleSessionFailure(UWorld* FailedWorld, const FText& Message);
    void RemoveFailureWidget();

    // 只恢复自己取得的暂停；EndPlay不调用世界输入 / 焦点恢复。
    void RestoreOwnedInput();
    void CancelPresentation();
    void NotifyMenuState();

    // ---- 蓝图装配契约；不改类名、字段名或默认地图 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Pause")
    TSubclassOf<UETPauseWidget> PauseWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSubclassOf<UETFailureWidget> FailureWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSoftObjectPtr<UWorld> MainMenuMap;

    // 本组件强持有的两种互斥View；移除时先解除自己的意图订阅。
    UPROPERTY(Transient)
    TObjectPtr<UETPauseWidget> PauseWidget;

    UPROPERTY(Transient)
    TObjectPtr<UETFailureWidget> FailureWidget;

    // 外部对象由各自生命周期拥有；每个Handle只撤销本组件的注册。
    TWeakObjectPtr<AETPlayerController> ControllerOwner;
    TWeakObjectPtr<UETUILocalPlayerSubsystem> UIContext;
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    TWeakObjectPtr<UETCombatLifeComponent> BoundLife;
    FDelegateHandle PauseRequestHandle;
    FDelegateHandle DeathHandle;
    FDelegateHandle SessionFailureHandle;
    FTimerHandle PresentationHandle;

    // 延迟呈现、切图命令、暂停所有权分别建模，不能合成一个busy。
    bool bPresentationQueued = false;
    bool bCommandPending = false;
    // 外部已暂停时不擅自取得所有权；失败替换自己的Pause时保留所有权。
    bool bOwnsPause = false;
    // 先封口再解绑，阻止结束过程中的迟到意图 / 回调。
    bool bEndingPlay = false;
};
