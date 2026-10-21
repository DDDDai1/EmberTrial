#include "Validation/ETGameplayAssetValidator.h"

#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGameplayTags.h"
#include "AI/ETAIController.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/EmberTrialCharacter.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Engine/Blueprint.h"
#include "EnhancedActionKeyMapping.h"
#include "GameplayEffect.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGameplayValidation, Log, All);

namespace ETGameplayValidationPaths
{
// Package用于白名单身份；HealActionObject用于对象引用。两种路径不可混用。
constexpr TCHAR PlayerBodyPackage[] = TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter");
constexpr TCHAR TrainingBodyPackage[] =
    TEXT("/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget");
constexpr TCHAR PlayerAttackPackage[] = TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack");
constexpr TCHAR EnemyAttackPackage[] = TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack");
constexpr TCHAR DodgeCostPackage[] = TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost");
constexpr TCHAR HealActionObject[] = TEXT("/Game/EmberTrial/Input/Actions/IA_ET_Heal.IA_ET_Heal");
} // namespace ETGameplayValidationPaths

namespace
{
// 显式白名单是当前正式资产契约，不能扩大为所有Character / GA子类。
UClass* GetGameplayValidationParent(const FString& Package)
{
    static const TMap<FString, UClass*> Profiles = {
        {ETGameplayValidationPaths::PlayerBodyPackage, AEmberTrialCharacter::StaticClass()},
        {TEXT("/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy"),
         AEmberTrialCharacter::StaticClass()},
        {ETGameplayValidationPaths::TrainingBodyPackage, AEmberTrialCharacter::StaticClass()},
        {ETGameplayValidationPaths::PlayerAttackPackage, UETGA_LightAttack::StaticClass()},
        {ETGameplayValidationPaths::EnemyAttackPackage, UETGA_LightAttack::StaticClass()},
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact"), UETGA_HitReact::StaticClass()},
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge"), UETGA_Dodge::StaticClass()},
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal"), UETGA_Heal::StaticClass()},
        {ETGameplayValidationPaths::DodgeCostPackage, UGameplayEffect::StaticClass()},
        {TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage"),
         UGameplayEffect::StaticClass()},
        {TEXT("/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController"),
         AETAIController::StaticClass()}};
    // 只接纳约定命名的隔离GA探针；故意非法的Waiting身体仍不在范围内。
    if (Package.StartsWith(TEXT("/Game/EmberTrial/Tests/Validation/GA_ETValidation_")))
    {
        return UETGA_LightAttack::StaticClass();
    }
    return Profiles.FindRef(Package);
}
} // namespace

// ---- 校验入口：筛选范围、分派与逐资产结果 ----
bool UETGameplayAssetValidator::CanValidateAsset_Implementation(
    const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
    const FString Package = InAssetData.PackageName.ToString();
    return InObject && ((InObject->IsA<UBlueprint>() && GetGameplayValidationParent(Package)) ||
                        (InObject->IsA<UInputMappingContext>() &&
                         Package == TEXT("/Game/ThirdPerson/Input/IMC_Default")));
}

EDataValidationResult UETGameplayAssetValidator::ValidateLoadedAsset_Implementation(
    const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    // 计数只属于本次资产；不支持的类型返回NotValidated，不冒充Valid。
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
    // AddIssue已逐条调用AssetFails；仅无问题时登记AssetPasses。
    if (Issues == 0)
    {
        AssetPasses(InAsset);
    }
    UE_LOG(LogETGameplayValidation, Display, TEXT("[M1-07C2] %s Asset=%s Issues=%d"),
           Issues == 0 ? TEXT("Valid") : TEXT("Invalid"), *GetPathNameSafe(InAsset), Issues);
    return Issues == 0 ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

// ---- 反射读取与诊断：区分原生字段失配和资产漏配 ----
void UETGameplayAssetValidator::AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Message,
                                         int32& Issues)
{
    ++Issues;
    AssetFails(Asset, FText::FromString(FString::Printf(TEXT("[%s] %s : %s"), Rule,
                                                        *GetPathNameSafe(Asset), *Message)));
}

UObject* UETGameplayAssetValidator::RequireObject(UObject* Asset, const UObject* Defaults,
                                                  const TCHAR* PropertyName, const TCHAR* Rule,
                                                  int32& Issues)
{
    // 字段缺失说明工具 / 原生Schema失配；字段存在但None则报调用方的业务规则。
    const FObjectPropertyBase* Property =
        IsValid(Defaults) ? FindFProperty<FObjectPropertyBase>(Defaults->GetClass(), PropertyName)
                          : nullptr;
    if (!Property)
    {
        AddIssue(
            Asset, TEXT("ET-SETUP-002"),
            FString::Printf(
                TEXT("Object property %s unavailable; check native schema / validator version."),
                PropertyName),
            Issues);
        return nullptr;
    }
    UObject* Value = Property->GetObjectPropertyValue_InContainer(Defaults);
    if (!IsValid(Value))
    {
        AddIssue(Asset, Rule,
                 FString::Printf(
                     TEXT("Required reference %s is None; restore its Class Defaults value."),
                     PropertyName),
                 Issues);
        return nullptr;
    }
    return Value;
}

TOptional<double> UETGameplayAssetValidator::ReadNumber(UObject* Asset, const UObject* Defaults,
                                                        const TCHAR* PropertyName, int32& Issues)
{
    // 调用方已验证CDO / 子对象；空Optional保留读取失败，不能当作数值0继续检查。
    const FNumericProperty* Property =
        FindFProperty<FNumericProperty>(Defaults->GetClass(), PropertyName);
    if (!Property || !Property->IsFloatingPoint())
    {
        AddIssue(
            Asset, TEXT("ET-SETUP-002"),
            FString::Printf(
                TEXT("Float property %s unavailable; check native schema / validator version."),
                PropertyName),
            Issues);
        return {};
    }
    return Property->GetFloatingPointPropertyValue(
        Property->ContainerPtrToValuePtr<void>(Defaults));
}

// ---- Blueprint：先验证生成类与原生父类，再只读CDO ----
void UETGameplayAssetValidator::ValidateBlueprint(UBlueprint* Blueprint, int32& Issues)
{
    if (!IsValid(Blueprint->GeneratedClass) || Blueprint->Status == BS_Error ||
        Blueprint->Status == BS_Dirty)
    {
        AddIssue(Blueprint, TEXT("ET-REF-001"),
                 TEXT("Blueprint not ready; Compile and Save before validation."), Issues);
        return;
    }
    UClass* ExpectedParent = GetGameplayValidationParent(Blueprint->GetOutermost()->GetName());
    if (!IsValid(ExpectedParent) || !Blueprint->GeneratedClass->IsChildOf(ExpectedParent))
    {
        AddIssue(Blueprint, TEXT("ET-REF-002"),
                 TEXT("Native parent does not match this asset profile; restore parent / update "
                      "the profile deliberately."),
                 Issues);
        return;
    }
    // 只读取设计时默认对象，不Spawn、初始化资源、授予能力或应用GE。
    UObject* Defaults = Blueprint->GeneratedClass->GetDefaultObject();
    if (!IsValid(Defaults))
    {
        AddIssue(Blueprint, TEXT("ET-REF-001"), TEXT("Class default object missing."), Issues);
    }
    else if (Defaults->IsA<AEmberTrialCharacter>())
    {
        ValidateBody(Blueprint, Defaults, Issues);
    }
    else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() ||
             Defaults->IsA<UETGA_Dodge>() || Defaults->IsA<UETGA_Heal>())
    {
        ValidateAbility(Blueprint, Defaults, Issues);
    }
    else if (Defaults->IsA<AETAIController>())
    {
        ValidateController(Blueprint, Defaults, Issues);
    }
    else if (const UGameplayEffect* Effect = Cast<UGameplayEffect>(Defaults))
    {
        if (Blueprint->GetOutermost()->GetName() == ETGameplayValidationPaths::DodgeCostPackage)
        {
            float Cost = 0.0f;
            if (!UETGA_Dodge::ReadCostAmount(Effect, 1.0f, Cost))
            {
                AddIssue(Blueprint, TEXT("ET-DODGE-001"),
                         TEXT("Cost GE requires Instant, one Stamina AddBase ScalableFloat in "
                              "[-100,-1], no executions."),
                         Issues);
            }
        }
        else
        {
            ValidateDamageEffect(Blueprint, Effect, Issues);
        }
    }
    else
    {
        AddIssue(Blueprint, TEXT("ET-REF-002"),
                 TEXT("Unsupported native parent for this formal validation profile; restore "
                      "parent / update profile deliberately."),
                 Issues);
    }
}

