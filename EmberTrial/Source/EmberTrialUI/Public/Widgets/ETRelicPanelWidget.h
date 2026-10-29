#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETRelicPanelWidget.generated.h"

class UButton;
class UListView;
class UTextBlock;
class UETRelicPresenter;
struct FETRelicPanelData;

// 嵌入现有暂停页的只读浏览页，不拥有世界暂停 / 输入模式，也没有换装确认接口。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETRelicPanelWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    bool HasRequiredLayout() const;
    void Render(const FETRelicPanelData& Data);
    void HandleSelectionChanged(UObject* Item);
    UFUNCTION()
    void HandleSlotA();
    UFUNCTION()
    void HandleSlotB();

    UPROPERTY(Transient)
    TObjectPtr<UETRelicPresenter> Presenter;
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
    bool bRendering = false;
};