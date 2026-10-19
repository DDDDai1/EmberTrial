#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "ETProjectAssetValidator.generated.h"

class UBehaviorTree;
class UWidgetBlueprint;

/** 只读项目BT / UMG结构的Editor校验器；不创建运行世界或修改资产。 */
UCLASS()
class EMBERTRIALEDITOR_API UETProjectAssetValidator : public UEditorValidatorBase
{
    GENERATED_BODY()

public:
    // UE Data Validation入口：先筛选支持范围，再逐资产汇总结果。
    virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject,
                                                 FDataValidationContext& InContext) const override;

    virtual EDataValidationResult ValidateLoadedAsset_Implementation(
        const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

private:
    // 诊断计数由当前校验调用持有，不在Validator实例上缓存。
    void AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Reason, int32& IssueCount);

    // 树遍历与设计树 / 父蓝图查找均限制在资产结构，不执行玩法。
    void ValidateTree(UBehaviorTree* Tree, int32& IssueCount);
    void ValidateWidget(UWidgetBlueprint* Blueprint, int32& IssueCount);
};
