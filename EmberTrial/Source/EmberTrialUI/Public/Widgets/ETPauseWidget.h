#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETPauseWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETPauseActionRequested);

// 暂停View只发继续 / 返回菜单意图；世界暂停、输入模式和生命判断由Flow拥有。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETPauseWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ---- 必需布局、Ready / Pending / Error与初始焦点 ----
    bool HasRequiredLayout() const;
    void ShowReady();
    void ShowAltarReady();
    void ShowPending(const FText& Message);
    void ShowError(const FText& Message);
    void FocusResume();

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Pause")
    FETPauseActionRequested OnResumeIntent;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Pause")
    FETPauseActionRequested OnMainMenuIntent;

protected:
    // 每个实例只在初始化绑定按钮；不在Construct重复注册业务。
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                           const FPointerEvent& InMouseEvent) override;

private:
    // ---- 按钮状态与点击意图；不在本层维护Flow的命令 / 暂停状态 ----
    void ApplyState(bool bPending, const FText& Message);

    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleMainMenuClicked();

    // 控件名 / 类型属于已保存资产契约，保留原名并由Designer装配。
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BTN_Resume;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainMenuButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    // busy只保护本View请求中的输入，不代表世界暂停或角色死亡。
    bool bBusy = false;
};
