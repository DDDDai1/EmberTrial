#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicEquipmentEffectsTest,
    "EmberTrial.M3.Inventory.EquipmentEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicEquipmentEffectsTest::RunTest(const FString& Parameters)
{
    // ---- 数值顺序 / 非法输入：加成先于段倍率，失败输出不残留 ----
    float Damage = -1.0f;
    TestTrue(TEXT("Bonus precedes multiplier"),
             UETGA_LightAttack::TryCalculateStageDamage(25.0f, 5.0f, 2.0f, Damage));
    TestEqual(TEXT("Expected stage damage"), Damage, 60.0f);
    TestTrue(TEXT("Signed tradeoff supported"),
             UETGA_LightAttack::TryCalculateStageDamage(25.0f, -10.0f, 1.0f, Damage));
    TestEqual(TEXT("Tradeoff damage"), Damage, 15.0f);
    TestFalse(TEXT("Nonpositive total rejected"),
              UETGA_LightAttack::TryCalculateStageDamage(25.0f, -25.0f, 1.0f, Damage));
    TestEqual(TEXT("Rejected damage cleared"), Damage, 0.0f);
    TestFalse(TEXT("NaN bonus rejected"), UETGA_LightAttack::TryCalculateStageDamage(
        25.0f, std::numeric_limits<float>::quiet_NaN(), 1.0f, Damage));
    TestFalse(TEXT("Overflow rejected"), UETGA_LightAttack::TryCalculateStageDamage(
        std::numeric_limits<float>::max(), 0.0f, 2.0f, Damage));

    // ---- 真实身体 / ASC / Infinite GE；没有Actor控制或动画夹具 ----
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    AEmberTrialCharacter* Other = World->SpawnActor<AEmberTrialCharacter>(
        FVector(400, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Independent body"), Other))
    {
        return false;
    }
    float Bonus = 99.0f;
    TestFalse(TEXT("Before BeginPlay snapshot unavailable"),
              Body->GetGameplayLightAttackBonusSnapshot(Bonus));
    TestEqual(TEXT("Unavailable output cleared"), Bonus, 0.0f);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* OtherASC = Other->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("ASC"), ASC) || !TestNotNull(TEXT("Other ASC"), OtherASC) ||
        !TestNotNull(TEXT("Owned Combat set"), ASC->GetSet<UETCombatAttributeSet>()))
    {
        return false;
    }
    const FGameplayAttribute BonusAttribute = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    const FGameplayAttribute MaxHealth = UETHealthAttributeSet::GetMaxHealthAttribute();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    TestTrue(TEXT("Ready includes combat snapshot"), Body->GetGameplayLightAttackBonusSnapshot(Bonus));
    TestEqual(TEXT("Default bonus zero"), Bonus, 0.0f);
    int32 CombatSetCount = 0;
    for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
    {
        CombatSetCount += IsValid(Set) && Set->IsA<UETCombatAttributeSet>() ? 1 : 0;
    }
    TestEqual(TEXT("Exactly one combat set"), CombatSetCount, 1);
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
            UETRelicEquipmentEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid())
        {
            return FActiveGameplayEffectHandle();
        }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentMaxHealth, HealthAdditive);
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, AttackAdditive);
        return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    ASC->SetNumericAttributeBase(Health, 80.0f);
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f);
    if (!TestTrue(TEXT("Infinite effect has a live handle"), OwnedHandle.IsValid()))
    {
        return false;
    }
    TestEqual(TEXT("Effect raises current max"), ASC->GetNumericAttribute(MaxHealth), 120.0f);
    TestEqual(TEXT("Effect leaves max Base"), ASC->GetNumericAttributeBase(MaxHealth), 100.0f);
    TestEqual(TEXT("Effect adds current bonus"), ASC->GetNumericAttribute(BonusAttribute), 5.0f);
    TestEqual(TEXT("Effect leaves bonus Base"), ASC->GetNumericAttributeBase(BonusAttribute), 0.0f);
    TestEqual(TEXT("Increasing max does not heal"), ASC->GetNumericAttribute(Health), 80.0f);
    TestTrue(TEXT("Repeated Ensure stays initialized"), Body->EnsureGameplayInitialized());
    TestEqual(TEXT("Repeated Ensure keeps equipment"), ASC->GetNumericAttribute(BonusAttribute), 5.0f);

    // 非有限Base回退到旧Base0，而不是含活动GE的Current5。
    ASC->SetNumericAttributeBase(BonusAttribute, std::numeric_limits<float>::quiet_NaN());
    TestEqual(TEXT("NaN Base retains zero"), ASC->GetNumericAttributeBase(BonusAttribute), 0.0f);
    TestEqual(TEXT("NaN does not double current bonus"), ASC->GetNumericAttribute(BonusAttribute), 5.0f);
    TMap<FGameplayTag, float> Replacement;
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentMaxHealth, 10.0f);
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 3.0f);
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(OwnedHandle, Replacement);
    TestEqual(TEXT("Same handle updates max"), ASC->GetNumericAttribute(MaxHealth), 110.0f);
    TestEqual(TEXT("Same handle updates bonus"), ASC->GetNumericAttribute(BonusAttribute), 3.0f);
    TestTrue(TEXT("Owned handle still live"), ASC->GetActiveGameplayEffect(OwnedHandle) != nullptr);

    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f);
    if (!TestTrue(TEXT("Independent effect active"), OtherHandle.IsValid()))
    {
        return false;
    }
    TestTrue(TEXT("Remove owned handle"), ASC->RemoveActiveGameplayEffect(OwnedHandle));
    TestEqual(TEXT("Other effect survives exact removal"), ASC->GetNumericAttribute(BonusAttribute), 2.0f);
    TestTrue(TEXT("Remove second handle"), ASC->RemoveActiveGameplayEffect(OtherHandle));
    TestEqual(TEXT("Removed max returns to Base"), ASC->GetNumericAttribute(MaxHealth), 100.0f);
    TestEqual(TEXT("Removed bonus returns to zero"), ASC->GetNumericAttribute(BonusAttribute), 0.0f);
    TestEqual(TEXT("Removal preserves health within new limit"), ASC->GetNumericAttribute(Health), 80.0f);
    TestEqual(TEXT("Other body max isolated"), OtherASC->GetNumericAttribute(MaxHealth), 100.0f);
    TestEqual(TEXT("Other body bonus isolated"), OtherASC->GetNumericAttribute(BonusAttribute), 0.0f);
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Bonus = 99.0f;
    TestFalse(TEXT("EndPlay snapshot revoked"), Body->GetGameplayLightAttackBonusSnapshot(Bonus));
    TestEqual(TEXT("EndPlay output cleared"), Bonus, 0.0f);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif