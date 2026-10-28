#include "Inventory/ETRelicDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FPrimaryAssetType UETRelicDefinition::AssetType(TEXT("ETRelic"));

// 命名空间有明确文件前缀，避免Unity Build合并时与其他cpp的局部名称冲突。
namespace ETRelicDefinitionPrivate
{
bool IsStableId(const FName Id)
{
    if (Id.IsNone())
    {
        return false;
    }

    const FString Text = Id.ToString();
    for (const TCHAR Character : Text)
    {
        if (!((Character >= TEXT('a') && Character <= TEXT('z')) ||
              (Character >= TEXT('0') && Character <= TEXT('9')) || Character == TEXT('_')))
        {
            return false;
        }
    }
    return true;
}

bool IsKnownSlot(const EETRelicSlot Slot)
{
    return Slot == EETRelicSlot::First || Slot == EETRelicSlot::Second;
}

bool IsKnownKind(const EETRelicModifierKind Kind)
{
    return Kind == EETRelicModifierKind::MaxHealth ||
           Kind == EETRelicModifierKind::LightAttackBonus;
}
}

FPrimaryAssetId UETRelicDefinition::GetPrimaryAssetId() const
{
    return RelicId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(AssetType, RelicId);
}

bool UETRelicDefinition::SupportsSlot(const EETRelicSlot Slot) const
{
    return ETRelicDefinitionPrivate::IsKnownSlot(Slot) && AllowedSlots.Contains(Slot);
}

bool UETRelicDefinition::CollectValidationErrors(TArray<FText>& OutErrors) const
{
    OutErrors.Reset();
    auto AddError = [&](const TCHAR* Code, const TCHAR* Message)
    {
        OutErrors.Add(FText::FromString(FString::Printf(TEXT("[%s] %s: %s"), Code,
                                                       *GetPathName(), Message)));
    };

    // ---- ID / 展示契约；Icon可空，由未来View提供占位 ----
    if (!ETRelicDefinitionPrivate::IsStableId(RelicId))
    {
        AddError(TEXT("ET-RELIC-001"), TEXT("RelicId requires lowercase a-z, 0-9 or underscore."));
    }
    if (DisplayName.IsEmptyOrWhitespace() || Description.IsEmptyOrWhitespace())
    {
        AddError(TEXT("ET-RELIC-002"), TEXT("DisplayName and Description must contain text."));
    }

    // ---- 资格与数值契约；不把重复配置静默去重成另一份合法定义 ----
    TSet<EETRelicSlot> SeenSlots;
    bool bSlotsValid = !AllowedSlots.IsEmpty();
    for (const EETRelicSlot Slot : AllowedSlots)
    {
        bSlotsValid &= ETRelicDefinitionPrivate::IsKnownSlot(Slot) && !SeenSlots.Contains(Slot);
        SeenSlots.Add(Slot);
    }
    if (!bSlotsValid)
    {
        AddError(TEXT("ET-RELIC-003"), TEXT("AllowedSlots must be nonempty, known and unique."));
    }

    TSet<EETRelicModifierKind> SeenKinds;
    bool bModifiersValid = !Modifiers.IsEmpty();
    for (const FETRelicModifier& Modifier : Modifiers)
    {
        bModifiersValid &= ETRelicDefinitionPrivate::IsKnownKind(Modifier.Kind) &&
                           !SeenKinds.Contains(Modifier.Kind) &&
                           FMath::IsFinite(Modifier.AdditiveValue) && Modifier.AdditiveValue != 0.0f;
        SeenKinds.Add(Modifier.Kind);
    }
    if (!bModifiersValid)
    {
        AddError(TEXT("ET-RELIC-004"),
                 TEXT("Modifiers require unique known kinds and finite nonzero additive values."));
    }
    return OutErrors.IsEmpty();
}

bool UETRelicDefinition::CollectCatalogErrors(
    const TArray<const UETRelicDefinition*>& Definitions, TArray<FText>& OutErrors)
{
    OutErrors.Reset();
    TMap<FName, const UETRelicDefinition*> ById;
    for (const UETRelicDefinition* Definition : Definitions)
    {
        if (!IsValid(Definition))
        {
            OutErrors.Add(FText::FromString(TEXT("[ET-RELIC-005] Catalog contains an invalid object.")));
            continue;
        }

        TArray<FText> DefinitionErrors;
        Definition->CollectValidationErrors(DefinitionErrors);
        OutErrors.Append(MoveTemp(DefinitionErrors));
        if (Definition->RelicId.IsNone())
        {
            continue;
        }
        if (const UETRelicDefinition* const* Existing = ById.Find(Definition->RelicId))
        {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("[ET-RELIC-006] Duplicate RelicId '%s': %s / %s"),
                *Definition->RelicId.ToString(), *(*Existing)->GetPathName(), *Definition->GetPathName())));
        }
        else
        {
            ById.Add(Definition->RelicId, Definition);
        }
    }
    return OutErrors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UETRelicDefinition::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    const bool bDefinitionValid = CollectValidationErrors(Errors);
    for (const FText& Error : Errors)
    {
        Context.AddError(Error);
    }
    return bDefinitionValid && ParentResult != EDataValidationResult::Invalid
               ? EDataValidationResult::Valid
               : EDataValidationResult::Invalid;
}
#endif