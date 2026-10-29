#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicEquipmentComponent.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicEquipmentTransactionTest,
    "EmberTrial.M3.Inventory.EquipmentTransactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicEquipmentTransactionTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicInventoryResult;
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    // FTestWorldWrapper(Game)会创建并初始化自己的GI，直接使用其真实子系统。
    // 不替换World的GI，不另外Init / Shutdown第二份实例。
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    UGameInstance* GI = World->GetGameInstance();
    UETRelicInventorySubsystem* Inventory = IsValid(GI) ? GI->GetSubsystem<UETRelicInventorySubsystem>() : nullptr;
    UETRunSessionSubsystem* Session = IsValid(GI) ? GI->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Inventory subsystem"), Inventory) ||
        !TestNotNull(TEXT("Session subsystem"), Session) ||
        !TestTrue(TEXT("Configured inventory ready"), Inventory->GetSnapshot().bReady))
    {
        return false;
    }
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APlayerController* PC = World->SpawnActor<APlayerController>();
    auto SpawnPlayerBody = [&](const float X)
    {
        AEmberTrialCharacter* Result = World->SpawnActor<AEmberTrialCharacter>(
            FVector(X, 0, 200), FRotator::ZeroRotator, Spawn);
        if (Result)
        {
            Result->CombatTeam = EETCombatTeam::Player;
            Result->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
        }
        return Result;
    };
    AEmberTrialCharacter* Body = SpawnPlayerBody(0);
    AEmberTrialCharacter* RetryBody = SpawnPlayerBody(400);
    AEmberTrialCharacter* NewRunBody = SpawnPlayerBody(800);
    if (!TestNotNull(TEXT("Controller"), PC) || !TestNotNull(TEXT("Body"), Body) ||
        !TestNotNull(TEXT("Retry body"), RetryBody) || !TestNotNull(TEXT("NewRun body"), NewRunBody))
    {
        return false;
    }
    PC->Possess(Body);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* RetryEquipment = RetryBody->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* NewEquipment = NewRunBody->FindComponentByClass<UETRelicEquipmentComponent>();
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAttribute Max = UETHealthAttributeSet::GetMaxHealthAttribute();
    const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    if (!TestNotNull(TEXT("Equipment"), Equipment) || !TestNotNull(TEXT("Retry equipment"), RetryEquipment) ||
        !TestNotNull(TEXT("New equipment"), NewEquipment) || !TestNotNull(TEXT("ASC"), ASC))
    {
        return false;
    }
    TestTrue(TEXT("Uses real same-GI domain objects"), Equipment->InventorySource.Get() == Inventory &&
             Equipment->SessionSource.Get() == Session);
    Equipment->RefreshEquipmentState();
    TestTrue(TEXT("Restored empty inventory ready"), Equipment->IsEquipmentReady());
    const FActiveGameplayEffectHandle Handle = Equipment->OwnedEffect;
    // ---- 预览反复执行没有属性 / GE / 库存副作用，失败与NoChange清空输出 ----
    const uint64 PreviewRevision = Inventory->GetSnapshot().Revision;
    const int32 EffectsBeforePreview = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
    FETRelicLoadoutPlan Preview;
    for (int32 Index = 0; Index < 20; ++Index)
    {
        TestTrue(TEXT("Life preview succeeds"), Equipment->TryBuildPreview(
            EETRelicSlot::First, LifeId, PreviewRevision, Preview).Result == EResult::Success);
    }
    TestEqual(TEXT("Preview max120"), Preview.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Preview attack25"), Preview.AfterPreview.LightAttackBaseDamage, 25.0f);
    TestTrue(TEXT("Preview leaves slots and revision unchanged"),
        Inventory->GetSnapshot().Revision == PreviewRevision && Inventory->GetSnapshot().Equipped.SlotA.IsNone());
    TestEqual(TEXT("Preview leaves real max100"), ASC->GetNumericAttribute(Max), 100.0f);
    TestEqual(TEXT("Preview leaves real health100"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Preview adds no effects"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), EffectsBeforePreview);
    TestTrue(TEXT("Preview preserves owned handle"), Equipment->OwnedEffect == Handle);
    TestTrue(TEXT("Unowned preview rejected"), Equipment->TryBuildPreview(
        EETRelicSlot::First, FName(TEXT("missing")), PreviewRevision, Preview).Result == EResult::RuleRejected);
    TestTrue(TEXT("Rejected preview clears previous success"), Preview.After.SlotA.IsNone() &&
        Preview.AfterPreview.MaxHealth == 0.0f);
    auto Equip = [&](EETRelicSlot Slot, FName Id)
    {
        return Equipment->TryEquip(Slot, Id, Inventory->GetSnapshot().Revision).Result;
    };

    // ---- 一次真实提交；属性回调已能读到新ID，重入写在提交点期间被阻止 ----
    int32 DomainEvents = 0;
    bool bDomainSeesSettled = false;
    const FDelegateHandle DomainHandle = Inventory->OnChanged.AddLambda(
        [&](const FETRelicInventorySnapshot& Changed)
        {
            ++DomainEvents;
            bDomainSeesSettled = Equipment->IsEquipmentReady() &&
                ASC->GetNumericAttribute(Max) == 120.0f && Changed.Equipped.SlotA == LifeId;
        });
    bool bAttributeSeesNewIds = false;
    EResult Nested = EResult::Success;
    const FDelegateHandle AttributeHandle = ASC->GetGameplayAttributeValueChangeDelegate(Max).AddLambda(
        [&](const FOnAttributeChangeData& Change)
        {
            bAttributeSeesNewIds = Inventory->GetSnapshot().Equipped.SlotA == LifeId;
            Nested = Equip(EETRelicSlot::Second, EdgeId);
        });
    const uint64 OldRevision = Inventory->GetSnapshot().Revision;
    TestTrue(TEXT("Equip life"), Equip(EETRelicSlot::First, LifeId) == EResult::Success);
    TestTrue(TEXT("Attribute sees committed IDs and nested write is Busy"),
             bAttributeSeesNewIds && Nested == EResult::Busy);
    TestTrue(TEXT("Domain event after settled body"), bDomainSeesSettled);
    TestEqual(TEXT("One domain event"), DomainEvents, 1);
    ASC->GetGameplayAttributeValueChangeDelegate(Max).Remove(AttributeHandle);
    Inventory->OnChanged.Remove(DomainHandle);
    TestEqual(TEXT("Live equip does not heal"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Max120"), ASC->GetNumericAttribute(Max), 120.0f);
    TestEqual(TEXT("Max Base100"), ASC->GetNumericAttributeBase(Max), 100.0f);
    TestTrue(TEXT("Preview existing life plus edge"), Equipment->TryBuildPreview(
        EETRelicSlot::Second, EdgeId, Inventory->GetSnapshot().Revision, Preview).Result == EResult::Success);
    TestEqual(TEXT("Existing equipment not counted twice"), Preview.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Edge preview30"), Preview.AfterPreview.LightAttackBaseDamage, 30.0f);
    TestTrue(TEXT("Same item preview NoChange"), Equipment->TryBuildPreview(
        EETRelicSlot::First, LifeId, Inventory->GetSnapshot().Revision, Preview).Result == EResult::NoChange);
    TestEqual(TEXT("NoChange clears preview output"), Preview.AfterPreview.MaxHealth, 0.0f);
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestTrue(TEXT("Busy body may read preview"), Equipment->TryBuildPreview(
        EETRelicSlot::Second, EdgeId, Inventory->GetSnapshot().Revision, Preview).Result == EResult::Success);
    TestTrue(TEXT("Busy body cannot commit"), Equipment->TryEquip(
        EETRelicSlot::Second, EdgeId, Inventory->GetSnapshot().Revision).Result == EResult::NotReady);
    ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestTrue(TEXT("Same exact GE handle"), Equipment->OwnedEffect == Handle);
    TestTrue(TEXT("Stale UI version rejected"),
        Equipment->TryEquip(EETRelicSlot::Second, EdgeId, OldRevision).Result == EResult::StalePlan);
    TestTrue(TEXT("Duplicate rejected"), Equip(EETRelicSlot::Second, LifeId) == EResult::RuleRejected);
    TestTrue(TEXT("Unowned rejected"), Equip(EETRelicSlot::Second, FName(TEXT("missing"))) == EResult::RuleRejected);
    TestTrue(TEXT("Same item NoChange"), Equip(EETRelicSlot::First, LifeId) == EResult::NoChange);
    TestEqual(TEXT("Rejections preserve health"), ASC->GetNumericAttribute(Health), 100.0f);
    TestTrue(TEXT("Equip edge"), Equip(EETRelicSlot::Second, EdgeId) == EResult::Success);
    TestEqual(TEXT("Real attack bonus5"), ASC->GetNumericAttribute(Bonus), 5.0f);
    float Damage = 0.0f;
    TestTrue(TEXT("Attack formula resolves30"), UETGA_LightAttack::TryCalculateStageDamage(
        Body->GetLightAttackConfiguration()->GetBaseAttackDamage(), ASC->GetNumericAttribute(Bonus), 1.0f, Damage));
    TestEqual(TEXT("Resolved30"), Damage, 30.0f);

    // ---- 活体卸下只夹限；失活 / 重激活与重复观察不恢复生命 ----
    ASC->SetNumericAttributeBase(Health, 110.0f);
    TestTrue(TEXT("Unequip life"), Equip(EETRelicSlot::First, NAME_None) == EResult::Success);
    TestEqual(TEXT("Current clamps100"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Base clamps100"), ASC->GetNumericAttributeBase(Health), 100.0f);
    TestTrue(TEXT("Equip life again"), Equip(EETRelicSlot::First, LifeId) == EResult::Success);
    ASC->SetNumericAttributeBase(Health, 70.0f);
    Equipment->Deactivate();
    Equipment->Activate(false);
    Equipment->RefreshEquipmentState();
    Equipment->RefreshEquipmentState();
    TestEqual(TEXT("Reactivation retains70"), ASC->GetNumericAttribute(Health), 70.0f);
    TestEqual(TEXT("Reactivation restores max120"), ASC->GetNumericAttribute(Max), 120.0f);
    Session->bTravelPending = true;
    TestFalse(TEXT("Pending locks input readiness"), Equipment->IsEquipmentReady());
    TestTrue(TEXT("Pending rejects mutation"), Equip(EETRelicSlot::First, NAME_None) == EResult::NotReady);
    const uint64 BeforeFailedTravel = Inventory->GetSnapshot().Revision;
    Session->bTravelPending = false;
    Session->OnTravelFailed.Broadcast(World, FText::GetEmpty());
    TestTrue(TEXT("Failed travel resumes unchanged inventory"), Equipment->IsEquipmentReady() &&
        Inventory->GetSnapshot().Revision == BeforeFailedTravel);
    TestEqual(TEXT("Failed travel never refills living body"), ASC->GetNumericAttribute(Health), 70.0f);

    // ---- 模拟确认意图 / 新身体，真实OpenLevel留实机；不是旧身体迁移ASC ----
    Session->bTravelPending = true;
    PC->Possess(RetryBody);
    RetryEquipment->RefreshEquipmentState();
    TestFalse(TEXT("New body waits pending"), RetryEquipment->IsEquipmentReady());
    TestTrue(TEXT("Old body cannot write inventory"), Equip(EETRelicSlot::First, NAME_None) == EResult::NotReady);
    TestEqual(TEXT("Old body keeps its physical max"), ASC->GetNumericAttribute(Max), 120.0f);
    Session->bTravelPending = false;
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::Retry);
    UAbilitySystemComponent* RetryASC = RetryBody->GetAbilitySystemComponent();
    TestTrue(TEXT("Retry body ready"), RetryEquipment->IsEquipmentReady());
    TestEqual(TEXT("Retry fills final max120"), RetryASC->GetNumericAttribute(Health), 120.0f);
    TestEqual(TEXT("Retry attack bonus restored"), RetryASC->GetNumericAttribute(Bonus), 5.0f);
    TestEqual(TEXT("Old body health still70"), ASC->GetNumericAttribute(Health), 70.0f);
    RetryASC->SetNumericAttributeBase(Health, 90.0f);
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::Continue);
    TestEqual(TEXT("Same body revision refresh does not refill"), RetryASC->GetNumericAttribute(Health), 90.0f);
    Session->bTravelPending = true;
    PC->Possess(NewRunBody);
    NewEquipment->RefreshEquipmentState();
    TestEqual(TEXT("Pending NewRun does not load previous gear"),
              NewRunBody->GetAbilitySystemComponent()->GetNumericAttribute(Max), 100.0f);
    Session->bTravelPending = false;
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::NewRun);
    TestTrue(TEXT("NewRun empty ready"), NewEquipment->IsEquipmentReady() &&
        Inventory->GetSnapshot().Equipped.SlotA.IsNone() && Inventory->GetSnapshot().Equipped.SlotB.IsNone());
    TestEqual(TEXT("NewRun final100"), NewRunBody->GetAbilitySystemComponent()->GetNumericAttribute(Health), 100.0f);

    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
    UAbilitySystemComponent* NewASC = NewRunBody->GetAbilitySystemComponent();
    NewASC->RemoveActiveGameplayEffect(NewEquipment->OwnedEffect);
    TestFalse(TEXT("Missing owned effect unavailable"), NewEquipment->IsEquipmentReady());
    const uint64 BeforeUnavailable = Inventory->GetSnapshot().Revision;
    TestTrue(TEXT("Missing handle refuses before model write"), NewEquipment->TryEquip(
        EETRelicSlot::First, LifeId, BeforeUnavailable).Result == EResult::NotReady);
    TestTrue(TEXT("Refusal preserves revision"), Inventory->GetSnapshot().Revision == BeforeUnavailable);
    Fixture.EndPlayInTestWorld();
    TestFalse(TEXT("End unsubscribes inventory"), NewEquipment->InventoryChangedHandle.IsValid());
    TestFalse(TEXT("End unsubscribes travel failure"), NewEquipment->TravelFailedHandle.IsValid());
    TestFalse(TEXT("End invalidates body handle"), RetryEquipment->OwnedEffect.IsValid());
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif