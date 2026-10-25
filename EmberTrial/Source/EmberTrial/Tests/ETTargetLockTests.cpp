#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetLockComponent.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Framework/ETPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTargetLockSelectionTest, "EmberTrial.M2.TargetLock.Selection",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETTargetLockSelectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto SpawnBody = [World, Spawn](FVector Location, EETCombatTeam Team)
    {
        AEmberTrialCharacter* Body =
            World->SpawnActor<AEmberTrialCharacter>(Location, FRotator::ZeroRotator, Spawn);
        if (Body)
        {
            Body->CombatTeam = Team;
            Body->GetCharacterMovement()->DisableMovement();
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
        }
        return Body;
    };
    AEmberTrialCharacter* Source = SpawnBody(FVector(0, 0, 200), EETCombatTeam::Player);
    AEmberTrialCharacter* Front = SpawnBody(FVector(600, 0, 200), EETCombatTeam::Enemy);
    AEmberTrialCharacter* Side = SpawnBody(FVector(350, 400, 200), EETCombatTeam::Enemy);
    AEmberTrialCharacter* Rear = SpawnBody(FVector(-800, 0, 200), EETCombatTeam::Enemy);
    AEmberTrialCharacter* Neutral = SpawnBody(FVector(250, 0, 200), EETCombatTeam::Neutral);
    AETPlayerController* PC = World->SpawnActor<AETPlayerController>();
    if (!TestNotNull(TEXT("Source"), Source) || !TestNotNull(TEXT("Front"), Front) ||
        !TestNotNull(TEXT("Side"), Side) || !TestNotNull(TEXT("Rear"), Rear) ||
        !TestNotNull(TEXT("Neutral"), Neutral) || !TestNotNull(TEXT("PC"), PC))
    {
        return false;
    }
    PC->Possess(Source);
    PC->SetControlRotation(FRotator::ZeroRotator);
    PC->SetViewTarget(Source);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    // 世界启动的PostInitializeComponents可能恢复默认移动模式。
    // 这个无地板夹具只验证锁定：启动后冻结初始身体，保留碰撞 / Timer / 生命周期。
    for (AEmberTrialCharacter* Body : {Source, Front, Side, Rear, Neutral})
    {
        Body->GetCharacterMovement()->DisableMovement();
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
    UETTargetLockComponent* Lock = PC->GetTargetLockComponent();
    if (!TestNotNull(TEXT("Native single component"), Lock) || !Advance(.05f))
    {
        return false;
    }
    TestTrue(TEXT("Native source is ready"), Source->IsGameplayReady());
    TestFalse(TEXT("Idle has no monitor timer"), Lock->MonitorTimer.IsValid());
    const FVector View(-400, 0, 200);
    TestTrue(TEXT("View weighting beats shorter distance; neutral excluded"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector) == Front);
    TestTrue(TEXT("Opposite view selects rear enemy"),
             Lock->FindBestTarget(Source, View, -FVector::ForwardVector) == Rear);
    AEmberTrialCharacter* Twin = World->SpawnActorDeferred<AEmberTrialCharacter>(
        AEmberTrialCharacter::StaticClass(), FTransform(FVector(600, 0, 200)), nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!TestNotNull(TEXT("Tie body"), Twin))
    {
        return false;
    }
    Twin->CombatTeam = EETCombatTeam::Enemy;
    Twin->FinishSpawning(FTransform(FVector(600, 0, 200)));
    Twin->GetCharacterMovement()->DisableMovement();
    Twin->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
    AEmberTrialCharacter* TieExpected =
        Front->GetPathName().Compare(Twin->GetPathName()) < 0 ? Front : Twin;
    TestTrue(TEXT("Equal score uses stable object path"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector) == TieExpected);
    Twin->Destroy();
    Front->SetGameplayReady(false);
    TestTrue(TEXT("Waiting body excluded"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector) == Side);
    Front->SetGameplayReady(true);
    Lock->BreakRadius = Lock->AcquireRadius - 1.0f;
    TestFalse(TEXT("Invalid hysteresis rejected"), Lock->ToggleTargetLock());
    Lock->BreakRadius = 1500.0f;
    if (!TestTrue(TEXT("Real toggle acquires"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    TestTrue(TEXT("Actual camera selects front"), Lock->GetLockedTarget() == Front);
    TestTrue(TEXT("One active target has monitor"),
             World->GetTimerManager().IsTimerActive(Lock->MonitorTimer));
    // ---- C：只扩已有Selection专项；离开本作用域先解绑，失败返回也不留下悬空Lambda ----
    {
        const FVector SavedSideLocation = Side->GetActorLocation();
        Side->SetActorLocation(FVector(3000, 400, 200));
        AEmberTrialCharacter* Left = SpawnBody(FVector(600, -300, 200), EETCombatTeam::Enemy);
        AEmberTrialCharacter* Right = SpawnBody(FVector(600, 300, 200), EETCombatTeam::Enemy);
        if (!TestNotNull(TEXT("Switch left"), Left) || !TestNotNull(TEXT("Switch right"), Right))
        {
            return false;
        }
        TestTrue(TEXT("Find screen left"),
                 Lock->FindSwitchTarget(Source, Front, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Left) == Left);
        TestTrue(TEXT("Find screen right"),
                 Lock->FindSwitchTarget(Source, Front, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Right) == Right);
        TestNull(TEXT("No wrap from right edge"),
                 Lock->FindSwitchTarget(Source, Right, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Right));
        Left->SetActorLocation(FVector(600, 0, 350));
        TestNull(TEXT("Same horizontal angle is not a left candidate"),
                 Lock->FindSwitchTarget(Source, Front, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Left));
        Left->SetActorLocation(FVector(600, -300, 200));

        int32 Changes = 0;
        AEmberTrialCharacter* LastPrevious = nullptr;
        AEmberTrialCharacter* LastCurrent = nullptr;
        bool bReentrantAccepted = false;
        const FDelegateHandle ChangedHandle = Lock->OnTargetChanged.AddLambda(
            [&](AEmberTrialCharacter* Previous, AEmberTrialCharacter* Current)
            {
                ++Changes;
                LastPrevious = Previous;
                LastCurrent = Current;
                bReentrantAccepted |= Lock->SwitchTarget(EETTargetLockSwitchDirection::Left);
            });
        ON_SCOPE_EXIT
        {
            Lock->OnTargetChanged.Remove(ChangedHandle);
        };

        const uint64 BeforeSwitchSerial = Lock->LockSerial;
        TestTrue(TEXT("Real right request accepted"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));
        TestTrue(TEXT("One direct old to new event"),
                 Changes == 1 && LastPrevious == Front && LastCurrent == Right);
        TestFalse(TEXT("Broadcast cannot reenter switch"), bReentrantAccepted);
        TestTrue(TEXT("One new target monitor"),
                 World->GetTimerManager().IsTimerActive(Lock->MonitorTimer));
        TestFalse(TEXT("Both directions share throttle"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        Lock->MaintainLock(BeforeSwitchSerial);
        Front->SetGameplayReady(false);
        TestTrue(TEXT("Old serial and old Ready cannot clear new target"),
                 Lock->GetLockedTarget() == Right);
        Front->SetGameplayReady(true);
        if (!Advance(.21f))
        {
            return false;
        }
        TestTrue(TEXT("Left chooses nearest intermediate target"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        TestTrue(TEXT("Nearest is original front"), Lock->GetLockedTarget() == Front);
        if (!Advance(.21f))
        {
            return false;
        }
        Right->SetGameplayReady(false);
        const int32 BeforeRejectedChanges = Changes;
        TestFalse(TEXT("No candidate keeps current"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));
        TestTrue(TEXT("Failure has no None event"),
                 Lock->GetLockedTarget() == Front && Changes == BeforeRejectedChanges);
        Right->SetGameplayReady(true);
        TestFalse(TEXT("Failed search also throttles"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));
        if (!Advance(.21f))
        {
            return false;
        }
        TestTrue(TEXT("Rejected input was not queued"), Lock->GetLockedTarget() == Front);
        TestTrue(TEXT("Fresh request switches"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));

        APlayerState* SwitchPauser = World->SpawnActor<APlayerState>();
        if (!TestNotNull(TEXT("Switch pauser"), SwitchPauser))
        {
            return false;
        }
        World->GetWorldSettings()->SetPauserPlayerState(SwitchPauser);
        const double Deadline = Lock->NextSwitchAllowedAt;
        TestFalse(TEXT("Pause rejects switch"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        if (!Advance(.25f))
        {
            return false;
        }
        TestTrue(TEXT("Pause does not alter switch deadline"),
                 Lock->NextSwitchAllowedAt == Deadline);
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        TestFalse(TEXT("Pause did not spend switch interval"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        if (!Advance(.21f))
        {
            return false;
        }
        TestTrue(TEXT("Resume does not replay request"), Lock->GetLockedTarget() == Right);
        TestTrue(TEXT("Fresh left restores original target"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        TestTrue(TEXT("Front restored"), Lock->GetLockedTarget() == Front);
        Left->Destroy();
        Right->Destroy();
        TestTrue(TEXT("Previous target EndPlay cannot clear current"),
                 Lock->GetLockedTarget() == Front);
        Side->SetActorLocation(SavedSideLocation);
    }
    TestTrue(TEXT("Second toggle releases"), Lock->ToggleTargetLock());
    TestNull(TEXT("Released target"), Lock->GetLockedTarget());
    TestFalse(TEXT("Release stops timer"), Lock->MonitorTimer.IsValid());
    if (!TestTrue(TEXT("Acquire before unpossess"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    PC->UnPossess();
    TestNull(TEXT("Unpossess releases immediately"), Lock->GetLockedTarget());
    PC->Possess(Source);
    Source->GetCharacterMovement()->DisableMovement();
    TestNull(TEXT("Repossess does not inherit a lock"), Lock->GetLockedTarget());
    if (!Lock->ToggleTargetLock())
    {
        return false;
    }
    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Wall"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(20, 1500, 1500));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(200, 0, 200));
    TestNull(TEXT("Acquisition never selects through a blocking wall"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector));
    if (!Advance(.20f))
    {
        return false;
    }
    TestTrue(TEXT("Brief occlusion retains identity"), Lock->GetLockedTarget() == Front);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestFalse(TEXT("Paused request is rejected"), Lock->ToggleTargetLock());
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("Pause does not spend occlusion grace"), Lock->GetLockedTarget() == Front);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!Advance(.5f))
    {
        return false;
    }
    TestNull(TEXT("Sustained occlusion releases"), Lock->GetLockedTarget());
    TestFalse(TEXT("Occlusion stops monitor"), Lock->MonitorTimer.IsValid());
    Wall->Destroy();
    if (!Advance(.05f))
    {
        return false;
    }
    TestTrue(TEXT("Stationary fixture target keeps its spawn position"),
             Front->GetActorLocation().Equals(FVector(600, 0, 200), .01));
    if (!TestTrue(TEXT("Acquire after wall removal"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    Front->GetAbilitySystemComponent()->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestNull(TEXT("Target death releases immediately"), Lock->GetLockedTarget());
    if (!TestTrue(TEXT("Dead front excluded; side acquired"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    TestTrue(TEXT("Remaining enemy selected"), Lock->GetLockedTarget() == Side);
    const uint64 OldSerial = Lock->LockSerial;
    Side->Destroy();
    TestNull(TEXT("Target EndPlay releases"), Lock->GetLockedTarget());
    // 销毁后不再读Side / 旧ASC；只检查新身份是否受旧代次影响。
    Rear->SetActorLocation(FVector(600, 0, 200));
    if (!TestTrue(TEXT("Another living enemy acquired"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    Lock->MaintainLock(OldSerial);
    TestTrue(TEXT("Stale monitor cannot clear new target"), Lock->GetLockedTarget() == Rear);
    Rear->SetActorLocation(FVector(2000, 0, 200));
    if (!Advance(.20f))
    {
        return false;
    }
    TestNull(TEXT("Break radius releases"), Lock->GetLockedTarget());
    Rear->SetActorLocation(FVector(600, 0, 200));
    if (!Lock->ToggleTargetLock())
    {
        return false;
    }
    Lock->Deactivate();
    TestNull(TEXT("Deactivate releases"), Lock->GetLockedTarget());
    TestFalse(TEXT("Inactive cannot reacquire"), Lock->ToggleTargetLock());
    Lock->Activate(false);
    if (!Lock->ToggleTargetLock())
    {
        return false;
    }
    Source->GetAbilitySystemComponent()->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestNull(TEXT("Source death releases"), Lock->GetLockedTarget());
    TestFalse(TEXT("Dead source cannot reacquire"), Lock->ToggleTargetLock());
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay clears timer"), Lock->MonitorTimer.IsValid());
    return !HasAnyErrors();
}
#endif