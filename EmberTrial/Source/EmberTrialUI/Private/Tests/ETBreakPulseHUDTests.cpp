#if WITH_DEV_AUTOMATION_TESTS
#include "Presenters/ETBreakPulsePresenter.h"
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETBreakPulseHUDTest, "EmberTrial.UI.BreakPulse.Lifecycle",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETBreakPulseHUDTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* PulseClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved pulse"), PulseClass) || !TestNotNull(TEXT("Quinn"), Quinn) ||
        !TestNotNull(TEXT("Engine for LocalPlayer outer"), GEngine))
    {
        return false;
    }
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AActor* Floor = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Floor"), Floor))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(2000, 2000, 20));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body =
        World->SpawnActor<AEmberTrialCharacter>(FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("PC"), PC))
    {
        return false;
    }
    auto Configure = [PulseClass, Quinn](AEmberTrialCharacter* Target)
    {
        Target->CombatTeam = EETCombatTeam::Player;
        Target->BreakPulseAbilityClass = PulseClass;
        Target->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Target->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Target->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Target->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Target->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Target->GetCharacterMovement()->bRunPhysicsWithNoController = true;
    };
    Configure(Body);
    // 只构造身份 / 观察夹具，不注册到真实GameInstance，不创建Viewport或屏幕Widget。
    TStrongObjectPtr<ULocalPlayer> LocalPlayer(NewObject<ULocalPlayer>(GEngine));
    LocalPlayer->SwitchController(PC);
    PC->Possess(Body);
    TStrongObjectPtr<UETUILocalPlayerSubsystem> Subsystem(
        NewObject<UETUILocalPlayerSubsystem>(LocalPlayer.Get()));
    Subsystem->PlayerControllerChanged(PC);
    TStrongObjectPtr<UETBreakPulsePresenter> Presenter(NewObject<UETBreakPulsePresenter>());
    Presenter->StartObserving(Subsystem.Get());
    Presenter->SetPresentationEnabled(true);
    TestTrue(TEXT("Before BeginPlay is waiting"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::WaitingForGameplayReady);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    auto Advance = [&Fixture](float Seconds)
    {
        for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
        {
            if (!Fixture.TickTestWorld(.01f))
            {
                return false;
            }
        }
        return true;
    };
    Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("Ready callback waits for final ability grant"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Ready);
    TestFalse(TEXT("Idle has no periodic timer"), Presenter->PeriodicRefreshTimer.IsValid());
    TestEqual(TEXT("Idle recharge fraction"), Presenter->GetSnapshot().ReadyFraction, 1.0f);
    if (!TestTrue(TEXT("Real F begins"), Body->RequestBreakPulse()) || !Advance(.32f))
    {
        return false;
    }
    TestTrue(TEXT("Cooling observed"), Presenter->GetSnapshot().bCoolingDown);
    TestTrue(TEXT("Casting observed"), Presenter->GetSnapshot().bCasting);
    TestTrue(TEXT("Visible cooling starts periodic refresh"),
             World->GetTimerManager().IsTimerActive(Presenter->PeriodicRefreshTimer));
    const float BeforePause = Presenter->GetSnapshot().Remaining;
    APlayerState* Pauser = PC->PlayerState;
    if (!IsValid(Pauser))
    {
        Pauser = World->SpawnActor<APlayerState>();
    }
    if (!TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("Pause freezes countdown"),
             FMath::IsNearlyEqual(Presenter->GetSnapshot().Remaining, BeforePause, .001f));
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!Advance(.3f))
    {
        return false;
    }
    Presenter->SetPresentationEnabled(false);
    TestFalse(TEXT("Hidden stops periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    const float HiddenRemaining = Presenter->GetSnapshot().Remaining;
    if (!Advance(.6f))
    {
        return false;
    }
    Presenter->SetPresentationEnabled(true);
    TestTrue(TEXT("Late visible snapshot catches actual GAS time"),
             Presenter->GetSnapshot().Remaining < HiddenRemaining);
    const FTimerHandle PreviousTimer = Presenter->PeriodicRefreshTimer;
    const uint64 PreviousSerial = Presenter->ObservationSerial;
    Presenter->StartObserving(Subsystem.Get());
    TestFalse(TEXT("Restart retires previous timer"),
              World->GetTimerManager().TimerExists(PreviousTimer));
    const FTimerHandle NewTimer = Presenter->PeriodicRefreshTimer;
    Presenter->HandleDeferredRefresh(PreviousSerial);
    Presenter->HandleTagChanged(ETGameplayTags::Cooldown_BreakPulse, 0,
                                Body->GetAbilitySystemComponent(), PreviousSerial);
    TestTrue(TEXT("Old callbacks do not replace new timer"),
             Presenter->PeriodicRefreshTimer == NewTimer);
    UAbilitySystemComponent* OldASC = Body->GetAbilitySystemComponent();
    PC->UnPossess();
    TestTrue(TEXT("Unpossess publishes NoPawn"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::NoPawn);
    TestFalse(TEXT("NoPawn stops timer"), Presenter->PeriodicRefreshTimer.IsValid());
    AEmberTrialCharacter* NewBody = World->SpawnActorDeferred<AEmberTrialCharacter>(
        AEmberTrialCharacter::StaticClass(), FTransform(FVector(500, 0, 200)), nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!TestNotNull(TEXT("New body"), NewBody))
    {
        return false;
    }
    Configure(NewBody);
    NewBody->FinishSpawning(FTransform(FVector(500, 0, 200)));
    PC->Possess(NewBody);
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("New body ready"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Ready);
    TestFalse(TEXT("New body does not inherit old cooldown"),
              Presenter->GetSnapshot().bCoolingDown);
    OldASC->AddLooseGameplayTag(ETGameplayTags::State_CastingBreakPulse);
    if (!Advance(.02f))
    {
        return false;
    }
    TestTrue(TEXT("Old body event stays isolated"),
             Presenter->GetSnapshot().PawnName == NewBody->GetName() &&
                 !Presenter->GetSnapshot().bCasting);
    OldASC->RemoveLooseGameplayTag(ETGameplayTags::State_CastingBreakPulse);
    if (!TestTrue(TEXT("New body's F accepted"), NewBody->RequestBreakPulse()) || !Advance(.02f))
    {
        return false;
    }
    NewBody->GetAbilitySystemComponent()->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    if (!Advance(.02f))
    {
        return false;
    }
    TestTrue(TEXT("Death displayed distinctly"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Dead);
    TestFalse(TEXT("Death stops periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    PC->Possess(Body);
    if (!Advance(.02f))
    {
        return false;
    }
    TestTrue(TEXT("Same old body retains its cooldown"), Presenter->GetSnapshot().bCoolingDown);
    if (!Advance(7.0f))
    {
        return false;
    }
    TestFalse(TEXT("Natural expiry observed"), Presenter->GetSnapshot().bCoolingDown);
    TestFalse(TEXT("Expiry stops periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    if (!TestTrue(TEXT("Final F for EndPlay"), Body->RequestBreakPulse()) || !Advance(.02f))
    {
        return false;
    }
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay clears periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    TestFalse(TEXT("EndPlay clears deferred refresh"), Presenter->DeferredRefreshTimer.IsValid());
    Presenter->Shutdown();
    TestTrue(TEXT("Shutdown snapshot unavailable"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Unavailable);
    Subsystem->PlayerControllerChanged(nullptr);
    PC->Player = nullptr;
    LocalPlayer->PlayerController = nullptr;
    return !HasAnyErrors();
}
#endif