# M0-05C2-03B：补齐准入保护与针对性回归

日期：2026-09-29。预计手动 **20–30 分钟**。本批不重做动画、蓝图或整套实机矩阵。

你报告的正常配置下的位移、碰撞、暂停、打断与旧功能结果已记录，实际约 50 分钟。源码复核发现 `RequestPracticeAction` **漏了原指南第 5 节的整个 Dodging 校验块**；其余六个相关函数忽略空白和注释后与指南一致。先补齐这个小范围遗漏，再完成 B 验收。

仍按约定：**我只写文档，不直接改工程 C++。** 以下新增测试与计时日志未由助手编译或运行，需你执行验证。

## 1. 为什么正常试玩和旧测试能通过

当前参数正好是 150 / 0.30，且试玩开始时没有其他位移源，所以缺少保护不会影响这些正常路径。但异常参数会直接进入 `Distance / Duration`，已有位移源时也可能叠加新闪避。

编辑器的 Clamp 元数据不能替代运行时检查。旧 Lifecycle 测试未包含非法闪避参数和外部 Root Motion Source，因此它成功并不能证明这两项保护存在。本批将这两类用例加入已有测试。

## 2. 补回 RequestPracticeAction 的检查块

保存并关闭 UE，打开：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.cpp
```

找到 `RequestPracticeAction` 中已有的：

```cpp
    if (IsPracticeActionIdle() && AnimInstance->IsAnyMontagePlaying())
    {
        return RejectRequest(RequestedState, TEXT("ExternalMontage"));
    }
```

**在这个 if 完整结束后、`// Retire the old serial...` 注释前**，插入以下代码；不要放在 Montage_Play 之后：

```cpp
    if (RequestedState == EETPracticeActionState::Dodging)
    {
        if (!FMath::IsFinite(DodgeDistance) || DodgeDistance < 1.0f || DodgeDistance > 300.0f
            || !FMath::IsFinite(DodgeDuration) || DodgeDuration < 0.05f || DodgeDuration > 0.50f)
        {
            return RejectRequest(RequestedState, TEXT("InvalidDodgeConfig"));
        }
        if (Movement->HasRootMotionSources())
        {
            return RejectRequest(RequestedState, TEXT("ExternalRootMotion"));
        }
    }
```

这样在开始动画、清除移动输入、创建位移源之前就拒绝无效请求。原有函数其他部分保留。

## 3. 补充计时日志，区分两种时间

当前 `Elapsed` 是结束与开始两个 `World->GetTimeSeconds()` 采样之差。世界在一帧开始阶段累加时间，输入随后可能启动位移，移动组件又在同一帧模拟该帧 DeltaTime。因此**累计模拟时间可以包含起步帧，而这两个时间采样之间没有计入起步帧间隔**。两者还受 Tick 顺序和帧时长影响，不应要求完全相等。

你的日志中，30 上限的 Elapsed=0.267、60 上限的 0.283，与约少一帧的解释相符；0.288 也不能单独判断为提前结束。原指南只提供 Elapsed 容易混淆，现补充 Root Motion Source 自己累计的 `SourceTime` 作区分。

在 **StopDodgeMovement** 中，找到：

```cpp
    const float Elapsed = GetWorld()
        ? GetWorld()->GetTimeSeconds() - DodgeStartWorldTime : 0.0f;
```

紧接着增加一行，必须在 `DodgeSource.Reset()` **之前**：

```cpp
    const float SourceTime = DodgeSource->GetTime();
```

然后把该函数末尾已有的 **整段 UE_LOG** 替换为：

```cpp
    UE_LOG(LogETPracticeAction, Display,
        TEXT("[M0-05C2] DodgeMove End Reason=%s Travel2D=%.1f Elapsed=%.3f SourceTime=%.3f"),
        Reason, Travel2D, Elapsed, SourceTime);
```

普通完整闪避预期 SourceTime 约 0.300，Travel2D 约 150；早期打断时 SourceTime 会更小。它记录位移源累计模拟时间，不是渲染帧率或现实墙钟时间。保持参数 150 / 0.30，不为凑齐 Elapsed 人为增加 Duration。

## 4. 在已有 Lifecycle 测试中加入保护用例

只修改现有文件，不新建类，不改头文件友元：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETPracticeActionTests.cpp
```

### 4.1 新增 include

在原 include 区域增加：

```cpp
#include "GameFramework/RootMotionSource.h"
#include <limits>
```

保留文件顶部 `#if WITH_DEV_AUTOMATION_TESTS` 和末尾 `#endif`。

### 4.2 准确的插入位置

找到已有测试的这一段：

```cpp
    Movement->SetMovementMode(MOVE_Falling);
    for (State Action : {State::Attacking, State::Dodging, State::HitReact})
    {
        TestFalse(TEXT("Airborne request refused"), Component->RequestPracticeAction(Action));
    }
    Movement->SetMovementMode(MOVE_Walking);
```

把下面整个代码块插在这个 **恢复 Walking 的语句后**、原 `Grounded attack accepted` 测试前。原有后续测试全部保留。

