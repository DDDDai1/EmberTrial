#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "ETProjectAssetValidator.generated.h"

class UBehaviorTree;
class UWidgetBlueprint;

UCLASS()
class EMBERTRIALEDITOR_API UETProjectAssetValidator : public UEditorValidatorBase
{
    GENERATED_BODY()

public:
    virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData,
        UObject* InObject, FDataValidationContext& InContext) const override;

    virtual EDataValidationResult ValidateLoadedAsset_Implementation(
        const FAssetData& InAssetData, UObject* InAsset,
        FDataValidationContext& Context) override;

private:
    void AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Reason,
        int32& IssueCount);
    void ValidateTree(UBehaviorTree* Tree, int32& IssueCount);
    void ValidateWidget(UWidgetBlueprint* Blueprint, int32& IssueCount);
};