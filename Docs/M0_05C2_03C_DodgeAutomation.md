# M0-05C2-03C：真实位移的专项自动回归

> 最终验收（2026-10-01 整理）：用户 9 月 29 日约 **30 分钟**完成，Build、DodgeMovement 两次和 Lifecycle 回归报告 Success；源码与指南一致，本地日志确认一次新测试 Success，三种固定步长均为 150.05 cm / 0.3001 s。C2-03 / C2 已完成，本页不用重做。下一步见 [C3 组合演示](M0_05C3_CombinedDemo.md)。

日期：2026-09-29；UE 5.5.4。预计手动 **30–45 分钟**。C2-03B 已验收：首次实现 50 分钟、补修 30 分钟；无需重做 B。

本批只增加测试入口，不改变玩家规则或蓝图。**下面的代码只存在于本文，助手已按本机引擎声明和当前工程静态核对，未写入工程、编译或运行。** 仍由你手动实现和验证。

## 1. 这项测试解决什么问题

已有 Lifecycle 为了隔离动画和状态，关闭了测试角色的移动 Tick，因此它无法发现“动画正常但实际没移动”“打断后继续滑行”等回归。本批新建独立测试，保留真实 CharacterMovement 更新和胶囊碰撞，在临时世界自动生成一块平地。

| 用例 | 验证内容 |
|---|---|
| 1/30、1/60、1/120 秒三种固定步长 | 150 cm 右侧位移、位移源累计约 0.30 秒、先停位移后解锁、结束后无漂移 |
| 1/60 秒步长下中途受击 | 已经移动但尚未走满；受击后立刻停位移、结束恢复 Idle |
| 1/60 秒步长下暂停 / 恢复 | 暂停时位置、动画进度和位移源时间都冻结，恢复后总距离不增加 |

固定步长是**模拟输入条件**，不代表 GPU 达到 30 / 60 / 120 FPS，也不是性能基准。B 中的墙壁 / 坡面 / 键鼠 / UI 仍以实机结果为据；本批不增加整套重复手测，也不扩展为测试框架工程。

## 2. 在头文件中增加一个测试友元

保存并关闭 UE。打开：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.h
```

在文件末尾已有的 `#if WITH_DEV_AUTOMATION_TESTS` 区域，保留旧行并增加一行，使该区域为：

```cpp
#if WITH_DEV_AUTOMATION_TESTS
    friend class FETPracticeActionLifecycleTest;
    friend class FETPracticeDodgeMovementTest;
#endif
```

这里只让新测试设置临时组件的资源、检查持有的位移源；不把这些字段改成 public，不新增 UFUNCTION / UPROPERTY，不修改玩家蓝图参数。

## 3. 新建一个普通 .cpp 文件

如果 VS 看不到 **Tests**（不是 Texts），先按 [VS 目录查看与同步指南](VS_FileVisibility.md) 切换文件夹视图。目录已经存在，不用清缓存或重复创建。解决方案视图中的逻辑分组不一定与当前磁盘同步。

