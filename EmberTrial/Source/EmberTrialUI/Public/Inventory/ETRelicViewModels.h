#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPtr.h"
#include "ETRelicViewModels.generated.h"

class UETRelicInventorySubsystem;
class UTexture2D;
struct FETRelicInventorySnapshot;

// UI自己的查询选项；不把显示用数组下标或文字当作领域槽位ID。
UENUM(BlueprintType)
enum class EETRelicListFilter : uint8
{
    All,
    CompatibleWithSlotA,
    CompatibleWithSlotB,
    Equipped
};

UENUM(BlueprintType)
enum class EETRelicListSort : uint8
{
    CatalogOrder,
    StableId
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETRelicListQuery
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relics")
    EETRelicListFilter Filter = EETRelicListFilter::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relics")
    EETRelicListSort Sort = EETRelicListSort::CatalogOrder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relics")
    FString SearchText;
};

// 列表与详情共用的值快照；没有Actor / ASC / GE句柄，不同步加载图标。
USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETRelicListRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    FName RelicId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    int32 SortOrder = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bSupportsSlotA = false;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bSupportsSlotB = false;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bEquippedInSlotA = false;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bEquippedInSlotB = false;

    bool operator==(const FETRelicListRow& Other) const;
};

enum class EETRelicRowsResult : uint8
{
    Success,
    Unavailable,
    InvalidQuery,
    InvalidSnapshot,
    InvalidCatalog
};

namespace ETRelicPresentation
{
// 只投影调用者给出的库存快照；旧快照不因此获得当前提交权限。
// Success可以是空列表；失败清空输出，不显示上一份成功结果。
EMBERTRIALUI_API EETRelicRowsResult BuildRows(
    const FETRelicInventorySnapshot& InventorySnapshot,
    const UETRelicInventorySubsystem* Catalog,
    const FETRelicListQuery& Query, TArray<FETRelicListRow>& OutRows);
}

DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicListItemChanged, const FETRelicListRow&);

// UListView的Item必须是UObject。此对象承载一行数据，不是被池化的条目Widget。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETRelicListItem : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Relics")
    FETRelicListRow GetSnapshot() const { return Row; }

    // 原生Presenter更新数据；相同数据不通知。空行用于解除数据身份。
    bool AssignRow(const FETRelicListRow& NewRow);
    FETRelicListItemChanged OnChanged;

private:
    UPROPERTY(Transient)
    FETRelicListRow Row;
};