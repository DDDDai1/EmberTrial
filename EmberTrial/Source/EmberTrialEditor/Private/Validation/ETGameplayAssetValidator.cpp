#include "Validation/ETGameplayAssetValidator.h"

#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_Dodge.h"
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
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge"), UETGA_Dodge::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost"), UGameplayEffect::StaticClass()},
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
    else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() || Defaults->IsA<UETGA_Dodge>())
    {
        ValidateAbility(Blueprint, Defaults, Issues);
    }
    else if (Defaults->IsA<AETAIController>())
    {
        ValidateController(Blueprint, Defaults, Issues);
    }
    else if (const UGameplayEffect* Effect = Cast<UGameplayEffect>(Defaults))
    {
        if (Blueprint->GetOutermost()->GetName() == TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost"))
        {
            float Cost = 0.0f;
            if (!UETGA_Dodge::ReadCostAmount(Effect, 1.0f, Cost))
            {
                AddIssue(Blueprint, TEXT("ET-DODGE-001"),
                    TEXT("Cost GE requires Instant, one Stamina AddBase ScalableFloat in [-100,-1], no executions."), Issues);
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
            TEXT("Unsupported native parent for this formal validation profile; restore parent / update profile deliberately."),
            Issues);
    }
}

void UETGameplayAssetValidator::ValidateBody(UBlueprint* Blueprint,
    UObject* Defaults, int32& Issues)
{
    for (const TCHAR* Name : {TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"),
        TEXT("StaminaAttributes"), TEXT("StaminaRecoveryComponent"),
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
    const TOptional<double> Stamina = ReadNumber(Blueprint, Defaults, TEXT("InitialStamina"), Issues);
    const TOptional<double> MaxStamina = ReadNumber(Blueprint, Defaults, TEXT("InitialMaxStamina"), Issues);
    if (Stamina.IsSet() && MaxStamina.IsSet() && (!FMath::IsFinite(Stamina.GetValue())
        || !FMath::IsFinite(MaxStamina.GetValue()) || MaxStamina.GetValue() < 1.0
        || Stamina.GetValue() < 0.0 || Stamina.GetValue() > MaxStamina.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-004"),
            TEXT("Require finite InitialMaxStamina>=1 and 0<=InitialStamina<=InitialMaxStamina."), Issues);
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
        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
        UObject* Mapping = RequireObject(Blueprint, Defaults,
            TEXT("DefaultMappingContext"), TEXT("ET-BODY-003"), Issues);
        if (IsValid(Mapping) && Mapping->GetPathName()
            != TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default"))
        {
            AddIssue(Blueprint, TEXT("ET-INPUT-001"),
                TEXT("Player DefaultMappingContext must reference the validated IMC_Default for this profile."),
                Issues);
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

    const FObjectPropertyBase* RecoveryProperty = FindFProperty<FObjectPropertyBase>(
        Defaults->GetClass(), TEXT("StaminaRecoveryComponent"));
    UObject* Recovery = RecoveryProperty ? RecoveryProperty->GetObjectPropertyValue_InContainer(Defaults) : nullptr;
    if (IsValid(Recovery))
    {
        const TOptional<double> Rate = ReadNumber(Blueprint, Recovery, TEXT("RecoveryRate"), Issues);
        const TOptional<double> Delay = ReadNumber(Blueprint, Recovery, TEXT("RecoveryDelay"), Issues);
        const TOptional<double> Interval = ReadNumber(Blueprint, Recovery, TEXT("RecoveryInterval"), Issues);
        const FBoolProperty* AutoActivate = FindFProperty<FBoolProperty>(
            Recovery->GetClass(), TEXT("bAutoActivate"));
        if (Rate.IsSet() && Delay.IsSet() && Interval.IsSet() &&
            (!FMath::IsFinite(Rate.GetValue()) || Rate.GetValue() <= 0.0 || Rate.GetValue() > 1000.0 ||
             !FMath::IsFinite(Delay.GetValue()) || Delay.GetValue() < 0.0 || Delay.GetValue() > 10.0 ||
             !FMath::IsFinite(Interval.GetValue()) || Interval.GetValue() < 0.05 || Interval.GetValue() > 0.5 ||
             !AutoActivate || !AutoActivate->GetPropertyValue_InContainer(Recovery)))
        {
            AddIssue(Blueprint, TEXT("ET-STAM-001"),
                TEXT("Recovery requires AutoActivate, finite 0<Rate<=1000, 0<=Delay<=10 and 0.05<=Interval<=0.5."), Issues);
        }
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
    if (Defaults->IsA<UETGA_Dodge>())
    {
        ValidateMontage(Blueprint, Cast<UAnimMontage>(RequireObject(Blueprint, Defaults,
            TEXT("DodgeMontage"), TEXT("ET-GA-001"), Issues)), Issues);
        UClass* CostClass = Cast<UClass>(RequireObject(Blueprint, Defaults,
            TEXT("CostGameplayEffectClass"), TEXT("ET-DODGE-001"), Issues));
        float Cost = 0.0f;
        if (CostClass && !UETGA_Dodge::ReadCostAmount(CostClass->GetDefaultObject<UGameplayEffect>(), 1.0f, Cost))
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-001"), TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."), Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidMovementConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-002"),
                TEXT("Dodge requires finite Distance 1..300, Duration .05...50, and an in-place montage longer than Duration."), Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidInvulnerabilityConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-003"),
                TEXT("InvulnerabilityDuration must be finite .01..DodgeDuration; native duration GE must grant only State.Invulnerable."), Issues);
        }
        return;
    }

    const bool bAttack = Defaults->IsA<UETGA_LightAttack>();

    ValidateMontage(Blueprint, Cast<UAnimMontage>(RequireObject(Blueprint, Defaults,
        bAttack ? TEXT("AttackMontage") : TEXT("HitReactMontage"), TEXT("ET-GA-001"), Issues)), Issues);
    if (!bAttack) { return; }

    const UETGA_LightAttack* Attack=CastChecked<UETGA_LightAttack>(Defaults);
    if (!Attack->HasValidComboConfiguration())
    {
        AddIssue(Blueprint,TEXT("ET-COMBO-001"),
            TEXT("Combo requires two distinct follow-ups, finite buffer .05...50, same skeleton / DefaultSlot / no root motion, one hit state each, one later combo state only on stages 1/2 before blend-out."),Issues);
    }
    const FString Package=Blueprint->GetOutermost()->GetName();
    if ((Package==TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack") && !Attack->IsComboEnabled()) ||
        (Package==TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack") && Attack->IsComboEnabled()))
    {
        AddIssue(Blueprint,TEXT("ET-COMBO-002"),TEXT("Formal player requires Combo enabled; formal enemy must keep Combo disabled."),Issues);
    }

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