准确路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETPracticeDodgeMovementTests.cpp
```

这是普通 C++ 测试文件，**不用 UE 的 Add C++ Class 向导**，不创建 .h，也不需要 generated.h。可以在 VS 对 Tests 文件夹使用“添加 → 新建项 → C++ 文件”，确认实际保存到上面的 Source 目录，而不是 Intermediate。必要时用“文件 → 新建文件”另存到此路径；避免生成 `.cpp.txt`。

下面是**新文件的全部内容**，一次完整写入，不要插进原 Lifecycle 文件中，也不要覆盖它。`State` 别名已放在函数最前面，避免前次的声明顺序问题。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "../ETPracticeActionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETPracticeDodgeMovementTest,
    "EmberTrial.M0.PracticeAction.DodgeMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETPracticeDodgeMovementTest::RunTest(const FString& Parameters)
{
    using State = EETPracticeActionState;
    for (const int32 StepRate : {30, 60, 120})
    {
        AddInfo(FString::Printf(TEXT("[DodgeMovement] Begin StepRate=%d"), StepRate));
        FTestWorldWrapper TestWorld;
        if (!TestWorld.CreateTestWorld(EWorldType::Game))
        {
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = TestWorld.GetTestWorld();
        AActor* Floor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Temporary floor actor"), Floor)) return false;
        UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(FloorBox);
        Floor->SetRootComponent(FloorBox);
        FloorBox->SetMobility(EComponentMobility::Static);
        FloorBox->SetBoxExtent(FVector(1000.0f, 1000.0f, 50.0f));
        FloorBox->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
        FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorBox->SetCollisionObjectType(ECC_WorldStatic);
        FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
        FloorBox->RegisterComponent();

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ACharacter* Character = World->SpawnActor<ACharacter>(
            FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator, SpawnParameters);
        if (!TestNotNull(TEXT("Temporary moving character"), Character)) return false;
        UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
        // This fixture has no player controller; real movement must still run.
        Movement->bRunPhysicsWithNoController = true;
        USkeletalMeshComponent* Mesh = Character->GetMesh();
        USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
        if (!TestNotNull(TEXT("Quinn mesh"), Quinn)) return false;
        Mesh->SetSkeletalMeshAsset(Quinn);
        Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

        UETPracticeActionComponent* Component = NewObject<UETPracticeActionComponent>(Character);
        Character->AddInstanceComponent(Component);
        Component->RegisterComponent();
        Component->DodgeDistance = 150.0f;
        Component->DodgeDuration = 0.30f;
        Component->DodgeMontage = LoadObject<UAnimMontage>(nullptr,
            TEXT("/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Dodge_01_Montage.AM_M0_Dodge_01_Montage"));
        Component->HitReactMontage = LoadObject<UAnimMontage>(nullptr,
            TEXT("/Game/EmberTrial/Animations/Practice/Montages/AM_M0_HitResult_01_Montage.AM_M0_HitResult_01_Montage"));
        if (!TestNotNull(TEXT("Dodge montage"), Component->DodgeMontage.Get())
            || !TestNotNull(TEXT("Hit montage"), Component->HitReactMontage.Get())) return false;
        if (!TestWorld.BeginPlayInTestWorld())
        {
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
        // Start above the floor; let real collision resolve landing.
        Movement->SetMovementMode(MOVE_Falling);
        const float Step = 1.0f / static_cast<float>(StepRate);
        auto Advance = [&TestWorld, Step, StepRate](float Seconds)
        {
            const int32 Count = FMath::RoundToInt(Seconds * StepRate);
            for (int32 Index = 0; Index < Count; ++Index)
            {
                if (!TestWorld.TickTestWorld(Step)) return false;
            }
            return true;
        };
        if (!Advance(1.0f))
        {
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
        if (!TestTrue(TEXT("Movement tick enabled"), Movement->IsComponentTickEnabled())
            || !TestTrue(TEXT("Character landed on real floor"), Movement->IsMovingOnGround())
            || !TestNotNull(TEXT("Animation instance"), Mesh->GetAnimInstance())) return false;

        // Full movement at each fixed step; observe the source without driving it.
        const FVector Start = Character->GetActorLocation();
        const FVector ExpectedEnd = Start + Character->GetActorRightVector() * 150.0f;
        if (!TestTrue(TEXT("Dodge starts"), Component->RequestPracticeAction(State::Dodging))) return false;
        TSharedPtr<FRootMotionSource> Source = Component->DodgeSource;
        if (!TestTrue(TEXT("Dodge owns a source"), Source.IsValid())) return false;
        if (!Advance(0.4f)) return false;
        const FVector StoppedAt = Character->GetActorLocation();
        const float Travel = FVector::Dist2D(Start, StoppedAt);
        TestTrue(TEXT("Dodge reaches right-side endpoint within 1 cm"),
            FVector::Dist2D(StoppedAt, ExpectedEnd) <= 1.0f);
        TestTrue(TEXT("Source simulates 0.30 seconds"),
            FMath::IsNearlyEqual(Source->GetTime(), 0.30f, 0.002f));
        TestFalse(TEXT("Movement source ownership released"), Component->DodgeSource.IsValid());
        TestFalse(TEXT("Movement source drained from movement component"), Movement->HasRootMotionSources());
        TestTrue(TEXT("State remains Dodging after movement ends"),
            Component->GetPracticeActionState() == State::Dodging);
        if (!Advance(0.6f)) return false;
        TestTrue(TEXT("Montage later returns Idle"), Component->IsPracticeActionIdle());
        TestTrue(TEXT("No horizontal drift after movement ends"),
            FVector::Dist2D(StoppedAt, Character->GetActorLocation()) <= 1.0f);
        TestTrue(TEXT("No residual horizontal velocity"), Movement->Velocity.Size2D() <= 1.0f);
        AddInfo(FString::Printf(TEXT("[DodgeMovement] StepRate=%d Travel2D=%.2f SourceTime=%.4f"),
            StepRate, Travel, Source->GetTime()));
        TestWorld.ForwardErrorMessages(this);
        if (HasAnyErrors()) return false;

        if (StepRate == 60)
        {
            // Interrupt after actual displacement has started.
            const FVector HitStart = Character->GetActorLocation();
            if (!TestTrue(TEXT("Dodge before hit"), Component->RequestPracticeAction(State::Dodging))) return false;
            if (!Advance(0.1f)) return false;
            const FVector HitAt = Character->GetActorLocation();
            const float BeforeHit = FVector::Dist2D(HitStart, HitAt);
            TestTrue(TEXT("Hit occurs during partial displacement"), BeforeHit > 1.0f && BeforeHit < 149.0f);
            if (!TestTrue(TEXT("Hit interrupts moving dodge"), Component->RequestPracticeAction(State::HitReact))) return false;
            TestFalse(TEXT("Hit immediately releases owned source"), Component->DodgeSource.IsValid());
            TestTrue(TEXT("State becomes HitReact"), Component->GetPracticeActionState() == State::HitReact);
            if (!Advance(1.0f)) return false;
            TestTrue(TEXT("Hit leaves no further horizontal displacement"),
                FVector::Dist2D(HitAt, Character->GetActorLocation()) <= 1.0f);
            TestFalse(TEXT("Hit leaves no root motion source"), Movement->HasRootMotionSources());
            TestTrue(TEXT("Hit eventually returns Idle"), Component->IsPracticeActionIdle());
            if (HasAnyErrors()) return false;

            // Pause halfway through movement, then resume the same source.
            const FVector PauseStart = Character->GetActorLocation();
            if (!TestTrue(TEXT("Dodge before pause"), Component->RequestPracticeAction(State::Dodging))) return false;
            if (!Advance(0.1f)) return false;
            TSharedPtr<FRootMotionSource> PausedSource = Component->DodgeSource;
            if (!TestTrue(TEXT("Source still active before pause"), PausedSource.IsValid())) return false;
            APlayerState* Pauser = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pause owner"), Pauser)) return false;
            UAnimInstance* Anim = Mesh->GetAnimInstance();
            const FVector PausedAt = Character->GetActorLocation();
            const float PausedSourceTime = PausedSource->GetTime();
            const float PausedMontageTime = Anim->Montage_GetPosition(Component->DodgeMontage);
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            const bool bPauseTicksSucceeded = Advance(0.5f);
            TestFalse(TEXT("Paused request is rejected"), Component->RequestPracticeAction(State::HitReact));
            TestTrue(TEXT("Pause freezes location"),
                FVector::Dist(PausedAt, Character->GetActorLocation()) <= 0.01f);
            TestTrue(TEXT("Pause freezes source time"),
                FMath::IsNearlyEqual(PausedSource->GetTime(), PausedSourceTime, 0.0001f));
            TestTrue(TEXT("Pause freezes montage time"),
                FMath::IsNearlyEqual(Anim->Montage_GetPosition(Component->DodgeMontage), PausedMontageTime, 0.0001f));
            // Always unpause before checking errors or leaving this test case.
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            TestWorld.ForwardErrorMessages(this);
            if (!bPauseTicksSucceeded || HasAnyErrors()) return false;
            if (!Advance(1.0f)) return false;
            TestTrue(TEXT("Resumed dodge covers 150 cm total"),
                FMath::Abs(FVector::Dist2D(PauseStart, Character->GetActorLocation()) - 150.0f) <= 1.0f);
            TestTrue(TEXT("Resumed source finishes at 0.30 seconds"),
                FMath::IsNearlyEqual(PausedSource->GetTime(), 0.30f, 0.002f));
            TestTrue(TEXT("Resume eventually returns Idle"), Component->IsPracticeActionIdle());
            TestFalse(TEXT("Resume releases owned source"), Component->DodgeSource.IsValid());
            TestFalse(TEXT("Resume leaves no root motion source"), Movement->HasRootMotionSources());
            TestTrue(TEXT("Resume leaves no horizontal velocity"), Movement->Velocity.Size2D() <= 1.0f);
        }
        TestWorld.ForwardErrorMessages(this);
        if (HasAnyErrors()) return false;
    }
    return true;
}

#endif
```

