#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ETMainMenuController.generated.h"

class UETMainMenuWidget;
class UETRunSessionSubsystem;
class UWorld;

// 菜单世界的唯一View所有者；会话 / 切图规则仍由Core RunSession处理。
UCLASS()
class EMBERTRIALUI_API AETMainMenuController : public APlayerController
{
    GENERATED_BODY()

public:
    AETMainMenuController();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // ---- 当前对象请求门控与输入模式；不缓存LocalPlayer / Widget有效性 ----
    bool CanHandleMenuAction() const;
    void ApplyMenuInputMode();

    // ---- Widget意图与会话失败适配 ----
    UFUNCTION()
    void HandleStartRequested();

    UFUNCTION()
    void HandleContinueRequested();

    UFUNCTION()
    void HandleQuitRequested();

    void HandleSessionFailure(UWorld* FailedWorld, const FText& Message);
    void RecoverAfterFailure(const FText& Message);
    void RemoveMenuWidget();

    // 蓝图装配Class与软地图引用；C++地图默认值只是现有回退，不触发同步加载。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Menu")
    TSubclassOf<UETMainMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Menu")
    TSoftObjectPtr<UWorld> GameplayMap;

    // 强持有本Controller创建的View；移除时先解除自己的按钮订阅。
    UPROPERTY(Transient)
    TObjectPtr<UETMainMenuWidget> MenuWidget;

    // GameInstance拥有Session；本Controller只持有弱引用和自己注册的失败Handle。
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    FDelegateHandle SessionFailureHandle;
    // View的busy只禁按钮；Controller还要拒绝重复 / 结束期间的直接请求。
    bool bTransitionPending = false;
    bool bEndingPlay = false;
};
