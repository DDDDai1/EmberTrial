# M1-07C2：战斗引用、输入与共享配置校验

更新2026-10-14，UE **5.5.4**。**[C2已验收](Reports/M1_07C2_Verification.md)**：用户14:09、手动20 / 等待5，全项通过，75Source /37cpp /48资产，无待补。本文保留已完成手动步骤，下一步[C3新版包](M1_07C3_UpdatedPackage.md)，不用重复九资产 / 负例 / 自动化。

## 1. 成果、时间与范围

新增独立 `UETGameplayAssetValidator`，继续使用 Asset Actions → Validate Assets。它读取设计时 Class Default Object（CDO），不生成角色、不授予能力、不运行 BeginPlay / 初始化生命值。少量反射集中在两个读取函数，用于检查现有类的私有配置，不增加Runtime公开Getter或编辑器依赖。

预计手动 **45–75分钟**：新代码25–45、配置 / 资产5–10、专项检查10–15、记录5；构建 / 索引等待约3–10分钟另列。需要一份可复用GA副本，不搭Controller验收函数。C1实际40 / 等待5已记录，工具总体投入继续按实耗校准。

| 规则 | 本批检查 |
| --- | --- |
| ET-REF-001 / 002 | 所选Blueprint可编译、原生类型符合本批支持范围 |
| ET-BODY-001 / 002 / 003 | 三个正式身体的ASC / AttributeSet / Trace / Life引用；初始生命值合法；受击能力引用，玩家 / 敌人攻击能力与玩家基础输入引用非空 |
| ET-GA-001 / 002 / 003 | 攻击 / 受击Montage非空、有Skeleton / 正时长 / DefaultSlot；ServerOnly / InstancedPerActor；攻击伤害 / 速度与GE引用合法 |
| ET-GE-001 | 伤害GE为Instant、单Health Add(Base) / SetByCaller.HealthDelta、无Execution，符合现有Runtime守卫 |
| ET-AI-005 / 006 | Controller的Stop / Resume / Melee / Retry关系与Runtime守卫一致，Tree与Controller的Blackboard引用一致 |
| ET-INPUT-001 | IMC_Default恰有一条左键映射，指向IA_ET_LightAttack |
| ET-CONFIG-001–004 | 脚本读取共享Default.ini，检查正式默认地图 / Editor地图 / 两Cook条目、文件存在和Cue扫描路径 |

规则限定当前正式资产及命名为 `GA_ETValidation_*` 的隔离样例；既有Waiting / 非法健康Tests不纳入。本批不证明输入事件Started白线 / Condition、UIBootstrap组件Class装配、Montage通知窗口 / Skeleton与具体Mesh兼容、地图Override / 导航、Runtime订阅及真实加载成功。共享配置脚本不读取平台 / Saved覆写，最终生效行为留C3新版包短流程。

## 2. 文件清单与依赖

Save All，关闭UE；在CLion手动创建两个文件，结尾准确为 **Validator**：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\
├─ Public\Validation\ETGameplayAssetValidator.h          新增
├─ Private\Validation\ETGameplayAssetValidator.cpp       新增
└─ EmberTrialEditor.Build.cs                             修改
```

原 `ETProjectAssetValidator.h / .cpp`、Module.cpp、两个Target与uproject保留。实际Source目前73 / cpp36；两新文件实施后预计75 / 37。后续保存GA Probe才新增第48项来源资产，不提前登记。

在 `EmberTrialEditor.Build.cs` **PrivateDependencyModuleNames** 现有列表末尾加入四项，最终私有列表如下（公开列表保持C1版本）：

```csharp
PrivateDependencyModuleNames.AddRange(new string[]
{
    "AIModule", "UMG", "UMGEditor", "UnrealEd", "EmberTrial",
    "GameplayAbilities", "GameplayTags", "EnhancedInput", "InputCore"
});
```

只改Editor Build.cs；Game / Core / UI不添加编辑器依赖。四项分别提供GAS类型、原生Tag、输入映射结构和EKeys。

## 3. 两个新文件的完整内容

### 3.1 ETGameplayAssetValidator.h

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Public\Validation\ETGameplayAssetValidator.h`。

```cpp
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
```

文件名与generated.h前缀一致；generated.h为最后一个include。不要添加同名UFUNCTION，不需要新增Validator蓝图。

