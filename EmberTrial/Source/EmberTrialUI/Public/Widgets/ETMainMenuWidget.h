#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETMenuActionRequested);

// 只展示状态并发出意图；不保存检查点、不查ASC、不执行切图。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ---- 布局契约与状态呈现；pending不改写已确认的Continue可用性 ----
    bool HasRequiredLayout() const;
    void ShowReady(bool bCanContinue);
    void ShowPending(const FText& Message);
    void ShowError(const FText& Message, bool bCanContinue);
    void FocusStartButton();

    // Controller订阅这些意图；Widget的busy与Controller门控各自负责本层边界。
    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Menu")
    FETMenuActionRequested OnStartRequested;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Menu")
    FETMenuActionRequested OnContinueRequested;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Menu")
    FETMenuActionRequested OnQuitRequested;

protected:
    // 每个实例初始化时绑定一次；移出 / 加回屏幕不重复构造按钮业务。
    virtual void NativeOnInitialized() override;

private:
    // ---- 统一按钮状态；点击回调只检查View状态并广播 ----
    void ApplyState(bool bPending, const FText& Message);

    UFUNCTION()
    void HandleStartClicked();

    UFUNCTION()
    void HandleContinueClicked();

    UFUNCTION()
    void HandleQuitClicked();

    // Designer名称 / 类型是资产契约，保留BindWidget字段，不另建蓝图点击链。
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> StartButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ContinueButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    // busy仅表示请求处理中，不等于世界暂停；Continue来自Session快照。
    bool bBusy = false;
    bool bContinueAvailable = false;
};