// ---- 身体：共享子对象、初值与玩家 / 训练目标差异 ----
void UETGameplayAssetValidator::ValidateBody(UBlueprint* Blueprint, UObject* Defaults,
                                             int32& Issues)
{
    for (const TCHAR* Name :
         {TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"), TEXT("StaminaAttributes"),TEXT("PoiseAttributes"),
          TEXT("StaminaRecoveryComponent"), TEXT("HealingResourceComponent"),
          TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent")})
    {
        RequireObject(Blueprint, Defaults, Name, TEXT("ET-BODY-001"), Issues);
    }

    const TOptional<double> Health = ReadNumber(Blueprint, Defaults, TEXT("InitialHealth"), Issues);
    const TOptional<double> Max = ReadNumber(Blueprint, Defaults, TEXT("InitialMaxHealth"), Issues);
    if (Health.IsSet() && Max.IsSet() &&
        (!FMath::IsFinite(Health.GetValue()) || !FMath::IsFinite(Max.GetValue()) ||
         Max.GetValue() < 1.0 || Health.GetValue() < 0.0 || Health.GetValue() > Max.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-002"),
                 TEXT("Require finite InitialMaxHealth>=1 and 0<=InitialHealth<=InitialMaxHealth."),
                 Issues);
    }

    const TOptional<double> Stamina =
        ReadNumber(Blueprint, Defaults, TEXT("InitialStamina"), Issues);
    const TOptional<double> MaxStamina =
        ReadNumber(Blueprint, Defaults, TEXT("InitialMaxStamina"), Issues);
    if (Stamina.IsSet() && MaxStamina.IsSet() &&
        (!FMath::IsFinite(Stamina.GetValue()) || !FMath::IsFinite(MaxStamina.GetValue()) ||
         MaxStamina.GetValue() < 1.0 || Stamina.GetValue() < 0.0 ||
         Stamina.GetValue() > MaxStamina.GetValue()))
    {
        AddIssue(
            Blueprint, TEXT("ET-BODY-004"),
            TEXT("Require finite InitialMaxStamina>=1 and 0<=InitialStamina<=InitialMaxStamina."),
            Issues);
    }

    const TOptional<double> Poise = ReadNumber(Blueprint, Defaults, TEXT("InitialPoise"), Issues);
    const TOptional<double> MaxPoise =
        ReadNumber(Blueprint, Defaults, TEXT("InitialMaxPoise"), Issues);
    if (Poise.IsSet() && MaxPoise.IsSet() &&
        (!FMath::IsFinite(Poise.GetValue()) || !FMath::IsFinite(MaxPoise.GetValue()) ||
         MaxPoise.GetValue() < 1.0 || Poise.GetValue() < 0.0 ||
         Poise.GetValue() > MaxPoise.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-005"),
                 TEXT("Require finite InitialMaxPoise>=1 and 0<=InitialPoise<=InitialMaxPoise."),
                 Issues);
    }

    // 训练目标无需攻击；玩家才要求正式输入、闪避和治疗引用。
    const bool bTraining =
        Blueprint->GetOutermost()->GetName() == ETGameplayValidationPaths::TrainingBodyPackage;
    if (!bTraining)
    {
        RequireObject(Blueprint, Defaults, TEXT("LightAttackAbilityClass"), TEXT("ET-BODY-003"),
                      Issues);
    }
    const bool bPlayer =
        Blueprint->GetOutermost()->GetName() == ETGameplayValidationPaths::PlayerBodyPackage;
    if (bPlayer)
    {
        const FObjectPropertyBase* HealingProperty = FindFProperty<FObjectPropertyBase>(
            Defaults->GetClass(), TEXT("HealingResourceComponent"));
        const UETHealingResourceComponent* Healing =
            HealingProperty ? Cast<UETHealingResourceComponent>(
                                  HealingProperty->GetObjectPropertyValue_InContainer(Defaults))
                            : nullptr;
        if (IsValid(Healing) && !Healing->HasValidConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-HEAL-001"),
                     TEXT("Healing requires AutoActivate, MaxCharges 1..9 and InitialCharges "
                          "0..MaxCharges."),
                     Issues);
        }

        RequireObject(Blueprint, Defaults, TEXT("HealAbilityClass"), TEXT("ET-HEAL-002"), Issues);
        const UInputAction* HealInput = Cast<UInputAction>(
            RequireObject(Blueprint, Defaults, TEXT("HealAction"), TEXT("ET-HEAL-004"), Issues));
        if (IsValid(HealInput) &&
            (HealInput->GetPathName() != ETGameplayValidationPaths::HealActionObject ||
             HealInput->ValueType != EInputActionValueType::Boolean ||
             HealInput->bTriggerWhenPaused || HealInput->Triggers.Num() != 0))
        {
            AddIssue(Blueprint, TEXT("ET-HEAL-004"),
                     TEXT("HealAction must be IA_ET_Heal, Boolean, no Triggers, not triggered "
                          "while paused."),
                     Issues);
        }

        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
        UObject* Mapping = RequireObject(Blueprint, Defaults, TEXT("DefaultMappingContext"),
                                         TEXT("ET-BODY-003"), Issues);
        if (IsValid(Mapping) &&
            Mapping->GetPathName() != TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default"))
        {
            AddIssue(Blueprint, TEXT("ET-INPUT-001"),
                     TEXT("Player DefaultMappingContext must reference the validated IMC_Default "
                          "for this profile."),
                     Issues);
        }
        for (const TCHAR* Name : {TEXT("JumpAction"), TEXT("MoveAction"), TEXT("LookAction")})
        {
            RequireObject(Blueprint, Defaults, Name, TEXT("ET-BODY-003"), Issues);
        }
    }

    // 先由必需引用检查报告缺失；以下仅深入有效子对象，避免重复的None诊断。
    const FObjectPropertyBase* LifeProperty =
        FindFProperty<FObjectPropertyBase>(Defaults->GetClass(), TEXT("CombatLifeComponent"));
    UObject* Life =
        LifeProperty ? LifeProperty->GetObjectPropertyValue_InContainer(Defaults) : nullptr;
    if (IsValid(Life))
    {
        RequireObject(Blueprint, Life, TEXT("HitReactAbilityClass"), TEXT("ET-BODY-003"), Issues);
    }

    const FObjectPropertyBase* RecoveryProperty =
        FindFProperty<FObjectPropertyBase>(Defaults->GetClass(), TEXT("StaminaRecoveryComponent"));
    UObject* Recovery =
        RecoveryProperty ? RecoveryProperty->GetObjectPropertyValue_InContainer(Defaults) : nullptr;
    if (IsValid(Recovery))
    {
        const TOptional<double> Rate =
            ReadNumber(Blueprint, Recovery, TEXT("RecoveryRate"), Issues);
        const TOptional<double> Delay =
            ReadNumber(Blueprint, Recovery, TEXT("RecoveryDelay"), Issues);
        const TOptional<double> Interval =
            ReadNumber(Blueprint, Recovery, TEXT("RecoveryInterval"), Issues);
        const FBoolProperty* AutoActivate =
            FindFProperty<FBoolProperty>(Recovery->GetClass(), TEXT("bAutoActivate"));
        if (Rate.IsSet() && Delay.IsSet() && Interval.IsSet() &&
            (!FMath::IsFinite(Rate.GetValue()) || Rate.GetValue() <= 0.0 ||
             Rate.GetValue() > 1000.0 || !FMath::IsFinite(Delay.GetValue()) ||
             Delay.GetValue() < 0.0 || Delay.GetValue() > 10.0 ||
             !FMath::IsFinite(Interval.GetValue()) || Interval.GetValue() < 0.05 ||
             Interval.GetValue() > 0.5 || !AutoActivate ||
             !AutoActivate->GetPropertyValue_InContainer(Recovery)))
        {
            AddIssue(Blueprint, TEXT("ET-STAM-001"),
                     TEXT("Recovery requires AutoActivate, finite 0<Rate<=1000, 0<=Delay<=10 and "
                          "0.05<=Interval<=0.5."),
                     Issues);
        }
    }
}

// ---- 能力：通用动画要求与各原生能力的配置契约 ----
void UETGameplayAssetValidator::ValidateMontage(UObject* Asset, const UAnimMontage* Montage,
                                                int32& Issues)
{
    if (!IsValid(Montage))
    {
        return;
    } // Missing reference was already reported.
    const bool bDefaultSlot = Montage->SlotAnimTracks.ContainsByPredicate(
        [](const FSlotAnimationTrack& Track)
        { return Track.SlotName == FName(TEXT("DefaultSlot")); });
    if (!IsValid(Montage->GetSkeleton()) || !FMath::IsFinite(Montage->GetPlayLength()) ||
        Montage->GetPlayLength() <= 0.0f || !bDefaultSlot)
    {
        AddIssue(
            Asset, TEXT("ET-GA-001"),
            FString::Printf(TEXT("Montage %s needs a Skeleton, positive length and DefaultSlot."),
                            *GetPathNameSafe(Montage)),
            Issues);
    }
}

void UETGameplayAssetValidator::ValidateAbility(UBlueprint* Blueprint, UObject* Defaults,
                                                int32& Issues)
{
    const UGameplayAbility* Ability = CastChecked<UGameplayAbility>(Defaults);
    if (Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::InstancedPerActor ||
        Ability->GetNetExecutionPolicy() != EGameplayAbilityNetExecutionPolicy::ServerOnly)
    {
        AddIssue(Blueprint, TEXT("ET-GA-002"),
                 TEXT("Formal abilities require InstancedPerActor / ServerOnly for current "
                      "standalone design."),
                 Issues);
    }

    // 复用Runtime原生配置检查；返回边界保持各能力独立，避免误套攻击规则。
    if (const UETGA_Heal* Heal = Cast<UETGA_Heal>(Defaults))
    {
        if (!Heal->HasValidConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-HEAL-002"),
                     TEXT("Heal requires valid tags, Amount 1..1000, no Cost/Cooldown GE, one "
                          "in-place DefaultSlot segment, one native branching Heal Commit before "
                          "blend-out, no clip notifies."),
                     Issues);
        }
        return;
    }
    if (const UETGA_HitReact* Hit = Cast<UETGA_HitReact>(Defaults))
    {
        if (!Hit->HasValidHealingCancelPolicy())
        {
            AddIssue(Blueprint, TEXT("ET-HEAL-003"),
                     TEXT("HitReact must cancel Ability.Heal and must not block State.Healing."),
                     Issues);
        }
    }

    if (Defaults->IsA<UETGA_Dodge>())
    {
        ValidateMontage(Blueprint,
                        Cast<UAnimMontage>(RequireObject(Blueprint, Defaults, TEXT("DodgeMontage"),
                                                         TEXT("ET-GA-001"), Issues)),
                        Issues);
        UClass* CostClass = Cast<UClass>(RequireObject(
            Blueprint, Defaults, TEXT("CostGameplayEffectClass"), TEXT("ET-DODGE-001"), Issues));
        float Cost = 0.0f;
        if (CostClass && !UETGA_Dodge::ReadCostAmount(
                             CostClass->GetDefaultObject<UGameplayEffect>(), 1.0f, Cost))
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-001"),
                     TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."),
                     Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidMovementConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-002"),
                     TEXT("Dodge requires finite Distance 1..300, Duration .05...50, and an "
                          "in-place montage longer than Duration."),
                     Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidRecoveryCancelPolicy())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-004"),
                     TEXT("Dodge must cancel only Ability.Attack.Light; do not block "
                          "State.Attacking; keep Dodging/HitReact/Dead/Healing blocked."),
                     Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidInvulnerabilityConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-003"),
                     TEXT("InvulnerabilityDuration must be finite .01..DodgeDuration; native "
                          "duration GE must grant only State.Invulnerable."),
                     Issues);
        }
        return;
    }

    const bool bAttack = Defaults->IsA<UETGA_LightAttack>();

    ValidateMontage(
        Blueprint,
        Cast<UAnimMontage>(RequireObject(Blueprint, Defaults,
                                         bAttack ? TEXT("AttackMontage") : TEXT("HitReactMontage"),
                                         TEXT("ET-GA-001"), Issues)),
        Issues);
    if (!bAttack)
    {
        return;
    }

    const UETGA_LightAttack* Attack = CastChecked<UETGA_LightAttack>(Defaults);
    if (!Attack->HasValidComboConfiguration())
    {
        AddIssue(Blueprint, TEXT("ET-COMBO-001"),
                 TEXT("Combo requires two distinct follow-ups, finite buffer .05...50, same "
                      "skeleton / DefaultSlot / no root motion, one hit state each, one later "
                      "combo state only on stages 1/2 before blend-out."),
                 Issues);
    }
    const FString Package = Blueprint->GetOutermost()->GetName();
    if ((Package == ETGameplayValidationPaths::PlayerAttackPackage && !Attack->IsComboEnabled()) ||
        (Package == ETGameplayValidationPaths::EnemyAttackPackage && Attack->IsComboEnabled()))
    {
        AddIssue(
            Blueprint, TEXT("ET-COMBO-002"),
            TEXT("Formal player requires Combo enabled; formal enemy must keep Combo disabled."),
            Issues);
    }

    const TOptional<double> Damage = ReadNumber(Blueprint, Defaults, TEXT("AttackDamage"), Issues);
    const TOptional<double> Speed =
        ReadNumber(Blueprint, Defaults, TEXT("AttackMaxHorizontalSpeed"), Issues);
    if (Damage.IsSet() && Speed.IsSet() &&
        (!FMath::IsFinite(Damage.GetValue()) || Damage.GetValue() <= 0.0 ||
         !FMath::IsFinite(Speed.GetValue()) || Speed.GetValue() < 0.0))
    {
        AddIssue(Blueprint, TEXT("ET-GA-003"),
                 TEXT("AttackDamage must be finite >0; AttackMaxHorizontalSpeed finite >=0."),
                 Issues);
    }
    UClass* EffectClass = Cast<UClass>(
        RequireObject(Blueprint, Defaults, TEXT("DamageEffectClass"), TEXT("ET-GA-003"), Issues));
    if (IsValid(EffectClass))
    {
        ValidateDamageEffect(Blueprint, EffectClass->GetDefaultObject<UGameplayEffect>(), Issues);
    }
}

// ---- 伤害GE：与闪避固定成本GE使用不同的Modifier契约 ----
void UETGameplayAssetValidator::ValidateDamageEffect(UObject* Asset, const UGameplayEffect* Effect,
                                                     int32& Issues)
{
    // 这里检查伤害的SetByCaller契约；固定负Stamina成本由ReadCostAmount检查。
    bool bValid = IsValid(Effect) &&
                  Effect->DurationPolicy == EGameplayEffectDurationType::Instant &&
                  Effect->Modifiers.Num() == 1 && Effect->Executions.Num() == 0;
    if (bValid)
    {
        const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
        bValid = Modifier.Attribute == UETHealthAttributeSet::GetHealthAttribute() &&
                 Modifier.ModifierOp == EGameplayModOp::AddBase &&
                 Modifier.ModifierMagnitude.GetMagnitudeCalculationType() ==
                     EGameplayEffectMagnitudeCalculation::SetByCaller &&
                 Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
                     ETGameplayTags::SetByCaller_HealthDelta;
    }
    if (!bValid)
    {
        AddIssue(Asset, TEXT("ET-GE-001"),
                 TEXT("Damage GE must be Instant, one Health Add(Base) / SetByCaller.HealthDelta "
                      "modifier, no Execution; restore GE_ET_LightAttackDamage."),
                 Issues);
    }
}

// ---- AI配置：距离关系与Tree / Blackboard一致性 ----
void UETGameplayAssetValidator::ValidateController(UBlueprint* Blueprint, UObject* Defaults,
                                                   int32& Issues)
{
    // 仅验证有限距离、迟滞 / 重试关系和引用，不证明导航可达或实际命中。
    const TOptional<double> Stop =
        ReadNumber(Blueprint, Defaults, TEXT("PursuitStopDistance"), Issues);
    const TOptional<double> Resume =
        ReadNumber(Blueprint, Defaults, TEXT("PursuitResumeDistance"), Issues);
    const TOptional<double> Melee =
        ReadNumber(Blueprint, Defaults, TEXT("MeleeAttackDistance"), Issues);
    const TOptional<double> Retry =
        ReadNumber(Blueprint, Defaults, TEXT("MeleeRetryDelay"), Issues);
    if (Stop.IsSet() && Resume.IsSet() && Melee.IsSet() && Retry.IsSet() &&
        (!FMath::IsFinite(Stop.GetValue()) || !FMath::IsFinite(Resume.GetValue()) ||
         !FMath::IsFinite(Melee.GetValue()) || !FMath::IsFinite(Retry.GetValue()) ||
         Stop.GetValue() < 1.0 || Resume.GetValue() <= Stop.GetValue() + 1.0 ||
         Melee.GetValue() < Resume.GetValue() || Retry.GetValue() < 0.1))
    {
        AddIssue(Blueprint, TEXT("ET-AI-005"),
                 TEXT("Require Stop>=1, Resume>Stop+1cm tolerance, Melee>=Resume, Retry>=0.1s; all "
                      "finite. Current defaults:90/100/100/0.5."),
                 Issues);
    }
    const UBehaviorTree* Tree = Cast<UBehaviorTree>(
        RequireObject(Blueprint, Defaults, TEXT("EnemyBehaviorTree"), TEXT("ET-AI-006"), Issues));
    const UBlackboardData* Board = Cast<UBlackboardData>(RequireObject(
        Blueprint, Defaults, TEXT("EnemyBlackboardAsset"), TEXT("ET-AI-006"), Issues));
    if (IsValid(Tree) && IsValid(Board) && Tree->BlackboardAsset.Get() != Board)
    {
        AddIssue(Blueprint, TEXT("ET-AI-006"),
                 TEXT("Controller and Behavior Tree must reference the same Blackboard asset."),
                 Issues);
    }
}

// ---- 输入资源：按映射行计数，检查唯一键位和目标Action ----
void UETGameplayAssetValidator::ValidateInput(UInputMappingContext* Context, int32& Issues)
{
    // 同时统计键位与Action出现次数，能发现重复行及把治疗Action绑到其他键。
    // 资源结构正确不等于角色蓝图Started执行线已连接。
    int32 LeftCount = 0;
    int32 CorrectCount = 0;
    int32 HealKeyCount = 0;
    int32 HealMappingCount = 0;
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        const bool bHealAction =
            IsValid(Mapping.Action) &&
            Mapping.Action->GetPathName() == ETGameplayValidationPaths::HealActionObject;
        if (Mapping.Key == EKeys::Q)
        {
            ++HealKeyCount;
        }
        if (bHealAction)
        {
            ++HealMappingCount;
        }
        if ((Mapping.Key == EKeys::Q && !bHealAction) || (bHealAction && Mapping.Key != EKeys::Q))
        {
            AddIssue(Context, TEXT("ET-HEAL-004"),
                     TEXT("Only Q may map to IA_ET_Heal; Q must not map to another action."),
                     Issues);
        }
        if (Mapping.Key == EKeys::LeftMouseButton)
        {
            ++LeftCount;
            if (IsValid(Mapping.Action) &&
                Mapping.Action->GetPathName() ==
                    TEXT("/Game/EmberTrial/Input/Actions/IA_ET_LightAttack.IA_ET_LightAttack"))
            {
                ++CorrectCount;
            }
        }
    }
    if (LeftCount != 1 || CorrectCount != 1)
    {
        AddIssue(Context, TEXT("ET-INPUT-001"),
                 TEXT("IMC_Default needs exactly one Left Mouse Button mapping to "
                      "IA_ET_LightAttack; restore that row and remove duplicates."),
                 Issues);
    }
    if (HealKeyCount != 1 || HealMappingCount != 1)
    {
        AddIssue(Context, TEXT("ET-HEAL-004"),
                 TEXT("IMC_Default needs exactly one Q mapping to IA_ET_Heal."), Issues);
    }
}