### 3.2 ETGameplayAssetValidator.cpp

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp`。

```cpp
#include "Validation/ETGameplayAssetValidator.h"

#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "AI/ETAIController.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/Blueprint.h"
#include "EnhancedActionKeyMapping.h"
#include "GameplayEffect.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/DataValidation.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGameplayValidation, Log, All);

namespace
{
    UClass* GetGameplayValidationParent(const FString& Package)
    {
        static const TMap<FString, UClass*> Profiles =
        {
            {TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"), AEmberTrialCharacter::StaticClass()},
            {TEXT("/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy"), AEmberTrialCharacter::StaticClass()},
            {TEXT("/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget"), AEmberTrialCharacter::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack"), UETGA_LightAttack::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack"), UETGA_LightAttack::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact"), UETGA_HitReact::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage"), UGameplayEffect::StaticClass()},
            {TEXT("/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController"), AETAIController::StaticClass()}
        };
        if (Package.StartsWith(TEXT("/Game/EmberTrial/Tests/Validation/GA_ETValidation_")))
        {
            return UETGA_LightAttack::StaticClass();
        }
        return Profiles.FindRef(Package);
    }
}

bool UETGameplayAssetValidator::CanValidateAsset_Implementation(
    const FAssetData& InAssetData, UObject* InObject,
    FDataValidationContext& InContext) const
{
    const FString Package = InAssetData.PackageName.ToString();
    return InObject && ((InObject->IsA<UBlueprint>() && GetGameplayValidationParent(Package))
        || (InObject->IsA<UInputMappingContext>()
            && Package == TEXT("/Game/ThirdPerson/Input/IMC_Default")));
}

EDataValidationResult UETGameplayAssetValidator::ValidateLoadedAsset_Implementation(
    const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    int32 Issues = 0;
    if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
    {
        ValidateBlueprint(Blueprint, Issues);
    }
    else if (UInputMappingContext* Input = Cast<UInputMappingContext>(InAsset))
    {
        ValidateInput(Input, Issues);
    }
    else
    {
        return EDataValidationResult::NotValidated;
    }
    if (Issues == 0)
    {
        AssetPasses(InAsset);
    }
    UE_LOG(LogETGameplayValidation, Display,
        TEXT("[M1-07C2] %s Asset=%s Issues=%d"),
        Issues == 0 ? TEXT("Valid") : TEXT("Invalid"), *GetPathNameSafe(InAsset), Issues);
    return Issues == 0 ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

void UETGameplayAssetValidator::AddIssue(UObject* Asset, const TCHAR* Rule,
    const FString& Message, int32& Issues)
{
    ++Issues;
    AssetFails(Asset, FText::FromString(FString::Printf(
        TEXT("[%s] %s : %s"), Rule, *GetPathNameSafe(Asset), *Message)));
}

UObject* UETGameplayAssetValidator::RequireObject(UObject* Asset,
    const UObject* Defaults, const TCHAR* PropertyName, const TCHAR* Rule, int32& Issues)
{
    const FObjectPropertyBase* Property = IsValid(Defaults)
        ? FindFProperty<FObjectPropertyBase>(Defaults->GetClass(), PropertyName) : nullptr;
    if (!Property)
    {
        AddIssue(Asset, TEXT("ET-SETUP-002"), FString::Printf(
            TEXT("Object property %s unavailable; check native schema / validator version."),
            PropertyName), Issues);
        return nullptr;
    }
    UObject* Value = Property->GetObjectPropertyValue_InContainer(Defaults);
    if (!IsValid(Value))
    {
        AddIssue(Asset, Rule, FString::Printf(
            TEXT("Required reference %s is None; restore its Class Defaults value."),
            PropertyName), Issues);
        return nullptr;
    }
    return Value;
}

TOptional<double> UETGameplayAssetValidator::ReadNumber(UObject* Asset,
    const UObject* Defaults, const TCHAR* PropertyName, int32& Issues)
{
    const FNumericProperty* Property = FindFProperty<FNumericProperty>(
        Defaults->GetClass(), PropertyName);
    if (!Property || !Property->IsFloatingPoint())
    {
        AddIssue(Asset, TEXT("ET-SETUP-002"), FString::Printf(
            TEXT("Float property %s unavailable; check native schema / validator version."),
            PropertyName), Issues);
        return {};
    }
    return Property->GetFloatingPointPropertyValue(
        Property->ContainerPtrToValuePtr<void>(Defaults));
}

void UETGameplayAssetValidator::ValidateBlueprint(UBlueprint* Blueprint, int32& Issues)
{
    if (!IsValid(Blueprint->GeneratedClass)
        || Blueprint->Status == BS_Error || Blueprint->Status == BS_Dirty)
    {
        AddIssue(Blueprint, TEXT("ET-REF-001"),
            TEXT("Blueprint not ready; Compile and Save before validation."), Issues);
        return;
    }
    UClass* ExpectedParent = GetGameplayValidationParent(Blueprint->GetOutermost()->GetName());
    if (!IsValid(ExpectedParent) || !Blueprint->GeneratedClass->IsChildOf(ExpectedParent))
    {
        AddIssue(Blueprint, TEXT("ET-REF-002"),
            TEXT("Native parent does not match this asset profile; restore parent / update the profile deliberately."),
            Issues);
        return;
    }
    UObject* Defaults = Blueprint->GeneratedClass->GetDefaultObject();
    if (!IsValid(Defaults))
    {
        AddIssue(Blueprint, TEXT("ET-REF-001"), TEXT("Class default object missing."), Issues);
    }
    else if (Defaults->IsA<AEmberTrialCharacter>())
    {
        ValidateBody(Blueprint, Defaults, Issues);
    }
    else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>())
    {
        ValidateAbility(Blueprint, Defaults, Issues);
    }
    else if (Defaults->IsA<AETAIController>())
    {
        ValidateController(Blueprint, Defaults, Issues);
    }
    else if (const UGameplayEffect* Effect = Cast<UGameplayEffect>(Defaults))
    {
        ValidateDamageEffect(Blueprint, Effect, Issues);
    }
    else
    {
        AddIssue(Blueprint, TEXT("ET-REF-002"),
            TEXT("Unsupported native parent for this formal validation profile; restore parent / update profile deliberately."),
            Issues);
    }
}

void UETGameplayAssetValidator::ValidateBody(UBlueprint* Blueprint,
    UObject* Defaults, int32& Issues)
{
    for (const TCHAR* Name : {TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"),
        TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent")})
    {
        RequireObject(Blueprint, Defaults, Name, TEXT("ET-BODY-001"), Issues);
    }
    const TOptional<double> Health = ReadNumber(Blueprint, Defaults, TEXT("InitialHealth"), Issues);
    const TOptional<double> Max = ReadNumber(Blueprint, Defaults, TEXT("InitialMaxHealth"), Issues);
    if (Health.IsSet() && Max.IsSet() && (!FMath::IsFinite(Health.GetValue())
        || !FMath::IsFinite(Max.GetValue()) || Max.GetValue() < 1.0
        || Health.GetValue() < 0.0 || Health.GetValue() > Max.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-002"),
            TEXT("Require finite InitialMaxHealth>=1 and 0<=InitialHealth<=InitialMaxHealth."), Issues);
    }
    const bool bTraining = Blueprint->GetOutermost()->GetName()
        == TEXT("/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget");
    if (!bTraining)
    {
        RequireObject(Blueprint, Defaults, TEXT("LightAttackAbilityClass"), TEXT("ET-BODY-003"), Issues);
    }
    const bool bPlayer = Blueprint->GetOutermost()->GetName()
        == TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter");
    if (bPlayer)
    {
        UObject* Mapping = RequireObject(Blueprint, Defaults,
            TEXT("DefaultMappingContext"), TEXT("ET-BODY-003"), Issues);
        if (IsValid(Mapping) && Mapping->GetPathName()
            != TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default"))
        {
            AddIssue(Blueprint, TEXT("ET-INPUT-001"),
                TEXT("Player DefaultMappingContext must reference the validated IMC_Default for this profile."), Issues);
        }
        for (const TCHAR* Name : {TEXT("JumpAction"), TEXT("MoveAction"), TEXT("LookAction")})
        {
            RequireObject(Blueprint, Defaults, Name, TEXT("ET-BODY-003"), Issues);
        }
    }
    const FObjectPropertyBase* LifeProperty = FindFProperty<FObjectPropertyBase>(
        Defaults->GetClass(), TEXT("CombatLifeComponent"));
    UObject* Life = LifeProperty ? LifeProperty->GetObjectPropertyValue_InContainer(Defaults) : nullptr;
    if (IsValid(Life))
    {
        RequireObject(Blueprint, Life, TEXT("HitReactAbilityClass"), TEXT("ET-BODY-003"), Issues);
    }
}

void UETGameplayAssetValidator::ValidateMontage(UObject* Asset,
    const UAnimMontage* Montage, int32& Issues)
{
    if (!IsValid(Montage)) { return; } // Missing reference was already reported.
    const bool bDefaultSlot = Montage->SlotAnimTracks.ContainsByPredicate(
        [](const FSlotAnimationTrack& Track) { return Track.SlotName == FName(TEXT("DefaultSlot")); });
    if (!IsValid(Montage->GetSkeleton()) || !FMath::IsFinite(Montage->GetPlayLength())
        || Montage->GetPlayLength() <= 0.0f || !bDefaultSlot)
    {
        AddIssue(Asset, TEXT("ET-GA-001"), FString::Printf(
            TEXT("Montage %s needs a Skeleton, positive length and DefaultSlot."),
            *GetPathNameSafe(Montage)), Issues);
    }
}

void UETGameplayAssetValidator::ValidateAbility(UBlueprint* Blueprint,
    UObject* Defaults, int32& Issues)
{
    const UGameplayAbility* Ability = CastChecked<UGameplayAbility>(Defaults);
    if (Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::InstancedPerActor
        || Ability->GetNetExecutionPolicy() != EGameplayAbilityNetExecutionPolicy::ServerOnly)
    {
        AddIssue(Blueprint, TEXT("ET-GA-002"),
            TEXT("Formal abilities require InstancedPerActor / ServerOnly for current standalone design."), Issues);
    }
    const bool bAttack = Defaults->IsA<UETGA_LightAttack>();
    ValidateMontage(Blueprint, Cast<UAnimMontage>(RequireObject(Blueprint, Defaults,
        bAttack ? TEXT("AttackMontage") : TEXT("HitReactMontage"), TEXT("ET-GA-001"), Issues)), Issues);
    if (!bAttack) { return; }
    const TOptional<double> Damage = ReadNumber(Blueprint, Defaults, TEXT("AttackDamage"), Issues);
    const TOptional<double> Speed = ReadNumber(Blueprint, Defaults, TEXT("AttackMaxHorizontalSpeed"), Issues);
    if (Damage.IsSet() && Speed.IsSet() && (!FMath::IsFinite(Damage.GetValue())
        || Damage.GetValue() <= 0.0 || !FMath::IsFinite(Speed.GetValue()) || Speed.GetValue() < 0.0))
    {
        AddIssue(Blueprint, TEXT("ET-GA-003"),
            TEXT("AttackDamage must be finite >0; AttackMaxHorizontalSpeed finite >=0."), Issues);
    }
    UClass* EffectClass = Cast<UClass>(RequireObject(Blueprint, Defaults,
        TEXT("DamageEffectClass"), TEXT("ET-GA-003"), Issues));
    if (IsValid(EffectClass))
    {
        ValidateDamageEffect(Blueprint, EffectClass->GetDefaultObject<UGameplayEffect>(), Issues);
    }
}

void UETGameplayAssetValidator::ValidateDamageEffect(UObject* Asset,
    const UGameplayEffect* Effect, int32& Issues)
{
    bool bValid = IsValid(Effect) && Effect->DurationPolicy == EGameplayEffectDurationType::Instant
        && Effect->Modifiers.Num() == 1 && Effect->Executions.Num() == 0;
    if (bValid)
    {
        const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
        bValid = Modifier.Attribute == UETHealthAttributeSet::GetHealthAttribute()
            && Modifier.ModifierOp == EGameplayModOp::AddBase
            && Modifier.ModifierMagnitude.GetMagnitudeCalculationType()
                == EGameplayEffectMagnitudeCalculation::SetByCaller
            && Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag
                == ETGameplayTags::SetByCaller_HealthDelta;
    }
    if (!bValid)
    {
        AddIssue(Asset, TEXT("ET-GE-001"),
            TEXT("Damage GE must be Instant, one Health Add(Base) / SetByCaller.HealthDelta modifier, no Execution; restore GE_ET_LightAttackDamage."),
            Issues);
    }
}

void UETGameplayAssetValidator::ValidateController(UBlueprint* Blueprint,
    UObject* Defaults, int32& Issues)
{
    const TOptional<double> Stop = ReadNumber(Blueprint, Defaults, TEXT("PursuitStopDistance"), Issues);
    const TOptional<double> Resume = ReadNumber(Blueprint, Defaults, TEXT("PursuitResumeDistance"), Issues);
    const TOptional<double> Melee = ReadNumber(Blueprint, Defaults, TEXT("MeleeAttackDistance"), Issues);
    const TOptional<double> Retry = ReadNumber(Blueprint, Defaults, TEXT("MeleeRetryDelay"), Issues);
    if (Stop.IsSet() && Resume.IsSet() && Melee.IsSet() && Retry.IsSet()
        && (!FMath::IsFinite(Stop.GetValue()) || !FMath::IsFinite(Resume.GetValue())
            || !FMath::IsFinite(Melee.GetValue()) || !FMath::IsFinite(Retry.GetValue())
            || Stop.GetValue() < 1.0 || Resume.GetValue() <= Stop.GetValue() + 1.0
            || Melee.GetValue() < Resume.GetValue() || Retry.GetValue() < 0.1))
    {
        AddIssue(Blueprint, TEXT("ET-AI-005"),
            TEXT("Require Stop>=1, Resume>Stop+1cm tolerance, Melee>=Resume, Retry>=0.1s; all finite. Current defaults:90/100/100/0.5."),
            Issues);
    }
    const UBehaviorTree* Tree = Cast<UBehaviorTree>(RequireObject(Blueprint, Defaults,
        TEXT("EnemyBehaviorTree"), TEXT("ET-AI-006"), Issues));
    const UBlackboardData* Board = Cast<UBlackboardData>(RequireObject(Blueprint, Defaults,
        TEXT("EnemyBlackboardAsset"), TEXT("ET-AI-006"), Issues));
    if (IsValid(Tree) && IsValid(Board) && Tree->BlackboardAsset.Get() != Board)
    {
        AddIssue(Blueprint, TEXT("ET-AI-006"),
            TEXT("Controller and Behavior Tree must reference the same Blackboard asset."), Issues);
    }
}

void UETGameplayAssetValidator::ValidateInput(UInputMappingContext* Context, int32& Issues)
{
    int32 LeftCount = 0;
    int32 CorrectCount = 0;
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        if (Mapping.Key == EKeys::LeftMouseButton)
        {
            ++LeftCount;
            if (IsValid(Mapping.Action) && Mapping.Action->GetPathName()
                == TEXT("/Game/EmberTrial/Input/Actions/IA_ET_LightAttack.IA_ET_LightAttack"))
            {
                ++CorrectCount;
            }
        }
    }
    if (LeftCount != 1 || CorrectCount != 1)
    {
        AddIssue(Context, TEXT("ET-INPUT-001"),
            TEXT("IMC_Default needs exactly one Left Mouse Button mapping to IA_ET_LightAttack; restore that row and remove duplicates."),
            Issues);
    }
}
```

说明：这个校验器用反射读配置、用类型化SDK检查GE / Montage / 输入。某个私有属性改名后会报ET-SETUP-002，不能静默跳过。规则不是通用资产库契约；未来增加新能力或身体时，显式更新所支持路径与规则，避免把练习 / 故意非法样例扫成正式错误。原生C++已有Runtime守卫仍保留，工具不会代替它们。

## 4. 共享配置与现成脚本

文件：`E:\UE_Project\Demo\EmberTrial\Config\DefaultGame.ini`。搜索有没有以下section；当前C1基线没有。若没有，在文件末尾增加一次；若已存在，在该section下补路径，避免重复section / 同一条目：

```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
+GameplayCueNotifyPaths=/Game/EmberTrial/Abilities
```

这里扫描已有Abilities目录及其子目录；未来本项目GameplayCue资源放入Abilities/Cues，若放到其他目录则明确扩展路径并同步更新脚本规则。当前没有自定义Cue，本批不新增特效。该配置针对已记录的“未指定路径→扫描整个/Game”Warning，**不声称FPS或加载时间已经改善**；Runtime Warning是否消失留C3实际包日志核对。

其余DefaultGame.ini内容和DefaultEngine.ini保持既有值；特别是两正式MapsToCook、默认菜单 / 编辑器正式图不删。保存后PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Validate-ProjectConfig.ps1
```

预期最后一行：`[M1-07C2] Config PASS Errors=0 Warnings=0`。脚本已由助手创建，不需要手抄PowerShell。若在添加Cue配置前执行，预期只有ET-CONFIG-004提示，最后PASS / Warnings=1；**本批完成要求Warnings=0**。

脚本只读项目共享配置 / 文件，不修改INI、不启动UE、不Cook。ET-CONFIG-001对应默认图、002对应Cook条目、003对应umap文件缺失、004对应当前Cue扫描目录。它没有使用UE完整INI层合并或解析umap内容；存在平台 / Saved覆写时以实际Project Settings / C3包为准。助手的脚本负例在Raw隔离副本中测试，用户无需修改正式地图配置制造错误。

## 5. 正常构建与注册

Save All并确认UE关闭，按顺序执行；首个error出现就停下来发文字日志：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor / 索引通过后：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion仍是三个CodeModel，新cpp / h在EmberTrialEditor中。Game这次通常Up to date（没有Runtime C++改动），仍应正常退出。启动UE后原模块ready仍显示M1-07C1，不需要为新校验器修改Module.cpp。第6节出现本批`[M1-07C2]`项目日志，才证明新原生Validator被发现；单纯Editor Build成功不能替代。

## 6. 六个入口的正式配置检查（约3–5分钟）

每个Blueprint先 **Compile → Save**，然后从Content Browser缩略图右键 → **Asset Actions → Validate Assets**。当前正式参数不需要重新设置；按资产原配置直接查。

| 顺序 | Content Browser资源路径 | 预期本项目日志 |
| --- | --- | --- |
| 1 | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` | M1-07C2 Valid / Issues=0 |
| 2 | `/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy` | M1-07C2 Valid / Issues=0 |
| 3 | `/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget` | M1-07C2 Valid / Issues=0 |
| 4 | `/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController` | M1-07C2 Valid / Issues=0 |
| 5 | 左侧目录树的 `/Game/EmberTrial/Abilities` 文件夹右键 → **Validate Assets in Folder** | 三个GA与Effects中的伤害GE各有M1-07C2 Valid；这是递归目录校验 |
| 6 | `/Game/ThirdPerson/Input/IMC_Default` | M1-07C2 Valid / Issues=0 |

第5项三个GA为 `GA_ET_LightAttack`、`GA_ET_EnemyLightAttack`、`GA_ET_HitReact`；GE为 `Effects/GE_ET_LightAttackDamage`。先分别打开三GA与GE Compile → Save，再对目录检查。其他目录资源可能由UE原生Validator处理；仅原生“校验完成”不能代替这四条项目标记。总计**九个正式资产**，不要求逐个重复截图。

如果报错只处理对应规则：ET-BODY参考Class Defaults的引用 / 初始数值，ET-GA参考能力的Montage / 伤害 / 策略，ET-GE参考伤害Modifier，ET-AI参考Controller距离 / Tree / BB，ET-INPUT参考左键行。若ET-BODY-001是HealthAttributes None，先发该资产与规则日志，沿用此前从原生类新建的排障经验，不自动调用运行初始化掩盖CDO问题。

## 7. 隔离GA的缺失引用→恢复（约3–4分钟）

1. 对正式 **GA_ET_LightAttack** 缩略图右键 → Duplicate，命名 **GA_ETValidation_AttackProbe**。将副本拖到已有 `/Game/EmberTrial/Tests/Validation` 文件夹，弹窗选Move Here；路径为 `/Game/EmberTrial/Tests/Validation/GA_ETValidation_AttackProbe`。原Abilities目录若留下该副本Redirector，对该目录执行Fix Up Redirectors in Folder。正式角色的能力Class保持原值。
2. 打开副本，进入 **Class Defaults**；Details搜索 `Attack Montage`，先记住当前值，清空为 **None**。只改这一项，Compile → Save。
3. 回副本缩略图，Validate Assets。预期 `[M1-07C2] Invalid ... Issues=1`，Message Log中 **ET-GA-001** 指向这个隔离副本，提示 `AttackMontage is None`。原生编译通过并不代表资源引用有效，这就是本负例的用途。
4. 回副本Class Defaults，将Attack Montage恢复为刚才原值，Compile → Save → Validate Assets，应为 `[M1-07C2] Valid ... Issues=0`。副本保留**合法配置**，不授予正式角色、不放地图。

只实测ET-GA-001负例；其他规则本批以正常资产 / 代码审查为证，不声称每条错误分支均有独立样例。现有BT Probe仍保留，无需再设计一次断键测试；C1实现本批未改。

## 8. 复用既有自动化（约1–2分钟）

不运行Play，在 **Output Log底部命令输入框** 输入一条（不用重新创建测试C++文件）：

```text
Automation RunTests EmberTrial.M0.PracticeAction
```

这是已有组，预期 **Lifecycle** 与 **DodgeMovement** 两项各一次 `Test Completed. Result={Success}`；可在Output Log搜完整测试路径核对，DodgeMovement含三种步长，是一项测试。测试创建自己的临时世界并清理，结束后编辑器继续正常。它复用已有动作生命周期 / 位移基线，不证明本批GA真实攻击或新工具所有错误规则；相关Gameplay实现未变，旧实机矩阵不重复。

最后Save All、确认GA Probe已恢复，再执行一次配置脚本确认PASS / Warnings=0。等待日志完成后按第9节反馈；不要求本轮Cook / 新Trace，下一批C3统一Attempt02与包内短流程。

## 9. 最简反馈

```text
M1-07C2 完成 / 卡在第__节第__步
两新文件 / Editor私有依赖；Editor Build / 索引 / Game Build：
Cue路径已保存；配置脚本PASS Errors0 / Warnings0：
九个正式资产均有M1-07C2 Valid / Issues0：
GA Probe None→ET-GA-001，恢复→Valid；已保存且未授予正式身体：
已有Lifecycle / DodgeMovement各一次Success：
源码资产Save All，失败或特别说明（无则填无）：
完成时分 / 实际手动用时 / 构建等待：
```

报编译错误发首个error的文字 / 文件行号；配置和资源问题发规则ID / 对象路径，必要时附Class Defaults或Details截图。规则失败后局部处理，不补搭Controller输入测试图。入口说明见[官方Data Validation，UE5.5](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine?application_version=5.5)，具体API签名按本机5.5.4头文件核对；新C++已由用户手动填写 / Editor构建并验收，助手未代写Source /运行Build。

## 10. 本批需要 Commit / Push（固定放在文档最后）

- 新增 `EmberTrial/Source/EmberTrialEditor/Public/Validation/ETGameplayAssetValidator.h` 与 `Private/Validation/ETGameplayAssetValidator.cpp`；修改同模块Build.cs。原C1两个Validator文件、两个Target / uproject只有出现真实新差异才加入。
- 新增 `Tools/Validate-ProjectConfig.ps1`；修改 `EmberTrial/Config/DefaultGame.ini` 的Cue路径。DefaultEngine.ini和MapsToCook原值保留，不人为制造差异。
- 新增**恢复合法配置**的 `EmberTrial/Content/EmberTrial/Tests/Validation/GA_ETValidation_AttackProbe.uasset`（LFS）。正式资产只读 / Compile / Save产生实际差异时核查后提交；不要留下故意None的负例最终版本或无用Redirector。
- 新增本指南、`Docs/Reports/M1_07C1_Verification.md`；C1清单47项 / HUD哈希和状态、路线 / 架构 / 方案B / VersionControl / Introduction的实际更新。C2报告已生成、AssetManifest实际第48项已登记，随最新收尾文字提交。
- Raw、日志 / 测试报告、CMake / Intermediate / Binaries / Saved、Artifacts包 / Trace继续留本地，不Force Add。新的Game exe不是Attempt02，包留C3。

CLion Save All → 刷新Changes → 重新Stage最新实现和恢复后的资产 → Staged Diff → Commit → Push。建议 `feat: validate combat references and project configuration`。C1主体已在本地e3ee554，收尾文档仍需追加提交；用户操作Git，助手不代Stage / Commit / Push。
