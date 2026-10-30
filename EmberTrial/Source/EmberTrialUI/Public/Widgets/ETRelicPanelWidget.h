#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "ETRelicPanelWidget.generated.h"

class UButton;
class UEditableTextBox;
class UListView;
class UTextBlock;
class UETRelicPresenter;
struct FETRelicPanelData;

// 页面只发浏览意图，不拥有暂停 / 装备权限；查询状态由自己的Presenter保存。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETRelicPanelWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry,
                                         const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry,
                                const FKeyEvent& InKeyEvent) override;

private:
    // 控件同步受保护，SetText / SetSelectedIndex产生的回调不能再发一次查询。
    bool HasRequiredLayout() const;
    bool IsSearchFocused() const;
    void InitializeQueryControls();
    void SyncQueryControls();
    void ApplyQueryFromControls();
    void Render(const FETRelicPanelData& Data);
    void HandleSelectionChanged(UObject* Item);

    UFUNCTION()
    void HandleSearchChanged(const FText& Text);

    UFUNCTION()
    void HandleQuerySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleSlotA();

    UFUNCTION()
    void HandleSlotB();

    // ---- 页面持有Presenter；以下11个BindWidget对应Designer的名字与类型 ----
    UPROPERTY(Transient)
    TObjectPtr<UETRelicPresenter> Presenter;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> SearchBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> FilterCombo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> SortCombo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UListView> RelicList;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InventoryStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailDescriptionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PreviewText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SlotText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotAButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotBButton;

    FDelegateHandle PresenterHandle;
    FDelegateHandle SelectionHandle;
    bool bSyncingQueryControls = false;
    bool bRendering = false;
};