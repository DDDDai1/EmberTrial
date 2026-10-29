#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ETRelicListEntryWidget.generated.h"

class UBorder;
class UTextBlock;
class UETRelicListItem;
struct FETRelicListRow;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETRelicListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;
    virtual void NativeDestruct() override;

private:
    void UnbindItem();
    void RenderRow(const FETRelicListRow& Row);
    void HandleRowChanged(const FETRelicListRow& Row, TWeakObjectPtr<UETRelicListItem> Source);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> SelectionBorder;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EquippedText;
    TWeakObjectPtr<UETRelicListItem> BoundItem;
    FDelegateHandle ItemHandle;
};