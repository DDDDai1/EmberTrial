#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "ETGameplayAssetValidator.generated.h"

class UBlueprint;
class UInputMappingContext;
class UAnimMontage;
class UGameplayEffect;

UCLASS()
class EMBERTRIALEDITOR_API UETGameplayAssetValidator : public UEditorValidatorBase
{
    GENERATED_BODY()

public:
    virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData,
        UObject* InObject, FDataValidationContext& InContext) const override;
    virtual EDataValidationResult ValidateLoadedAsset_Implementation(
        const FAssetData& InAssetData, UObject* InAsset,
        FDataValidationContext& Context) override;

private:
    void AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Message, int32& Issues);
    UObject* RequireObject(UObject* Asset, const UObject* Defaults,
        const TCHAR* PropertyName, const TCHAR* Rule, int32& Issues);
    TOptional<double> ReadNumber(UObject* Asset, const UObject* Defaults,
        const TCHAR* PropertyName, int32& Issues);
    void ValidateBlueprint(UBlueprint* Blueprint, int32& Issues);
    void ValidateBody(UBlueprint* Blueprint, UObject* Defaults, int32& Issues);
    void ValidateAbility(UBlueprint* Blueprint, UObject* Defaults, int32& Issues);
    void ValidateController(UBlueprint* Blueprint, UObject* Defaults, int32& Issues);
    void ValidateMontage(UObject* Asset, const UAnimMontage* Montage, int32& Issues);
    void ValidateDamageEffect(UObject* Asset, const UGameplayEffect* Effect, int32& Issues);
    void ValidateInput(UInputMappingContext* Context, int32& Issues);
};