### 3.1 理解这几个关键点

- 地板是临时 UBoxComponent，顶面 Z=0；角色从 Z=200 自然落地。不能为了通过测试直接一直设 Walking 或关闭移动 Tick，那会掩盖没有地板 / 没有真实移动的问题。
- `bRunPhysicsWithNoController=true` 只属于本测试临时角色，因为它没有玩家 Controller。不要复制到玩家蓝图来“修”其他问题。
- 每种步长都使用新的 FTestWorldWrapper；析构负责销毁自己的世界。测试不保存地图或资产，不修改全局 `t.MaxFPS`、时间膨胀或玩家参数。
- 测试保存 Source 的共享指针只是读取完成后的时间，不推进它、不延长实际移动。移动权仍由 CharacterMovement 持有 / 移除。
- 1 cm / 0.002 s 是本次平地自动回归的容差，不要求浮点值逐位相等。如果超差先定位，不直接放宽阈值。
- AttackMontage 在这项测试中不赋值，因为没有攻击请求；攻击仍由原 Lifecycle 覆盖。

## 4. Build 与运行

1. 保存头文件和新 `.cpp`，确认 UE 已关闭。
2. VS 使用原来的 **Development Editor / Win64** 配置正常 Build。
3. 如果 VS 的项目树没显示新文件，先用文件夹视图或 Ctrl+O 按完整路径确认；若需要同步解决方案树，再按 [目录同步指南](VS_FileVisibility.md) 轻量生成项目文件，无需清缓存。不要往 Intermediate 中补一份，也不要删除 Saved 排障。
4. 成功后打开 UE，**停止 PIE**，在 Output Log 控制台执行：

```text
Automation RunTests EmberTrial.M0.PracticeAction.DodgeMovement
```

预期 `Test Completed. Result={Success}`，路径为 `EmberTrial.M0.PracticeAction.DodgeMovement`，并出现三组 StepRate 摘要。它是一项综合测试，不是三个独立测试。

首次通过后，再执行一次同一命令，确认临时世界销毁和重复运行没有残留。最后回归旧测试一次：

```text
Automation RunTests EmberTrial.M0.PracticeAction.Lifecycle
```

这两项通过后，本批不要求再跑一次完整手动矩阵；后续 C3 做约 30 秒组合演示，随后首次打包。

### 4.1 常见失败入口

| 现象 | 先检查 |
|---|---|
| 私有成员无法访问 | 头文件 friend 名称必须是 `FETPracticeDodgeMovementTest`，与宏第一参数一致 |
| 找不到测试 | 新文件真实路径、Build 是否成功、是否仍打开旧编辑器进程；先确认日志，不反复复制文件 |
| `Character landed on real floor` 失败 | 临时 Box 的注册 / 碰撞、角色出生点、无 Controller 的物理开关 |
| Travel 为 0 或位移超差 | 是否保留移动 Tick、Source 时间、碰撞地板以及移动更新；不要修改断言凑通过 |
| 动画状态不符合预期 | Montage 路径是否使用项目实际的 `_Montage` / `HitResult` 名称 |
| 暂停断言失败 | 保留日志，由我对比本机引擎时序；不把暂停用例直接删除 |

## 5. 最简反馈

不用手工抄 Begin / End 或逐条数日志。我默认读取本地 `EmberTrial/Saved/Logs/EmberTrial.log`；告诉我完成时间，避免后续启动覆盖需要的片段即可。有错误时优先贴第一条 error。

```text
M0-05C2-03C 完成 / 卡在第 __ 节
测试 friend / 新文件路径：已按文档完成
关闭 UE 后 Build：成功 / 第一条错误
DodgeMovement 第一次 / 第二次：Success / 实际结果
Lifecycle 回归：Success / 实际结果
大致完成时间：今天 __:__，日志由你读取
源码已保存：是
实际用时：约 __ 分钟
```

本批通过后才将 C2-03 总任务和 C2 标记完成。指南已生成不代表测试已经实现或通过；助手仍按默认约定先给修改建议，只有你明确要求时才直接接管 C++。
