#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETDodgeMovementTest, "EmberTrial.M2.Dodge.Movement",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETDodgeMovementTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    // 归一化方向的原绝对容差；不与秒数 / 厘米误差共用同一语义常量。
    constexpr float DirectionTolerance = 0.0001f;
    // Direction conversion is independent of this frame's Move callback ordering.
    for (const FVector2D Axis :
         {FVector2D(0, 1), FVector2D(0, -1), FVector2D(1, 0), FVector2D(-1, 0), FVector2D(1, 1),
          FVector2D(-1, 1), FVector2D(1, -1), FVector2D(-1, -1)})
    {
        const FVector Expected = FVector(Axis.Y, Axis.X, 0.0f).GetSafeNormal2D();
        const FVector Result =
            AEmberTrialCharacter::MakeDodgeWorldDirection(Axis, 0.0f, FVector::ForwardVector);
        TestTrue(TEXT("Eight input directions normalize correctly"),
                 Result.Equals(Expected, DirectionTolerance));
        const FVector Rotated =
            AEmberTrialCharacter::MakeDodgeWorldDirection(Axis, 90.0f, FVector::ForwardVector);
        TestTrue(TEXT("Control yaw rotates input into world space"),
                 Rotated.Equals(FRotator(0, 90, 0).RotateVector(Expected), DirectionTolerance));
    }
    TestTrue(TEXT("No input uses body forward, not camera forward"),
             AEmberTrialCharacter::MakeDodgeWorldDirection(FVector2D::ZeroVector, 90.0f,
                                                           FVector::ForwardVector)
                 .Equals(FVector::ForwardVector, DirectionTolerance));
    TestTrue(TEXT("Default movement configuration"),
             UETAbilityTask_DodgeMove::IsConfigurationValid(150, 0.30f));
    TestFalse(TEXT("Zero duration rejected"),
              UETAbilityTask_DodgeMove::IsConfigurationValid(150, 0));
    TestFalse(TEXT("Excessive distance rejected"),
              UETAbilityTask_DodgeMove::IsConfigurationValid(301, 0.30f));

    // ---- 保存动画 / GE资源；只装配瞬态角色与能力实例 ----
    UAnimMontage* Montage =
        LoadObject<UAnimMontage>(nullptr, TEXT("/Game/EmberTrial/Animations/Practice/Montages/"
                                               "AM_M0_Dodge_01_Montage.AM_M0_Dodge_01_Montage"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UClass* CostClass = LoadClass<UGameplayEffect>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost.GE_ET_DodgeCost_C"));
    if (!TestNotNull(TEXT("Saved dodge montage"), Montage) || !TestNotNull(TEXT("Quinn"), Quinn) ||
        !TestNotNull(TEXT("Saved formal cost GE"), CostClass))
    {
        return false;
    }

    // 固定30 / 60 / 120步长验证物理积分，不代表实测游戏FPS。
    for (const int32 Rate : {30, 60, 120})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        // 地面 / 墙走真实查询与物理碰撞，不能换成直接设置期望终点。
        auto Block = [World](const FVector& Center, const FVector& HalfSize)
        {
            AActor* Actor = World->SpawnActor<AActor>();
            if (!Actor)
            {
                return Actor;
            }
            UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
            Actor->AddInstanceComponent(Box);
            Actor->SetRootComponent(Box);
            Box->SetMobility(EComponentMobility::Static);
            Box->SetBoxExtent(HalfSize);
            Box->SetRelativeLocation(Center);
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->RegisterComponent();
            return Actor;
        };
        if (!TestNotNull(TEXT("Real floor"), Block(FVector(0, 0, -50), FVector(2000, 2000, 50))))
        {
            return false;
        }
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Native body"), Body) ||
            !TestNotNull(TEXT("Standalone controller"), PC))
        {
            return false;
        }
        Body->CombatTeam = EETCombatTeam::Player; // Transient fixture only; no asset/CDO writes.
        USkeletalMeshComponent* Mesh = Body->GetMesh();
        Mesh->SetSkeletalMeshAsset(Quinn);
        Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        PC->Possess(Body);
        UCharacterMovementComponent* Move = Body->GetCharacterMovement();
        Move->bRunPhysicsWithNoController = true;
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        // RoundToInt与至少一帧的原推进规则保留，时间误差边界不放宽。
        const float Step = 1.0f / static_cast<float>(Rate);
        auto Advance = [&Fixture, Step, Rate](float Seconds)
        {
            const int32 Count = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));
            for (int32 Index = 0; Index < Count; ++Index)
            {
                if (!Fixture.TickTestWorld(Step))
                {
                    return false;
                }
            }
            return true;
        };
        Move->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Landed on floor"), Move->IsMovingOnGround()) ||
            !TestTrue(TEXT("Ready and possessed"),
                      Body->IsGameplayReady() && PC->GetPawn() == Body))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        const FGameplayAbilitySpecHandle Handle =
            ASC->GiveAbility(FGameplayAbilitySpec(UETGA_Dodge::StaticClass(), 1));
        FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
        UETGA_Dodge* Ability = Spec ? Cast<UETGA_Dodge>(Spec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Granted native ability instance"), Ability))
        {
            return false;
        }
        Ability->DodgeMontage = Montage;
        Ability->CostGameplayEffectClass = CostClass;
        const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
        auto Refill = [ASC, Stamina]() { ASC->SetNumericAttributeBase(Stamina, 100.0f); };
        auto Start = [this, ASC, Handle, Ability]()
        {
            return TestTrue(TEXT("Full GAS activation"), ASC->TryActivateAbility(Handle, false)) &&
                   TestTrue(TEXT("Ability and owned movement are active"),
                            Ability->IsActive() && IsValid(Ability->MoveTask) &&
                                Ability->MoveTask->IsMovementReady());
        };
        // ---- 完整GAS激活与一次扣费；转身后仍沿已缓存的方向移动 ----
        const FVector StartAt = Body->GetActorLocation();
        const FVector Direction = Body->GetActorForwardVector().GetSafeNormal2D();
        if (!Start())
        {
            return false;
        }
        TSharedPtr<FRootMotionSource> Source = Ability->MoveTask->Source;
        TestEqual(TEXT("Full activation pays once"), ASC->GetNumericAttribute(Stamina), 75.0f);
        TestFalse(TEXT("Busy duplicate is rejected"), ASC->TryActivateAbility(Handle, false));
        TestEqual(TEXT("Busy duplicate pays nothing"), ASC->GetNumericAttribute(Stamina), 75.0f);
        if (!Advance(0.1f))
        {
            return false;
        }
        PC->SetControlRotation(FRotator(0, 90, 0));
        Body->SetActorRotation(FRotator(0, 90, 0));
        if (!Advance(0.3f))
        {
            return false;
        }
        const FVector StoppedAt = Body->GetActorLocation();
        TestTrue(TEXT("Endpoint follows cached forward within 1 cm"),
                 FVector::Dist2D(StoppedAt, StartAt + Direction * 150.0f) <= 1.0f);
        TestTrue(TEXT("Source time is .30 seconds"),
                 FMath::IsNearlyEqual(Source->GetTime(), 0.30f, 0.002f));
        TestTrue(TEXT("Movement completes before montage action"),
                 Ability->IsActive() && !Ability->MoveTask &&
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        TestEqual(TEXT("Recovery remains blocked after movement ends"),
                  ASC->GetNumericAttribute(Stamina), 75.0f);
        if (!Advance(0.6f))
        {
            return false;
        }
        TestFalse(TEXT("Montage later ends ability"), Ability->IsActive());
        TestFalse(TEXT("No leftover tag"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        TestFalse(TEXT("No leftover source"), Move->HasRootMotionSources());
        TestTrue(TEXT("No drift"), FVector::Dist2D(StoppedAt, Body->GetActorLocation()) <= 1.0f &&
                                       Move->Velocity.Size2D() <= 1.0f);
        AddInfo(FString::Printf(TEXT("[M2-02B] Rate=%d Travel2D=%.2f SourceTime=%.4f"), Rate,
                                FVector::Dist2D(StartAt, StoppedAt), Source->GetTime()));

        // 60步长承担配置 / 外部RMS / 中断 / 碰撞 / 暂停 / 死亡窄场景。
        if (Rate == 60)
        {
            Refill();
            Ability->DodgeDuration = 0.0f;
            TestFalse(TEXT("Invalid config rejects before payment"),
                      ASC->TryActivateAbility(Handle, false));
            TestEqual(TEXT("Invalid config is free"), ASC->GetNumericAttribute(Stamina), 100.0f);
            Ability->DodgeDuration = 0.30f;
            // 外部RMS必须保留；测试自行删除自己的夹具源并推进一帧。
            TSharedPtr<FRootMotionSource_ConstantForce> Foreign =
                MakeShared<FRootMotionSource_ConstantForce>();
            Foreign->InstanceName = FName(TEXT("ETDodgeForeignFixture"));
            Foreign->Duration = 1.0f;
            Foreign->Force = FVector::ZeroVector;
            const uint16 ForeignID = Move->ApplyRootMotionSource(Foreign);
            TestFalse(TEXT("Foreign source rejects before payment"),
                      ASC->TryActivateAbility(Handle, false));
            TestTrue(TEXT("Foreign source is not removed"),
                     Move->GetRootMotionSourceByID(ForeignID) == Foreign);
            TestEqual(TEXT("Foreign source rejection is free"), ASC->GetNumericAttribute(Stamina),
                      100.0f);
            Move->RemoveRootMotionSourceByID(ForeignID);
            if (!Advance(Step))
            {
                return false;
            }

            if (!Start() || !Advance(0.1f))
            {
                return false;
            }
            const FVector CancelAt = Body->GetActorLocation();
            ASC->CancelAbilityHandle(Handle);
            TestTrue(TEXT("Cancel immediately clears movement owner"),
                     Ability->MoveTask == nullptr);
            if (!Advance(0.4f))
            {
                return false;
            }
            TestTrue(TEXT("Cancel stops partial displacement"),
                     FVector::Dist2D(CancelAt, Body->GetActorLocation()) <= 1.0f);
            TestFalse(TEXT("Cancel drains source"), Move->HasRootMotionSources());
            TestEqual(TEXT("Cancel does not refund"), ASC->GetNumericAttribute(Stamina), 75.0f);

            // Reset the fixture position, not the runtime dodge implementation.
            Body->SetActorLocation(FVector(0, 0, Body->GetActorLocation().Z), false, nullptr,
                                   ETeleportType::TeleportPhysics);
            Body->SetActorRotation(FRotator::ZeroRotator);
            PC->SetControlRotation(FRotator::ZeroRotator);
            AActor* Wall = Block(FVector(120, 0, 100), FVector(10, 500, 200));
            if (!TestNotNull(TEXT("Blocking wall"), Wall))
            {
                return false;
            }
            Refill();
            if (!Start() || !Advance(1.0f))
            {
                return false;
            }
            TestTrue(TEXT("Capsule stops before wall, no teleport through it"),
                     Body->GetActorLocation().X > 1.0f && Body->GetActorLocation().X <= 70.0f);
            TestFalse(TEXT("Blocked movement still cleans source"), Move->HasRootMotionSources());
            Wall->Destroy();
            if (!Advance(Step))
            {
                return false;
            }

            Refill();
            // ---- 暂停须同时冻结位置、RMS与Montage；检查后总是先恢复世界 ----
            const FVector PauseStart = Body->GetActorLocation();
            if (!Start() || !Advance(0.1f))
            {
                return false;
            }
            TSharedPtr<FRootMotionSource> PausedSource = Ability->MoveTask->Source;
            const FVector PauseAt = Body->GetActorLocation();
            const float PauseTime = PausedSource->GetTime();
            const float MontageTime = Mesh->GetAnimInstance()->Montage_GetPosition(Montage);
            APlayerState* Pauser = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pause owner"), Pauser))
            {
                return false;
            }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            const bool bPausedTicks = Advance(0.5f);
            TestTrue(TEXT("Pause freezes location"),
                     FVector::Dist(PauseAt, Body->GetActorLocation()) <= 0.01f);
            TestTrue(TEXT("Pause freezes source"),
                     FMath::IsNearlyEqual(PauseTime, PausedSource->GetTime(), 0.0001f));
            TestTrue(TEXT("Pause freezes montage"),
                     FMath::IsNearlyEqual(MontageTime,
                                          Mesh->GetAnimInstance()->Montage_GetPosition(Montage),
                                          0.0001f));
            World->GetWorldSettings()->SetPauserPlayerState(
                nullptr); // Restore even if assertions failed.
            if (!bPausedTicks || !Advance(1.0f))
            {
                return false;
            }
            TestTrue(TEXT("Resume completes original 150 cm"),
                     FMath::Abs(FVector::Dist2D(PauseStart, Body->GetActorLocation()) - 150.0f) <=
                         1.0f);
            TestFalse(TEXT("Resume leaves no source"), Move->HasRootMotionSources());
            TestFalse(TEXT("Resume ends action"), Ability->IsActive());

            Refill();
            if (!Start() || !Advance(0.1f))
            {
                return false;
            }
            // 直接Health写入是诊断式致死夹具，不替代普通战斗受击结算。
            const FVector DeathAt = Body->GetActorLocation();
            ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
            TestTrue(TEXT("Real health loss seals death"), Body->IsCombatDead());
            TestFalse(TEXT("Death cancels active ability"), Ability->IsActive());
            TestTrue(TEXT("Death releases task owner"), Ability->MoveTask == nullptr);
            if (!Advance(0.4f))
            {
                return false;
            }
            TestTrue(TEXT("Death leaves no displacement"),
                     FVector::Dist2D(DeathAt, Body->GetActorLocation()) <= 1.0f);
            TestFalse(TEXT("Death drains source"), Move->HasRootMotionSources());
        }
        else
        {
            Refill();
            if (!Start() || !Advance(0.1f))
            {
                return false;
            }
        }
        // At 30/120, EndPlay happens while the actual source is moving.
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("EndPlay cancels ability"), Ability->IsActive());
        TestTrue(TEXT("EndPlay releases owned task"), Ability->MoveTask == nullptr);
        TestFalse(TEXT("EndPlay clears Dodging"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        Fixture.ForwardErrorMessages(this);
        if (HasAnyErrors())
        {
            return false;
        }
    }
    return !HasAnyErrors();
}

#endif
