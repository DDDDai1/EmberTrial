#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/ETRelicTypes.h"
#include "UObject/PrimaryAssetId.h"
#include "ETRelicDefinition.generated.h"

class UTexture2D;

/** 静态只读定义；不存已拥有 / 已装备、ASC、GE句柄或Widget状态。 */
UCLASS(BlueprintType)
class EMBERTRIAL_API UETRelicDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    static const FPrimaryAssetType AssetType;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    FName GetRelicId() const { return RelicId; }
    const FText& GetDisplayName() const { return DisplayName; }
    const FText& GetDescription() const { return Description; }
    const TSoftObjectPtr<UTexture2D>& GetIcon() const { return Icon; }
    int32 GetSortOrder() const { return SortOrder; }
    const TArray<FETRelicModifier>& GetModifiers() const { return Modifiers; }

    bool SupportsSlot(EETRelicSlot Slot) const;

    // 纯检查，不加载图标、不应用效果；输出由本次调用重新生成。
    bool CollectValidationErrors(TArray<FText>& OutErrors) const;
    static bool CollectCatalogErrors(const TArray<const UETRelicDefinition*>& Definitions,
                                     TArray<FText>& OutErrors);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    friend class FETRelicDefinitionsTest;

    // ---- 稳定身份与本地化展示；更名资产不应更改已经发出的RelicId ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Identity",
              meta = (AllowPrivateAccess = "true"))
    FName RelicId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true"))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true", MultiLine = "true"))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true"))
    int32 SortOrder = 0;

    // ---- 装备资格与效果参数；运行时应用由后续Equipment组件负责 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Equipment",
              meta = (AllowPrivateAccess = "true"))
    TArray<EETRelicSlot> AllowedSlots = {EETRelicSlot::First, EETRelicSlot::Second};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Equipment",
              meta = (AllowPrivateAccess = "true"))
    TArray<FETRelicModifier> Modifiers;
};