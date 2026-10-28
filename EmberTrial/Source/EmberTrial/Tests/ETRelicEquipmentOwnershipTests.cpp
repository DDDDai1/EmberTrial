#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicEquipmentComponent.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicEquipmentOwnershipTest,
    "EmberTrial.M3.Inventory.EquipmentOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicEquipmentOwnershipTest::RunTest(const FString& Parameters)
{
    using EWrite = EETRelicEffectWriteResult;
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
    AEmberTrialCharacter* Neutral = World->SpawnActor<AEmberTrialCharacter>(
        FVector(800, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Player body"), Body) || !TestNotNull(TEXT("Other player body"), Other) ||
        !TestNotNull(TEXT("Neutral body"), Neutral))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Player;
    Other->CombatTeam = EETCombatTeam::Player;
    UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* OtherEquipment = Other->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* NeutralEquipment = Neutral->FindComponentByClass<UETRelicEquipmentComponent>();
    if (!TestNotNull(TEXT("Native component"), Equipment) ||
        !TestNotNull(TEXT("Other native component"), OtherEquipment) ||
        !TestNotNull(TEXT("Neutral inherits component"), NeutralEquipment))
    {
        return false;
    }
    FETRelicModifierTotals Snapshot;
    TestFalse(TEXT("Before BeginPlay unavailable"), Equipment->GetAppliedModifiers(Snapshot));
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* OtherASC = Other->GetAbilitySystemComponent();
    const FGameplayAttribute MaxHealth = UETHealthAttributeSet::GetMaxHealthAttribute();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    auto CountOwned = [](UAbilitySystemComponent* SourceASC, UETRelicEquipmentComponent* Source)
    {
        int32 Count = 0;
        for (const FActiveGameplayEffectHandle Handle : SourceASC->GetActiveEffects(FGameplayEffectQuery()))
        {
            const FActiveGameplayEffect* Effect = SourceASC->GetActiveGameplayEffect(Handle);
            Count += Effect && Effect->Spec.GetContext().GetSourceObject() == Source ? 1 : 0;
        }
        return Count;
    };

    // ---- 零幅度槽与重复初始化；非玩家不创建效果 ----
    TestTrue(TEXT("Player effect available"), Equipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("Empty max modifier"), Snapshot.MaxHealthAdditive, 0.0f);
    TestEqual(TEXT("Empty attack modifier"), Snapshot.LightAttackBonus, 0.0f);
    TestFalse(TEXT("Neutral remains dormant"), NeutralEquipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("One owned active effect"), CountOwned(ASC, Equipment), 1);
    const FActiveGameplayEffectHandle InitialHandle = Equipment->OwnedEffect;
    TestTrue(TEXT("Repeated Ensure stays ready"), Body->EnsureGameplayInitialized());
    TestTrue(TEXT("Repeated activation reuses handle"), Equipment->EnsureOwnedEffect());
    TestTrue(TEXT("Stable owned handle"), Equipment->OwnedEffect == InitialHandle);
    TestEqual(TEXT("No duplicate active effect"), CountOwned(ASC, Equipment), 1);

    // ---- 同一句柄更新，不经过基础上限100；提高上限不免费治疗 ----
    TestTrue(TEXT("Apply20 and5"), Equipment->TrySetModifiers({20.0f, 5.0f}) == EWrite::Applied);
    TestEqual(TEXT("Max120"), ASC->GetNumericAttribute(MaxHealth), 120.0f);
    TestEqual(TEXT("Bonus5"), ASC->GetNumericAttribute(Bonus), 5.0f);
    TestEqual(TEXT("Increasing max does not heal"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Max Base stays100"), ASC->GetNumericAttributeBase(MaxHealth), 100.0f);
    TestEqual(TEXT("Bonus Base stays0"), ASC->GetNumericAttributeBase(Bonus), 0.0f);
    ASC->SetNumericAttributeBase(Health, 110.0f);
    int32 MaxEvents = 0;
    bool bTemporaryDrop = false;
    EWrite NestedWrite = EWrite::Applied;
    const FDelegateHandle MaxObserver = ASC->GetGameplayAttributeValueChangeDelegate(MaxHealth).AddLambda(
        [&](const FOnAttributeChangeData& Change)
        {
            ++MaxEvents;
            bTemporaryDrop |= Change.NewValue < 120.0f;
            NestedWrite = Equipment->TrySetModifiers({0.0f, 0.0f});
        });
    TestTrue(TEXT("Update40 and7"), Equipment->TrySetModifiers({40.0f, 7.0f}) == EWrite::Applied);
    TestTrue(TEXT("Same handle after update"), Equipment->OwnedEffect == InitialHandle);
    TestEqual(TEXT("Max140"), ASC->GetNumericAttribute(MaxHealth), 140.0f);
    TestEqual(TEXT("Health110 preserved"), ASC->GetNumericAttribute(Health), 110.0f);
    TestEqual(TEXT("One final max event"), MaxEvents, 1);
    TestFalse(TEXT("No intermediate max drop"), bTemporaryDrop);
    TestTrue(TEXT("Nested write is Busy"), NestedWrite == EWrite::Busy);
    TestTrue(TEXT("Identical totals are unchanged"), Equipment->TrySetModifiers({40.0f, 7.0f}) == EWrite::Unchanged);
    TestEqual(TEXT("Unchanged emits no extra max event"), MaxEvents, 1);
    ASC->GetGameplayAttributeValueChangeDelegate(MaxHealth).Remove(MaxObserver);

    // ---- 降上限只夹最终越界值；夹限没有伤害GE来源 ----
    bool bClampHadDamageSource = false;
    const FDelegateHandle HealthObserver = ASC->GetGameplayAttributeValueChangeDelegate(Health).AddLambda(
        [&](const FOnAttributeChangeData& Change) { bClampHadDamageSource |= Change.GEModData != nullptr; });
    TestTrue(TEXT("Signed attack modifier"), Equipment->TrySetModifiers({5.0f, -3.0f}) == EWrite::Applied);
    TestEqual(TEXT("Final max105"), ASC->GetNumericAttribute(MaxHealth), 105.0f);
    TestEqual(TEXT("Current health clamped105"), ASC->GetNumericAttribute(Health), 105.0f);
    TestEqual(TEXT("Base health clamped105"), ASC->GetNumericAttributeBase(Health), 105.0f);
    TestFalse(TEXT("Clamp is not a damage GE"), bClampHadDamageSource);
    ASC->GetGameplayAttributeValueChangeDelegate(Health).Remove(HealthObserver);
    TestFalse(TEXT("Clamp did not kill body"), Body->IsCombatDead());
    TestTrue(TEXT("NaN rejected"), Equipment->TrySetModifiers(
        {std::numeric_limits<float>::quiet_NaN(), 0.0f}) == EWrite::InvalidModifiers);
    TestTrue(TEXT("Nonpositive final max rejected"),
             Equipment->TrySetModifiers({-101.0f, 0.0f}) == EWrite::InvalidModifiers);
    TestTrue(TEXT("Infinite attack rejected"), Equipment->TrySetModifiers(
        {0.0f, std::numeric_limits<float>::infinity()}) == EWrite::InvalidModifiers);
    TestEqual(TEXT("Refusal leaves current max"), ASC->GetNumericAttribute(MaxHealth), 105.0f);
    TestEqual(TEXT("Refusal leaves signed bonus"), ASC->GetNumericAttribute(Bonus), -3.0f);
    TestEqual(TEXT("Other body max isolated"), OtherASC->GetNumericAttribute(MaxHealth), 100.0f);

    // ---- 同类但非自有的GE必须保留；空槽用0更新，失活才移除句柄 ----
    FGameplayEffectSpecHandle ForeignSpec = ASC->MakeOutgoingSpec(
        UETRelicEquipmentEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!TestTrue(TEXT("Independent spec"), ForeignSpec.IsValid()))
    {
        return false;
    }
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentMaxHealth, 0.0f);
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 2.0f);
    const FActiveGameplayEffectHandle Foreign = ASC->ApplyGameplayEffectSpecToSelf(*ForeignSpec.Data.Get());
    if (!TestTrue(TEXT("Independent active effect"), Foreign.IsValid()))
    {
        return false;
    }
    TestTrue(TEXT("Empty totals update owned GE"), Equipment->TrySetModifiers({}) == EWrite::Applied);
    TestTrue(TEXT("Empty retains owned handle"), Equipment->OwnedEffect == InitialHandle);
    TestEqual(TEXT("Foreign bonus2 retained"), ASC->GetNumericAttribute(Bonus), 2.0f);
    Equipment->Deactivate();
    TestFalse(TEXT("Inactive view unavailable"), Equipment->GetAppliedModifiers(Snapshot));
    TestTrue(TEXT("Inactive owns no effect"), ASC->GetActiveGameplayEffect(InitialHandle) == nullptr);
    TestTrue(TEXT("Foreign survives deactivation"), ASC->GetActiveGameplayEffect(Foreign) != nullptr);
    Equipment->Activate(false);
    TestTrue(TEXT("Explicit reactivation starts zero slot"), Equipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("Reactivated own attack zero"), Snapshot.LightAttackBonus, 0.0f);
    TestEqual(TEXT("Exactly one rebuilt own effect"), CountOwned(ASC, Equipment), 1);

    // 外部移除不被缓存掩盖，也不静默补建；显式失活 / 激活才能重新建空槽。
    ASC->RemoveActiveGameplayEffect(Equipment->OwnedEffect);
    TestFalse(TEXT("External removal detected"), Equipment->GetAppliedModifiers(Snapshot));
    TestTrue(TEXT("Lost handle refuses write"), Equipment->TrySetModifiers({20.0f, 5.0f}) == EWrite::Unavailable);
    TestFalse(TEXT("Ensure does not hide externally lost handle"), Equipment->EnsureOwnedEffect());
    Equipment->Deactivate();
    Equipment->Activate(false);
    const FActiveGameplayEffectHandle BeforeDestroy = Equipment->OwnedEffect;
    Equipment->DestroyComponent();
    TestTrue(TEXT("Component end removes exact own handle"), ASC->GetActiveGameplayEffect(BeforeDestroy) == nullptr);
    TestTrue(TEXT("Component end preserves foreign GE"), ASC->GetActiveGameplayEffect(Foreign) != nullptr);
    ASC->RemoveActiveGameplayEffect(Foreign);

    const FActiveGameplayEffectHandle OtherHandle = OtherEquipment->OwnedEffect;
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Snapshot.MaxHealthAdditive = 99.0f;
    TestFalse(TEXT("World end revokes snapshot"), OtherEquipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("Unavailable output cleared"), Snapshot.MaxHealthAdditive, 0.0f);
    TestTrue(TEXT("World end removes owned effect"), OtherASC->GetActiveGameplayEffect(OtherHandle) == nullptr);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif