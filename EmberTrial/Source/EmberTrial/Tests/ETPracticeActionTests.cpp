#if WITH_DEV_AUTOMATION_TESTS

#include "ETPracticeActionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "GameFramework/RootMotionSource.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETPracticeActionLifecycleTest,
                                 "EmberTrial.M0.PracticeAction.Lifecycle",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETPracticeActionLifecycleTest::RunTest(const FString& Parameters)
{
    // ---- 瞬态角色与保存Montage；只测试兼容组件，不接回正式GAS输入 ----
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    ACharacter* Character = World->SpawnActor<ACharacter>();
    if (!TestNotNull(TEXT("Temporary character"), Character))
        return false;
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Real project skeletal mesh"), Quinn))
        return false;
    Mesh->SetSkeletalMeshAsset(Quinn);
    Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Mesh->VisibilityBasedAnimTickOption =
        EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

    UETPracticeActionComponent* Component = NewObject<UETPracticeActionComponent>(Character);
    Character->AddInstanceComponent(Component);
    Component->RegisterComponent();
    Component->AttackMontage =
        LoadObject<UAnimMontage>(nullptr, TEXT("/Game/EmberTrial/Animations/Practice/Montages/"
                                               "AM_M0_Attack_01_Montage.AM_M0_Attack_01_Montage"));
    Component->DodgeMontage =
        LoadObject<UAnimMontage>(nullptr, TEXT("/Game/EmberTrial/Animations/Practice/Montages/"
                                               "AM_M0_Dodge_01_Montage.AM_M0_Dodge_01_Montage"));
    Component->HitReactMontage = LoadObject<UAnimMontage>(
        nullptr, TEXT("/Game/EmberTrial/Animations/Practice/Montages/"
                      "AM_M0_HitResult_01_Montage.AM_M0_HitResult_01_Montage"));
    if (!TestNotNull(TEXT("Attack asset"), Component->AttackMontage.Get()) ||
        !TestNotNull(TEXT("Dodge asset"), Component->DodgeMontage.Get()) ||
        !TestNotNull(TEXT("Hit asset"), Component->HitReactMontage.Get()))
        return false;
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    // Isolate animation/state lifecycle from terrain; C2-03 tests real displacement.
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    Movement->SetComponentTickEnabled(false);
    Movement->SetMovementMode(MOVE_Walking);
    // 生命周期夹具关闭Movement Tick；.02秒动画推进及原错误转发保持。
    auto Advance = [&TestWorld](float Seconds)
    {
        for (int32 Frame = 0; Frame < FMath::CeilToInt(Seconds / 0.02f); ++Frame)
        {
            TestWorld.TickTestWorld(0.02f);
        }
    };
    // 别名限定在本RunTest，不能让其他测试依赖此函数的局部作用域。
    using State = EETPracticeActionState;
    TestTrue(TEXT("Initial Idle"), Component->IsPracticeActionIdle());
    TestFalse(TEXT("Idle tick disabled"), Component->IsComponentTickEnabled());
    TestFalse(TEXT("Idle is not a playable request"),
              Component->RequestPracticeAction(State::Idle));
    Movement->Velocity = FVector(100, 0, 0);
    TestFalse(TEXT("Moving attack refused"), Component->RequestPracticeAction(State::Attacking));
    Movement->Velocity = FVector::ZeroVector;
    Movement->SetMovementMode(MOVE_Falling);
    for (State Action : {State::Attacking, State::Dodging, State::HitReact})
    {
        TestFalse(TEXT("Airborne request refused"), Component->RequestPracticeAction(Action));
    }
    Movement->SetMovementMode(MOVE_Walking);

    // Guard regressions: these requests must fail before any animation or motion starts.
    const float SavedDistance = Component->DodgeDistance;
    const float SavedDuration = Component->DodgeDuration;
    // ---- 八项独立非法配置逐项断言；失败也保留恢复 / 取消保护 ----
    struct FInvalidDodgeCase
    {
        const TCHAR* Name;
        float Distance;
        float Duration;
    };
    const FInvalidDodgeCase InvalidCases[] = {
        {TEXT("Zero distance"), 0.0f, 0.30f},
        {TEXT("Negative distance"), -1.0f, 0.30f},
        {TEXT("Too large distance"), 301.0f, 0.30f},
        {TEXT("NaN distance"), std::numeric_limits<float>::quiet_NaN(), 0.30f},
        {TEXT("Zero duration"), 150.0f, 0.0f},
        {TEXT("Negative duration"), 150.0f, -0.10f},
        {TEXT("Too long duration"), 150.0f, 0.60f},
        {TEXT("Infinite duration"), 150.0f, std::numeric_limits<float>::infinity()}};
    for (const FInvalidDodgeCase& Case : InvalidCases)
    {
        Component->DodgeDistance = Case.Distance;
        Component->DodgeDuration = Case.Duration;
        const bool bStarted = Component->RequestPracticeAction(State::Dodging);
        TestFalse(FString::Printf(TEXT("%s refused"), Case.Name), bStarted);
        TestTrue(FString::Printf(TEXT("%s leaves Idle"), Case.Name),
                 Component->IsPracticeActionIdle());
        TestFalse(FString::Printf(TEXT("%s starts no montage"), Case.Name),
                  Mesh->GetAnimInstance()->IsAnyMontagePlaying());
        TestFalse(FString::Printf(TEXT("%s creates no dodge source"), Case.Name),
                  Component->DodgeSource.IsValid());
        // Avoid advancing an invalid source if this regression ever fails.
        if (bStarted || !Component->IsPracticeActionIdle() || Component->DodgeSource.IsValid())
        {
            Component->CancelPracticeAction();
            Component->DodgeDistance = SavedDistance;
            Component->DodgeDuration = SavedDuration;
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
    }
    Component->DodgeDistance = SavedDistance;
    Component->DodgeDuration = SavedDuration;

    // A foreign pending source must also block a new dodge.
    // 外部RMS不能被兼容组件取消；仅夹具自己按ID删除并清理待移除源。
    TSharedPtr<FRootMotionSource_ConstantForce> ForeignSource =
        MakeShared<FRootMotionSource_ConstantForce>();
    ForeignSource->InstanceName = FName(TEXT("M0GuardTestForeignSource"));
    ForeignSource->AccumulateMode = ERootMotionAccumulateMode::Override;
    ForeignSource->Force = FVector(10.0f, 0.0f, 0.0f);
    ForeignSource->Duration = 1.0f;
    const uint16 ForeignID = Movement->ApplyRootMotionSource(ForeignSource);
    if (!TestTrue(TEXT("Foreign source registered"),
                  ForeignID != static_cast<uint16>(ERootMotionSourceID::Invalid)))
    {
        return false;
    }
    TestFalse(TEXT("Foreign root motion blocks dodge"),
              Component->RequestPracticeAction(State::Dodging));
    TestTrue(TEXT("Foreign source rejection leaves Idle"), Component->IsPracticeActionIdle());
    TestFalse(TEXT("Foreign source rejection starts no montage"),
              Mesh->GetAnimInstance()->IsAnyMontagePlaying());
    TestFalse(TEXT("Foreign source rejection creates no owned source"),
              Component->DodgeSource.IsValid());
    Component->CancelPracticeAction();
    TestTrue(TEXT("Cancellation preserves foreign source"),
             Movement->GetRootMotionSourceByID(ForeignID).Get() == ForeignSource.Get() &&
                 !ForeignSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));

    // This fixture disables movement ticking, so explicitly drain its marked sources.
    Movement->RemoveRootMotionSourceByID(ForeignID);
    Movement->CurrentRootMotion.CleanUpInvalidRootMotion(0.0f, *Character, *Movement);
    TestFalse(TEXT("Guard fixture leaves no root motion sources"),
              Movement->HasRootMotionSources());
    if (HasAnyErrors())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }

    if (!TestTrue(TEXT("Grounded attack accepted"),
                  Component->RequestPracticeAction(State::Attacking)))
        return false;
    TestTrue(TEXT("Busy tick enabled"), Component->IsComponentTickEnabled());
    TestFalse(TEXT("Repeated attack refused"), Component->RequestPracticeAction(State::Attacking));
    TestFalse(TEXT("Dodge cannot interrupt attack"),
              Component->RequestPracticeAction(State::Dodging));
    UAnimInstance* Anim = Mesh->GetAnimInstance();
    FAnimMontageInstance* OldInstance =
        Anim->GetMontageInstanceForID(Component->ActiveMontageInstanceID);
    if (!TestNotNull(TEXT("Owned montage instance"), OldInstance))
        return false;
    // Keep a real delegate copy, as UE can do when queueing montage end events.
    // ---- 保存真实委托副本，仅模拟迟到旧事件，不伪造成功动画结束 ----
    FOnMontageEnded StaleEnd = OldInstance->OnMontageEnded;
    if (!TestTrue(TEXT("Hit interrupts attack"), Component->RequestPracticeAction(State::HitReact)))
        return false;
    StaleEnd.ExecuteIfBound(Component->AttackMontage, true);
    TestTrue(TEXT("Old callback cannot release new hit"),
             Component->CurrentState == State::HitReact);
    for (State Action : {State::Attacking, State::Dodging, State::HitReact})
    {
        TestFalse(TEXT("Hit cannot be restarted or interrupted"),
                  Component->RequestPracticeAction(Action));
    }
    Advance(1.0f);
    TestTrue(TEXT("Natural montage end releases state"), Component->IsPracticeActionIdle());
    TestFalse(TEXT("Natural end disables safety tick"), Component->IsComponentTickEnabled());

    Movement->Velocity = FVector(200, 0, 0);
    TestTrue(TEXT("Dodge accepts ground movement"),
             Component->RequestPracticeAction(State::Dodging));
    TestTrue(TEXT("Starting action clears old velocity"), Movement->Velocity.IsNearlyZero());
    TestTrue(TEXT("Hit interrupts dodge"), Component->RequestPracticeAction(State::HitReact));
    Advance(1.0f);

    TestTrue(TEXT("Attack before pause"), Component->RequestPracticeAction(State::Attacking));
    Advance(0.1f);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    const float PausedPosition = Anim->Montage_GetPosition(Component->AttackMontage);
    TestFalse(TEXT("Paused hit refused"), Component->RequestPracticeAction(State::HitReact));
    Advance(0.4f);
    TestEqual(TEXT("Paused montage does not advance"),
              Anim->Montage_GetPosition(Component->AttackMontage), PausedPosition);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    Advance(1.5f);
    TestTrue(TEXT("Resume finishes original action"), Component->IsPracticeActionIdle());

    UAnimMontage* SavedHit = Component->HitReactMontage;
    Component->HitReactMontage = nullptr;
    TestTrue(TEXT("Attack before invalid hit config"),
             Component->RequestPracticeAction(State::Attacking));
    TestFalse(TEXT("Missing hit asset refused"), Component->RequestPracticeAction(State::HitReact));
    TestTrue(TEXT("Missing asset does not cancel existing action"),
             Component->CurrentState == State::Attacking);
    Component->HitReactMontage = SavedHit;
    Component->CancelPracticeAction();
    Component->CancelPracticeAction();
    TestTrue(TEXT("Repeated cancellation is safe"), Component->IsPracticeActionIdle());
    Advance(0.2f);

    // ---- 播放失败回滚、外部动画归属与动画实例替换分别验证 ----
    UAnimMontage* SavedAttack = Component->AttackMontage;
    Component->AttackMontage = NewObject<UAnimMontage>();
    Component->AttackMontage->SetSkeleton(SavedAttack->GetSkeleton());
    TestFalse(TEXT("Zero-length montage playback fails"),
              Component->RequestPracticeAction(State::Attacking));
    TestTrue(TEXT("Playback failure rolls back state"), Component->IsPracticeActionIdle());
    TestFalse(TEXT("Playback failure leaves no tick"), Component->IsComponentTickEnabled());
    Component->AttackMontage = SavedAttack;

    TestTrue(TEXT("External montage starts"), Anim->Montage_Play(SavedAttack) > 0.0f);
    TestFalse(TEXT("Unowned montage is not stolen"),
              Component->RequestPracticeAction(State::Dodging));
    Component->CancelPracticeAction();
    TestTrue(TEXT("Idle cancellation preserves external montage"),
             Anim->Montage_IsPlaying(SavedAttack));
    Advance(1.5f);

    TestTrue(TEXT("Attack before external interruption"),
             Component->RequestPracticeAction(State::Attacking));
    Anim->Montage_Stop(0.05f, SavedAttack);
    Advance(0.2f);
    TestTrue(TEXT("External stop releases state"), Component->IsPracticeActionIdle());

    TestTrue(TEXT("Attack before AnimInstance replacement"),
             Component->RequestPracticeAction(State::Attacking));
    Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Advance(0.1f);
    TestTrue(TEXT("AnimInstance replacement cannot leave action stuck"),
             Component->IsPracticeActionIdle());
    Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
    // ---- 组件失活 / 重激活与Owner结束独立覆盖，不能删掉兼容入口 ----
    TestTrue(TEXT("Attack before deactivation"),
             Component->RequestPracticeAction(State::Attacking));
    Component->Deactivate();
    TestTrue(TEXT("Deactivation releases state"), Component->IsPracticeActionIdle());
    TestFalse(TEXT("Inactive component refuses requests"),
              Component->RequestPracticeAction(State::Attacking));
    Component->Activate();
    Advance(0.2f);
    TestTrue(TEXT("Attack after reactivation"), Component->RequestPracticeAction(State::Attacking));
    Character->Destroy();
    TestTrue(TEXT("Owner EndPlay releases state"), Component->IsPracticeActionIdle());
    TestFalse(TEXT("Ended component refuses requests"),
              Component->RequestPracticeAction(State::Attacking));
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
