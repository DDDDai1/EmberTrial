#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETHealingAbilityTest,
    "EmberTrial.M2.Healing.Ability",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETHealingAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* HealClass=LoadClass<UETGA_Heal>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal.GA_ET_Heal_C"));
    UClass* HitClass=LoadClass<UETGA_HitReact>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    UClass* AttackClass=LoadClass<UETGA_LightAttack>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass=LoadClass<UETGA_Dodge>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    USkeletalMesh* Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved heal GA"),HealClass) || !TestNotNull(TEXT("Saved hit GA"),HitClass) ||
        !TestNotNull(TEXT("Attack GA"),AttackClass) || !TestNotNull(TEXT("Dodge GA"),DodgeClass) || !TestNotNull(TEXT("Quinn"),Quinn)) { return false; }
    if (!TestTrue(TEXT("Saved heal config"),HealClass->GetDefaultObject<UETGA_Heal>()->HasValidConfiguration()) ||
        !TestTrue(TEXT("Saved hit cancels Healing"),HitClass->GetDefaultObject<UETGA_HitReact>()->HasValidHealingCancelPolicy())) { return false; }
    for (int32 Scenario=0;Scenario<13;++Scenario)
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this);return false; }
        UWorld* World=Fixture.GetTestWorld();
        AActor* Floor=World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"),Floor)) { return false; }
        UBoxComponent* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
        Box->SetMobility(EComponentMobility::Static);Box->SetBoxExtent(FVector(2000,2000,50));
        Box->SetRelativeLocation(FVector(0,0,-50));Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
        FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body=World->SpawnActor<AEmberTrialCharacter>(FVector(0,0,200),FRotator::ZeroRotator,Spawn);
        APlayerController* PC=World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Body"),Body) || !TestNotNull(TEXT("Controller"),PC)) { return false; }
        Body->CombatTeam=EETCombatTeam::Player;Body->InitialHealth=40.0f;
        Body->HealAbilityClass=HealClass;Body->LightAttackAbilityClass=AttackClass;Body->DodgeAbilityClass=DodgeClass;
        Body->CombatLifeComponent->HitReactAbilityClass=HitClass;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->SetRelativeLocation(FVector(0,0,-90));Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->bRunPhysicsWithNoController=true;PC->Possess(Body);
        if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        auto Advance=[&Fixture](float Seconds)
        { for (int32 I=0;I<FMath::CeilToInt(Seconds/.01f);++I) { if (!Fixture.TickTestWorld(.01f)) { return false; } }return true; };
        auto Until=[&Fixture](auto Predicate,float Limit)
        { for (int32 I=0;I<FMath::CeilToInt(Limit/.01f) && !Predicate();++I) { if (!Fixture.TickTestWorld(.01f)) { return false; } }return Predicate(); };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Grounded"),Body->GetCharacterMovement()->IsMovingOnGround())) { return false; }
        UAbilitySystemComponent* ASC=Body->GetAbilitySystemComponent();
        FGameplayAbilitySpec* Spec=ASC->FindAbilitySpecFromHandle(Body->HealAbilityHandle);
        UETGA_Heal* Heal=Spec ? Cast<UETGA_Heal>(Spec->GetPrimaryInstance()) : nullptr;
        UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
        UETStaminaRecoveryComponent* Recovery=Body->FindComponentByClass<UETStaminaRecoveryComponent>();
        if (!TestNotNull(TEXT("Heal instance"),Heal) || !TestNotNull(TEXT("Healing resource"),Resource) || !TestNotNull(TEXT("Recovery"),Recovery)) { return false; }
        UAnimMontage* Montage=Heal->HealMontage;
        UAnimInstance* Anim=Body->GetMesh()->GetAnimInstance();
        const FGameplayAttribute Health=UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Stamina=UETStaminaAttributeSet::GetStaminaAttribute();
        auto Charges=[Resource]() { int32 C=0,M=0;return Resource->GetHealingSnapshot(C,M) ? C : INDEX_NONE; };
        auto Check=[this,ASC,Health,&Charges](const TCHAR* Label,float Value,int32 Count)
        {
            TestEqual(FString::Printf(TEXT("%s Health Current"),Label),ASC->GetNumericAttribute(Health),Value);
            TestEqual(FString::Printf(TEXT("%s Health Base"),Label),ASC->GetNumericAttributeBase(Health),Value);
            TestEqual(FString::Printf(TEXT("%s charges"),Label),Charges(),Count);
        };
        auto Idle=[&Until,&Advance,Body]() { return Until([Body](){return Body->IsFormalActionIdle();},3.0f) && Advance(.25f); };
        if (Scenario==0)
        {
            Heal->HealMontage=nullptr;TestFalse(TEXT("None montage rejects"),Body->RequestHeal());Heal->HealMontage=Montage;
            ASC->SetNumericAttributeBase(Health,100.0f);TestFalse(TEXT("Full health rejects"),Body->RequestHeal());
            ASC->SetNumericAttributeBase(Health,40.0f);
            Body->GetCharacterMovement()->Velocity=FVector(10,0,0);
            TestFalse(TEXT("Moving rejects"),Body->RequestHeal());Body->GetCharacterMovement()->Velocity=FVector::ZeroVector;
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);TestFalse(TEXT("Air rejects"),Body->RequestHeal());
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Check(TEXT("Prechecks"),40.0f,3);
            ASC->SetNumericAttributeBase(Stamina,75.0f);
        }
        if (Scenario==4)
        {
            for (int32 I=0;I<3;++I)
            {
                if (!TestTrue(TEXT("Arrange consumed charge with real GE"),Resource->TryApplyHealing(40.0f))) { return false; }
                ASC->SetNumericAttributeBase(Health,40.0f);
            }
            TestFalse(TEXT("Zero charges rejects activation"),Body->RequestHeal());Check(TEXT("Exhausted"),40.0f,0);
        }
        else
        {
            if (!TestTrue(TEXT("Start heal"),Body->RequestHeal()) || !TestTrue(TEXT("Healing tag owned"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing))) { return false; }
            const int32 FirstID=Heal->ActiveMontageInstanceID;
            UAnimNotify* Token=Heal->ActiveCommitToken.Get();
            const TStrongObjectPtr<UAbilityTask_PlayMontageAndWait> FirstTask(Heal->MontageTask);
            if (!TestNotNull(TEXT("Started montage task"),FirstTask.Get()) ||
                !TestTrue(TEXT("Actual montage instance"),FirstID!=INDEX_NONE)) { return false; }
            if (!TestFalse(TEXT("Busy repeat refuses"),Body->RequestHeal())) { return false; }
            TestFalse(TEXT("Attack blocked while Healing"),Body->RequestLightAttack());
            TestFalse(TEXT("Dodge blocked while Healing"),Body->RequestDodge());
            TestFalse(TEXT("Shared idle closes move / jump / E"),Body->IsFormalActionIdle());
            Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token); // Negative early-notify injection.
            Check(TEXT("Before genuine notify"),40.0f,3);
            if (Scenario==1)
            {
                if (!Advance(.20f) || !TestTrue(TEXT("Precommit hit accepted"),Body->RequestHitReact()) || !Idle()) { return false; }
                Check(TEXT("Precommit cancel"),40.0f,3);
            }
            else if (Scenario==6 || Scenario==7)
            {
                if (Scenario==7 && !Until([Heal](){return Heal->bCommitAttempted;},2.0f)) { return false; }
                const int32 ExpectedCharges=Scenario==7 ? 2 : 3;
                ASC->SetNumericAttributeBase(Health,0.0f);
                TestTrue(TEXT("Death sealed"),Body->IsCombatDead());TestFalse(TEXT("Death cancels heal"),Heal->IsActive());
                if (!Advance(1.2f)) { return false; }Check(TEXT("Death no late heal or refund"),0.0f,ExpectedCharges);
            }
            else if (Scenario==8 || Scenario==9)
            {
                if (Scenario==9 && !Until([Heal](){return Heal->bCommitAttempted;},2.0f)) { return false; }
                Check(TEXT("Before EndPlay"),Scenario==9 ? 80.0f : 40.0f,Scenario==9 ? 2 : 3);
                if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
                FirstTask->OnMontageBlendingOut(Montage,false);FirstTask->OnMontageEnded(Montage,false);
                TestFalse(TEXT("EndPlay cannot resurrect ability"),Heal->IsActive());
                TestFalse(TEXT("EndPlay clears Healing tag"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));
                TestTrue(TEXT("Retired task detached"),FirstTask->Ability==nullptr);
                Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }continue;
            }
            else
            {
                FDelegateHandle CancelOnHealth;
                if (Scenario==3)
                {
                    if (!Advance(.20f)) { return false; }
                    APlayerState* Pauser=World->SpawnActor<APlayerState>();if (!TestNotNull(TEXT("Pauser"),Pauser)) { return false; }
                    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
                    const double FrozenTime=World->GetTimeSeconds();const float FrozenPosition=Anim->Montage_GetPosition(Montage);
                    const bool bPausedAdvance=Advance(.60f);
                    TestFalse(TEXT("Pause rejects request"),Body->RequestHeal());
                    Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token);
                    TestEqual(TEXT("Paused clock frozen"),World->GetTimeSeconds(),FrozenTime);
                    TestEqual(TEXT("Paused montage frozen"),Anim->Montage_GetPosition(Montage),FrozenPosition);
                    Check(TEXT("Paused"),40.0f,3);World->GetWorldSettings()->SetPauserPlayerState(nullptr);
                    if (!bPausedAdvance) { return false; }
                }
                if (Scenario==5)
                {
                    ASC->GameplayEffectApplicationQueries.Add(FGameplayEffectApplicationQuery::CreateLambda(
                        [](const FActiveGameplayEffectsContainer&,const FGameplayEffectSpec&){return false;}));
                }
                if (Scenario==10)
                {
                    const FGameplayAbilitySpecHandle HealHandle=Body->HealAbilityHandle;
                    CancelOnHealth=ASC->GetGameplayAttributeValueChangeDelegate(Health).AddLambda(
                        [ASC,HealHandle](const FOnAttributeChangeData& Change)
                        { if (Change.NewValue>Change.OldValue) { ASC->CancelAbilityHandle(HealHandle); } });
                }
                if (Scenario==11) { PC->UnPossess(); }
                if (Scenario==12) { ASC->SetNumericAttributeBase(Health,100.0f); }
                if (!Until([Heal](){return Heal->bCommitAttempted || !Heal->IsActive();},2.0f)) { return false; }
                if (Scenario==5 || Scenario==11 || Scenario==12)
                {
                    ASC->GameplayEffectApplicationQueries.Reset();
                    if (!Idle()) { return false; }
                    Check(TEXT("Rejected at commit"),Scenario==12 ? 100.0f : 40.0f,3);
                }
                else
                {
                    Check(TEXT("One real notify commit"),80.0f,2);
                    Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token);Check(TEXT("Duplicate ignored"),80.0f,2);
                    if (Scenario==0)
                    {
                        TestEqual(TEXT("No Stamina regeneration during Healing"),ASC->GetNumericAttribute(Stamina),75.0f);
                        TestFalse(TEXT("No recovery timer during Healing"),Recovery->IsRecoveryScheduled());
                    }
                    if (Scenario==2 && !TestTrue(TEXT("Postcommit hit accepted"),Body->RequestHitReact())) { return false; }
                    if (Scenario==10)
                    {
                        ASC->GetGameplayAttributeValueChangeDelegate(Health).Remove(CancelOnHealth);
                        TestFalse(TEXT("GE callback cancelled action"),Heal->IsActive());
                    }
                    if (!Idle()) { return false; }Check(TEXT("Completion or cancel no refund"),80.0f,2);
                    TestTrue(TEXT("Ended heal releases ASC montage ownership"),ASC->GetAnimatingAbility()!=Heal);
                    if (Scenario==0)
                    {
                        ASC->SetNumericAttributeBase(Health,40.0f);
                        if (!TestTrue(TEXT("A later independent heal can start"),Body->RequestHeal())) { return false; }
                        TestTrue(TEXT("New montage instance"),Heal->ActiveMontageInstanceID!=FirstID);
                        Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token);
                        FirstTask->OnMontageBlendingOut(Montage,false);FirstTask->OnMontageEnded(Montage,false);
                        Check(TEXT("Old notify / task cannot affect new activation"),40.0f,2);
                        TestTrue(TEXT("New activation keeps montage ownership"),Heal->IsActive() && ASC->GetAnimatingAbility()==Heal);
                        if (!Idle()) { return false; }Check(TEXT("New actual notify commits independently"),80.0f,1);
                    }
                }
            }
        }
        if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        TestFalse(TEXT("Final no Healing tag"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));
        TestFalse(TEXT("Final inactive / task released"),Heal->IsActive() || Heal->MontageTask!=nullptr);
        Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }
    }
    AddInfo(TEXT("[M2-04B2] Healing.Ability: actual saved montage / notify / GE; pre/post cancel, duplicate/old callbacks, pause, rejection, death, EndPlay, possession, synchronous cancel checked."));
    return !HasAnyErrors();
}
#endif