```cpp
    // Guard regressions: these requests must fail before any animation or motion starts.
    const float SavedDistance = Component->DodgeDistance;
    const float SavedDuration = Component->DodgeDuration;
    struct FInvalidDodgeCase
    {
        const TCHAR* Name;
        float Distance;
        float Duration;
    };
    const FInvalidDodgeCase InvalidCases[] =
    {
        {TEXT("Zero distance"), 0.0f, 0.30f},
        {TEXT("Negative distance"), -1.0f, 0.30f},
        {TEXT("Too large distance"), 301.0f, 0.30f},
        {TEXT("NaN distance"), std::numeric_limits<float>::quiet_NaN(), 0.30f},
        {TEXT("Zero duration"), 150.0f, 0.0f},
        {TEXT("Negative duration"), 150.0f, -0.10f},
        {TEXT("Too long duration"), 150.0f, 0.60f},
        {TEXT("Infinite duration"), 150.0f, std::numeric_limits<float>::infinity()}
    };
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
        if (bStarted || !Component->IsPracticeActionIdle()
            || Component->DodgeSource.IsValid())
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
        Movement->GetRootMotionSourceByID(ForeignID).Get() == ForeignSource.Get()
        && !ForeignSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));

    // This fixture disables movement ticking, so explicitly drain its marked sources.
    Movement->RemoveRootMotionSourceByID(ForeignID);
    Movement->CurrentRootMotion.CleanUpInvalidRootMotion(0.0f, *Character, *Movement);
    TestFalse(TEXT("Guard fixture leaves no root motion sources"), Movement->HasRootMotionSources());
    if (HasAnyErrors())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
```

这些字段在原 `FETPracticeActionLifecycleTest` 的友元范围内，所以不用把组件字段改成 public。测试只修改临时角色的参数，不会更改玩家蓝图里的 150 / 0.30。

最后的 `CleanUpInvalidRootMotion` 只用于**测试环境收尾**：旧测试关闭了 CharacterMovement Tick，移除标记不会像真实角色那样在后续移动更新中自动清理。不要把它复制到 Gameplay 清理逻辑中，也不要开启旧测试角色的移动 Tick 来规避这一步。

## 5. Build 和自动回归

1. 确认第 2 节保护块已补齐，两个 `.cpp` 已保存，UE 已关闭。
2. 正常 Development Editor / Win64 → Build。
3. 成功后打开 UE，停止 PIE，在 Output Log 控制台执行：

```text
Automation RunTests EmberTrial.M0.PracticeAction.Lifecycle
```

4. 预期最终 `Test Completed. Result={Success}`；过程中应出现 **8 条 InvalidDodgeConfig**、**1 条 ExternalRootMotion**，随后原有生命周期用例继续执行。
5. 若失败，发第一条测试错误和附近日志，不修改期望值让测试强行变绿。

这仍是 **1 项综合自动测试**，内部多了 8 组非法配置与一组外部位移源场景，不声称增加了 9 个独立测试，也不声称覆盖真实平地距离 / 墙壁 / 坡面。真实位移专项自动测试仍在下一批。

## 6. 简短实机复测与 120 FPS 证据

正常配置仍是 150 / 0.30，先做一次平地闪避，记录包含 **SourceTime** 的完整 Begin / End 日志；再做一次 2→3 打断和 P 暂停 / 继续，确认旧行为保持。

此次日志中 30 上限的帧编号 816→824、Elapsed=0.267；60 上限 411→428、0.283，支持相应更新节奏。120 上限那次为 **993→998，仅 5 个帧间隔，Elapsed=0.253**。它支持距离 150.0，但与“位移期间稳定在 120 FPS”不一致，需要再取一个有效样本；具体是焦点、卡顿还是其他设置所致，当前没有证据定因。

只重测 120 这一组即可，不要求重新做整套墙壁 / 坡面矩阵：

1. 新开 PIE，在空地站定。控制台输入 `t.MaxFPS 120`，用 `stat fps` 查看实际值。
2. 将焦点放回游戏视口，等待 2–3 秒，不在位移期间切到 Output Log 或其他窗口。
3. 实际帧率稳定后按 2，录一段能同时看到动作与 FPS 的短视频，或至少记录位移期间观察到的 FPS。
4. 动作结束后再查看日志，复制完整 Begin / End 行，包括时间戳、帧编号、Travel2D、Elapsed、SourceTime。
5. 若达不到 120，就如实填实际值，暂不标记 120 已验证；不要改全局时间膨胀、固定 DeltaTime 或动画速率凑数。
6. 最后恢复 `t.MaxFPS 0`，关闭 `stat fps`，保存。

## 7. 反馈模板

```text
M0-05C2-03B 补修完成 / 卡在第 __ 节
RequestPracticeAction 的两项保护已加回且位于 Montage_Play 前：是
SourceTime 在 DodgeSource.Reset 前读取，结束日志已更新：是
Lifecycle 新保护用例已加入，原测试保留：是
UE 关闭后的 Build：成功 / 第一条错误
Lifecycle：Success / 测试错误
InvalidDodgeConfig 8 条 / ExternalRootMotion 1 条：符合 / 实际日志
正常平地 Begin / End 完整日志：
一次 2→3 / 一次位移中 P 暂停恢复：正常 / 实际结果
120 上限复测：位移期间实际约 __ FPS，完整 Begin / End 日志：
源码已保存、帧率设置恢复：是
补修实际用时：约 __ 分钟
```

本批补齐后复核 B 的最终结论，再安排实际移动专项自动测试和 C3 组合演示。不重算前次 50 分钟；补修时间单独记录。
