#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "ETGameplayAssetValidator.generated.h"

class UBlueprint;
class UInputMappingContext;
class UAnimMontage;
class UGameplayEffect;

/** 只读正式玩法配置的Editor校验器；范围由cpp白名单显式限定。 */
UCLASS()
class EMBERTRIALEDITOR_API UETGameplayAssetValidator : public UEditorValidatorBase
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
    void AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Message, int32& Issues);

    // 原生字段失配与资产引用缺失分开报告；只读已验证的CDO / 子对象。
    UObject* RequireObject(UObject* Asset, const UObject* Defaults, const TCHAR* PropertyName,
                           const TCHAR* Rule, int32& Issues);
    TOptional<double> ReadNumber(UObject* Asset, const UObject* Defaults, const TCHAR* PropertyName,
                                 int32& Issues);

    // 分类规则：共同入口先确认原生父类，各类别保留自身配置边界。
    void ValidateBlueprint(UBlueprint* Blueprint, int32& Issues);
    void ValidateBody(UBlueprint* Blueprint, UObject* Defaults, int32& Issues);
    void ValidateAbility(UBlueprint* Blueprint, UObject* Defaults, int32& Issues);
    void ValidateController(UBlueprint* Blueprint, UObject* Defaults, int32& Issues);

    // 供能力复用的资源结构检查，以及独立的输入映射规则。
    void ValidateMontage(UObject* Asset, const UAnimMontage* Montage, int32& Issues);
    void ValidateDamageEffect(UObject* Asset, const UGameplayEffect* Effect, int32& Issues);
    void ValidateInput(UInputMappingContext* Context, int32& Issues);
};
