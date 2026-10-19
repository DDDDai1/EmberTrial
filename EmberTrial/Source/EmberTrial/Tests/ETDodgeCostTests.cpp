#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_Dodge.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETDodgeCostContractTest, "EmberTrial.M2.Dodge.CostContract",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETDodgeCostContractTest::RunTest(const FString& Parameters)
{
    // ---- 保存资产作为成本契约，临时世界 / 原生身体负责真实ASC修改 ----
    UClass* CostClass = LoadClass<UGameplayEffect>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost.GE_ET_DodgeCost_C"));
    if (!TestNotNull(TEXT("Create and save the formal Cost GE before running this test"),
                     CostClass))
    {
        return false;
    }
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    AEmberTrialCharacter* Body = TestWorld.GetTestWorld()->SpawnActor<AEmberTrialCharacter>();
    if (!TestNotNull(TEXT("Temporary native body"), Body))
    {
        return false;
    }
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Temporary ASC"), ASC) || !ASC->AbilityActorInfo.IsValid())
    {
        return false;
    }
    const FGameplayAbilitySpecHandle Handle =
        ASC->GiveAbility(FGameplayAbilitySpec(UETGA_Dodge::StaticClass(), 1));
    FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
    UETGA_Dodge* Ability = Spec ? Cast<UETGA_Dodge>(Spec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("Granted primary instance"), Ability))
    {
        return false;
    }
    // 只配置授予后的瞬态主实例，不写保存资产或CDO。
    Ability->CostGameplayEffectClass = CostClass; // Transient instance only, never the asset CDO.
    const FGameplayAbilityActorInfo* Info = ASC->AbilityActorInfo.Get();
    const FGameplayAbilityActivationInfo Activation;
    const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
    auto Value = [&]() { return ASC->GetNumericAttribute(Stamina); };
    float Cost = 0.0f;
    TestTrue(TEXT("Formal fixed cost schema"),
             UETGA_Dodge::ReadCostAmount(Ability->GetCostGameplayEffect(), 1.0f, Cost));
    TestEqual(TEXT("Formal default cost"), Cost, 25.0f);
    TestTrue(TEXT("100 can pay"), Ability->CheckCost(Handle, Info, nullptr));
    // ---- 同步属性回调内再次付款，验证重入与重复提交仍只产生一次扣费 ----
    int32 Changes = 0;
    const FDelegateHandle Observer =
        ASC->GetGameplayAttributeValueChangeDelegate(Stamina).AddLambda(
            [&](const FOnAttributeChangeData& Change)
            {
                ++Changes;
                Ability->ApplyCost(
                    Handle, Info,
                    Activation); // A synchronous recursive attempt must not charge twice.
            });
    Ability->ApplyCost(Handle, Info, Activation);
    Ability->ApplyCost(Handle, Info, Activation);
    ASC->GetGameplayAttributeValueChangeDelegate(Stamina).Remove(Observer);
    TestEqual(TEXT("One real payment"), Value(), 75.0f);
    TestEqual(TEXT("Duplicate and recursive payment produce one change"), Changes, 1);
    TestEqual(TEXT("Base also paid once"), ASC->GetNumericAttributeBase(Stamina), 75.0f);

    // ---- 少1点拒绝、恰好足额可付；期望值独立于生产配置 ----
    ASC->SetNumericAttributeBase(Stamina, 24.0f);
    Ability->bCostSubmitted = false; // Simulate the next attempt, without starting any montage.
    TestFalse(TEXT("24 cannot pay 25"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Rejected payment preserves 24"), Value(), 24.0f);
    TestFalse(TEXT("Rejected payment is not marked submitted"), Ability->bCostSubmitted);
    ASC->SetNumericAttributeBase(Stamina, 25.0f);
    TestTrue(TEXT("Exactly 25 can pay"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Exact payment leaves zero"), Value(), 0.0f);
    TestEqual(TEXT("Exact payment Base zero"), ASC->GetNumericAttributeBase(Stamina), 0.0f);
    TestTrue(TEXT("Zero stamina remains Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Zero stamina cannot pay again"), Ability->CheckCost(Handle, Info, nullptr));
    TestFalse(TEXT("Zero stamina is not death"), Body->IsCombatDead());

    ASC->SetNumericAttributeBase(Stamina, 100.0f);
    Ability->bCostSubmitted = false;
    // ---- 缺失成本与死亡不得转成免费闪避；此处只测成本，不播放Montage ----
    Ability->CostGameplayEffectClass = nullptr;
    TestFalse(TEXT("Missing cost cannot become free dodge"),
              Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Missing cost writes nothing"), Value(), 100.0f);
    Ability->CostGameplayEffectClass = CostClass;
    TestEqual(TEXT("Cost never changes Health"),
              ASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);
    ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestFalse(TEXT("Dead body cannot pay"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Dead body resource unchanged"), Value(), 100.0f);
    // ---- 先撤销本次授予，再结束临时世界并转发真实引擎错误 ----
    ASC->ClearAbility(Handle);
    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
