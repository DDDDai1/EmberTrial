#include "Combat/ETCombatDamage.h"

#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/HitResult.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

bool ETCombatDamage::IsValidConfiguration(TSubclassOf<UGameplayEffect> EffectClass, float Damage)
{
    if (!FMath::IsFinite(Damage) || Damage <= 0.0f || !IsValid(EffectClass.Get()))
    {
        return false;
    }

    const UGameplayEffect* Effect = EffectClass.GetDefaultObject();
    if (!IsValid(Effect) || Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }

    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    return Modifier.Attribute == UETHealthAttributeSet::GetHealthAttribute() &&
           Modifier.ModifierOp == EGameplayModOp::AddBase &&
           Modifier.ModifierMagnitude.GetMagnitudeCalculationType() ==
               EGameplayEffectMagnitudeCalculation::SetByCaller &&
           Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
               ETGameplayTags::SetByCaller_HealthDelta;
}

ETCombatDamage::FResult ETCombatDamage::ApplyHit(
    AEmberTrialCharacter* Source, UAbilitySystemComponent* SourceASC, const FHitResult& Hit,
    TSubclassOf<UGameplayEffect> EffectClass, float Damage, float Level,
    const UObject* SourceObject, AActor* EffectCauser)
{
    // 发射时的数值已经快照，身份、生命和防御Tag以命中时为准。
    float SourceHealth = 0.0f;
    float SourceMaxHealth = 0.0f;
    if (!IsValid(Source) || Source->IsActorBeingDestroyed() || Source->IsCombatDead() ||
        !Source->HasAuthority() || !IsValid(SourceASC) ||
        Source->GetAbilitySystemComponent() != SourceASC || !SourceASC->AbilityActorInfo.IsValid() ||
        SourceASC->AbilityActorInfo->OwnerActor.Get() != Source ||
        SourceASC->AbilityActorInfo->AvatarActor.Get() != Source ||
        !Source->GetGameplayHealthSnapshot(SourceHealth, SourceMaxHealth) ||
        !FMath::IsFinite(SourceHealth) || !FMath::IsFinite(SourceMaxHealth) ||
        SourceHealth <= 0.0f || SourceMaxHealth < 1.0f || SourceHealth > SourceMaxHealth ||
        UGameplayStatics::IsGamePaused(Source))
    {
        return {TEXT("SourceUnavailable")};
    }
    if (!IsValidConfiguration(EffectClass, Damage) || !FMath::IsFinite(Level) || Level <= 0.0f)
    {
        return {TEXT("InvalidDamageConfig")};
    }

    AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Hit.GetActor());
    if (!IsValid(Target) || Target == Source || Target->GetWorld() != Source->GetWorld())
    {
        return {TEXT("UnsupportedTarget")};
    }
    if (!Source->CanDealCombatDamageTo(Target))
    {
        return {TEXT("CombatTeam")};
    }
    if (Target->IsCombatDead())
    {
        return {TEXT("TargetDead")};
    }

    FResult Result;
    float MaxHealthBefore = 0.0f;
    if (!Target->GetGameplayHealthSnapshot(Result.HealthBefore, MaxHealthBefore) ||
        !FMath::IsFinite(Result.HealthBefore) || !FMath::IsFinite(MaxHealthBefore) ||
        MaxHealthBefore < 1.0f || Result.HealthBefore < 0.0f || Result.HealthBefore > MaxHealthBefore)
    {
        return {TEXT("TargetNotReady")};
    }
    if (Result.HealthBefore <= 0.0f)
    {
        return {TEXT("TargetZeroHealth")};
    }

    const FGameplayAttribute HealthAttribute = UETHealthAttributeSet::GetHealthAttribute();
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!IsValid(TargetASC) || !TargetASC->AbilityActorInfo.IsValid() ||
        TargetASC->AbilityActorInfo->OwnerActor.Get() != Target ||
        TargetASC->AbilityActorInfo->AvatarActor.Get() != Target ||
        !TargetASC->HasAttributeSetForAttribute(HealthAttribute))
    {
        return {TEXT("TargetASCUnavailable")};
    }

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    if (!Context.IsValid())
    {
        return {TEXT("ContextUnavailable")};
    }
    if (EffectCauser)
    {
        Context.AddInstigator(Source, EffectCauser);
    }
    Context.AddSourceObject(SourceObject);
    Context.AddHitResult(Hit, true);
    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, Level, Context);
    if (!Spec.IsValid())
    {
        return {TEXT("SpecUnavailable")};
    }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_HealthDelta, -Damage);

    if (TargetASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable))
    {
        return {TEXT("Invulnerable")};
    }

    const TWeakObjectPtr<AEmberTrialCharacter> WeakTarget(Target);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC(TargetASC);
    const FActiveGameplayEffectHandle Handle =
        SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
    Result.bApplied = Handle.WasSuccessfullyApplied();
    Result.bActiveHandle = Handle.IsValid();

    // 属性回调可能同步取消能力、销毁目标或更换ASC。
    const AEmberTrialCharacter* LiveTarget = WeakTarget.Get();
    const UAbilitySystemComponent* LiveASC = WeakTargetASC.Get();
    if (LiveTarget && LiveASC && LiveTarget->GetAbilitySystemComponent() == LiveASC &&
        LiveTarget->GetGameplayHealthSnapshot(Result.HealthAfter, Result.MaxAfter))
    {
        Result.BaseAfter = LiveASC->GetNumericAttributeBase(HealthAttribute);
        Result.bHasPostHitSnapshot = true;
    }
    return Result;
}