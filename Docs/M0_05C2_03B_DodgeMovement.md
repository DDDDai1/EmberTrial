# M0-05C2-03B：手动接入 CharacterMovement 闪避位移

2026-10-19第19批清理同步：以下配置、接入及验收属于M0历史，当前左键 / 1、2、Q已走正式GAS；Practice只保留兼容及原回归入口。完整代码以Source链接为准，本轮按用户授权由助手直接清理；实际新检查和提交清单见[Cleanup](Cleanup.md)。

> 2026-09-29 最终验收：首次实现 **50 分钟**、[保护补修](M0_05C2_03B_GuardCorrection.md) **30 分钟**；源码、Lifecycle、新计时日志及约 119 FPS 样本均已核对，正常场景按用户实机结果通过。**C2-03B 已完成，不用重做本页。** 下一步执行 [C2-03C 位移专项自动回归](M0_05C2_03C_DodgeAutomation.md)。

日期：2026-09-28。引擎：UE 5.5.4。预计手动 **45–75 分钟**，包括代码、正常 Build 和分组测试；以实际反馈为准。

原开发按手动实践完成，本轮清理按最新授权由助手直接执行。历史代码已移除副本，当前实现 / 检查以Source和Cleanup为准。

C2-03A 已验收，实际约 10 分钟。你添加的 `DodgeDistance=150`、`DodgeDuration=0.30` 保留。此批不创建新类，不改蓝图输入，不改 Character / AnimBP / AS。

## 1. 最终行为与职责

按 2：角色沿**开始时角色自身右侧**移动，平地无障碍目标 150 cm / 0.30 秒。不是屏幕右侧，也不根据镜头中途改方向。位移交给 CharacterMovement 的 Root Motion Source，胶囊仍参与碰撞。

| 情况 | 预期 |
|---|---|
| 0.30 秒位移结束 | 水平速度停止；仍为 Dodging，等待约 0.70 秒的 Montage 完成再开放操作 |
| 撞墙 / 沿斜面 | 使用 CharacterMovement 碰撞与地面运动；墙会缩短距离，斜向撞墙可能沿墙滑动 |
| 3 打断闪避 | 清掉闪避位移，开始受击，受击结束恢复 |
| P 暂停 | 动画和位移一起冻结；恢复后继续剩余部分 |
| 中途离开地面 | 结束本次水平闪避位移，保留下落速度；不强制拉回地面，Montage 仍正常收尾 |
| 取消 / 动画实例失效 / EndPlay | 清理本组件拥有的位移源和回调，不留持续推动 |

本页描述的是旧M0侧闪占位，不含无敌帧或成本；当前正式2已由M2方向闪避接管。原动画双脚没有步伐，世界位移会呈现滑步占位效果；此批验收功能与碰撞，正式步伐以后打磨。

位移源的 `Force` 在本实现中提供目标速度向量：150 / 0.30 = **500 cm/s**；不是给刚体施加冲量。动画 Enable Root Motion 保持 False，不叠加根骨提取位移。

## 2. 开始前

1. 保存蓝图 / 地图，停止 PIE，关闭 UE；VS 可以保留。
2. 保存现有源码。本轮只编辑以下两个文件：
   - `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.h`
   - `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.cpp`
3. 按第 3–8 节全部完成后再 Build。中间阶段声明与定义可能不齐，不以中途 IntelliSense 红线判断最终失败。

下文写“替换函数”时，替换从函数签名到匹配的最后一个 `}` 的**整段定义**，不保留重复函数。写“新增”时只插入给出的代码。

## 3. 修改 .h：只新增运行时信息与私有函数

### 3.1 前置声明

找到头文件顶部 `class UAnimMontage;`，在它后面添加：

```cpp
class UCharacterMovementComponent;
struct FRootMotionSource;
```

### 3.2 私有字段

找到现有 `bool bEndingPlay = false;`，在它后面添加：

```cpp
    UPROPERTY(Transient)
    TWeakObjectPtr<UCharacterMovementComponent> DodgeMovement;

    // Native struct, not a UObject; owned only for the current dodge.
    TSharedPtr<FRootMotionSource> DodgeSource;
    FVector DodgeStartLocation = FVector::ZeroVector;
    float DodgeStartWorldTime = 0.0f;
```

`DodgeMovement` 是非持有式 UObject 引用；`DodgeSource` 是普通 C++ 结构体的共享指针，用于区分自己创建的位移源。它不是 UObject，不加 UPROPERTY，也不让蓝图修改。

### 3.3 私有函数声明

在现有 `RejectRequest(...) const;` 后、`#if WITH_DEV_AUTOMATION_TESTS` 前添加：

```cpp
    bool StartDodgeMovement(ACharacter* Character, UCharacterMovementComponent* Movement);
    void StopDodgeMovement(const TCHAR* Reason);
    void UpdateDodgeMovement();
    void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, uint32 Serial);
```

保留原来的三个 Montage、两个闪避参数、CurrentState 和全部原有函数声明。

## 4. 修改 .cpp：增加 include 和 Tick 顺序

### 4.1 include

在 `.cpp` 的 include 区域新增以下两行，不删原有 include：

```cpp
#include "Engine/World.h"
#include "GameFramework/RootMotionSource.h"
```

### 4.2 BeginPlay 内增加一段

找到已有这段代码：

```cpp
    if (USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh())
    {
        AddTickPrerequisiteComponent(Mesh);
    }
```

在它后面、`Component ready` 日志前增加：

```cpp
    if (UCharacterMovementComponent* Movement = CharacterOwner->GetCharacterMovement())
    {
        AddTickPrerequisiteComponent(Movement);
    }
```

作用：忙碌时的组件检查安排在移动与动画更新之后，读取该帧实际处理后的状态。仍仅忙碌时 Tick。

## 5. 替换 RequestPracticeAction 整个函数

以下保留 C2-02 的规则，增加参数校验、已有位移源检查、淡出回调和启动位移。

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

只有请求成功播放动画后才创建位移源；创建失败会停止本次动画并释放状态。`ExternalRootMotion` 是保守的 M0 规则：已经有位移源时不叠加新闪避；没有为后续多技能系统制定优先级矩阵。

## 6. 替换 ReleaseAction 整个函数

正常结束、受击打断、取消、Deactivate、EndPlay 已经都经过这里。统一在此清理位移，避免每条退出路径各写一套。

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

原来的 `HandleMontageEnded` / `CancelPracticeAction` / `Activate` / `Deactivate` / `EndPlay` 保留。不要新增角色蓝图 EndPlay 节点或 Delay 解锁。

## 7. 替换 TickComponent 整个函数

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

这里只检查生命周期，没有每帧 SetActorLocation。真正位移由 CharacterMovement 更新。

## 8. 在 .cpp 末尾新增四个函数定义

插在现有 EndPlay 函数完整结束之后。以下四个函数都放在文件作用域，不能放进 EndPlay 的大括号内。

### 8.1 创建位移源

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

方向用世界坐标保存，因此中途转镜头不会改变轨迹。`IgnoreZAccumulate` 使这份水平位移不覆盖垂直速度；结束参数 Clamp=0 会清掉水平动量、保留下落的负 Z。清除 `DisablePartialEndTick` 是为了处理结束时不足一整帧的情况，实际距离仍需 30 / 60 / 120 FPS 验证。

### 8.2 幂等清理与记录距离

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

重复调用会立即返回。用 ID 加共享指针身份确认自己拥有的源，不清空 CharacterMovement 的全部 Root Motion。引擎移除源是先标记、在移动更新中清理；这里也清除当前水平速度，避免取消后沿用本次闪避速度。`Travel2D` 是起终点的水平直线距离，不是沿墙转弯的路径长度；碰撞时小于 150 属于可能的正常结果。

2026-09-29 增加诊断字段 `SourceTime`，在 Reset 前读取位移源累计模拟时间。`Elapsed` 是世界时间采样差，可能少计起步所在帧；不要用它单独判断位移源提前结束。现有实现者按补修指南添加两处日志改动即可。

### 8.3 检查位移结束 / 离地

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

这里不把 CurrentState 改回 Idle。位移窗口先结束，Montage 生命周期继续管理动作限制。

### 8.4 外部动画打断时及时停位移

当前源码入口：[ETPracticeActionComponent.cpp](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

结束回调保留动作解锁职责；淡出回调只在被打断时提前停止位移，避免较长淡出期间还在闪避。仍检查动作序号，旧回调不影响新动作。

## 9. 正常 Build 与编辑器确认

1. 两个文件保存；UE 必须已关闭。
2. VS：Development Editor / Win64 → EmberTrial 项目 → Build。
3. 成功后重开 UE，打开 BP_ThirdPersonCharacter，选择已有 PracticeAction。
4. 核对 150 cm / 0.30 s、三个 Montage、Auto Activate=True；Compile / Save。
5. 三条 Started 和 E 的 Idle 门控保持 C2-02 接线，不新增任何位移节点。

如果编译失败，先发第一条真实错误及上下文，我给出修正代码。没有你的明确接管要求，我不会直接改源码。本轮不要求删除生成目录、修改引擎或安装新插件。

## 10. 分组实机验收

### 10.1 无障碍平地、方向与收尾

1. 在 ThirdPersonMap 找右侧至少 3 m 的空地，站定按 2，随后不按移动键。
2. 角色向自身右侧平移；日志先有 `DodgeMove Begin`，随后有动作 Begin。约 0.30 秒出现 `DodgeMove End`，约 0.70 秒动作 Completed。
3. 普通结束的 Reason 通常为 `DistanceWindowEnded`；引擎先移除源时也可能为 `SourceRemoved`。无障碍平地 `Travel2D` 初步验收目标为 **145–155 cm**；这是目标范围，尚未实测。
4. 位移停下后到动画结束前，移动 / 跳跃 / E 仍应被拒绝；动作结束后恢复。
5. 再试一次：一直按住 W，动作结束后继续移动是预期；在动作期间松开 W，结束后不应自行漂移。
6. 先转动角色朝向再站定按 2，依然朝角色右侧；只转镜头后再按，不应改为屏幕右侧。闪避中转镜头，轨迹不弯向镜头。

### 10.2 打断与暂停

1. 按 2 后约 0.1 秒按 3，确认右移提前停止并转入受击；`DodgeMove End` 的 Reason 应为 HitInterrupt，Travel2D 通常小于完整闪避距离。重复五次。
2. 受击期间连按 1/2/3 不重启、不再启动右移；结束后可再次闪避。
3. 先按下 P 暂不松开，再按 2，并立刻松 P，让菜单在位移尚未结束时打开。保持暂停约 2 秒，然后继续：角色和动画应从暂停处继续，没有瞬移或多走一段。若打开菜单时位移早已结束，重试本项。
4. 菜单中按下 / 松开 2/E/W，继续后无补播、无卡键。松开所有移动键后观察约 1 秒，不继续漂移。
5. 闪避期间停止 PIE，再 Play；连续三次，无错误或残留位移。

### 10.3 墙壁与坡面

1. 先用地图已有的高墙 / 大型方块，调整角色朝向，让**角色右侧**正对墙面，胶囊距墙面约 50–100 cm；不要使用可跨越的小台阶作为挡墙。
2. 按 2：角色被墙挡住，不穿透、不突然弹到墙后；位移窗口 / 动画仍结束，操作恢复。Travel2D 小于 150 正常。
3. 再斜向撞墙，允许沿墙滑动，但不能穿透、无限持续滑行或卡在 Dodging。
4. 在已有可行走坡面中段沿上坡 / 下坡方向各测试一次：贴合地面、没有明显悬浮或瞬移。坡面距离不使用平地 145–155 cm 的严格阈值。
5. 若地图缺少合适坡面，可临时添加 Basic Cube：Scale=(6,4,0.2)，Rotation 的 Pitch=15°，放到地面并将低端埋入地面，Collision 使用 BlockAll。位置以地面高度调整；确认普通走路能上下，再测闪避。测试后删除这个临时立方体、恢复场景设置。

靠近台边的离地路径暂作补充：一旦真正进入 Falling，应结束水平闪避并正常下落，不悬停。不要为了这一项修改全局重力或跳跃设置。

### 10.4 帧率与旧功能

停止 PIE 后重新开始，在同一块空地测试。打开 Output Log 底部控制台输入框，依次使用 `t.MaxFPS 30`、`t.MaxFPS 60`、`t.MaxFPS 120`；每个设置分别闪避一次，记录 Travel2D，结束后恢复 `t.MaxFPS 0`。

`t.MaxFPS` 是上限，不保证电脑实际达到该帧率。可用 `stat fps` 查看，反馈“上限 / 观察到的 FPS / Travel2D”；达不到 120 就如实写实际帧率，不能标为已验证 120 FPS。测试后再次输入 `stat fps` 关闭显示。

确认原攻击 / 受击、地面移动时攻击拒绝而 2/3 允许、空中三键拒绝、E 的 A/B 交互、P、移动 / 转向 / 跳跃均保持 C2-02 规则。

### 10.5 已有自动回归

停止 PIE，在 Output Log 控制台执行：

```text
Automation RunTests EmberTrial.M0.PracticeAction.Lifecycle
```

等待 `Test Completed. Result={Success}`。把结果行发来；失败则发第一条测试错误和附近日志。本条是已有的 C2-02 生命周期综合回归：它关闭测试角色的移动 Tick，**不验证真实距离、墙壁或坡面**。新增位移专项自动测试留待这批代码与实机结果核对后，继续以文档代码形式交给你实现。

## 11. 新日志与排障

| 日志 | 含义 |
|---|---|
| `DodgeMove Begin` | 位移源创建成功，记录 ID / 距离 / 时间 / 世界方向 |
| `DodgeMove End` | 位移源释放，记录原因、水平直线距离、游戏时间差 |
| `InvalidDodgeConfig` | 参数超出本批范围或非有限值，检查组件配置 |
| `ExternalRootMotion` | 已有位移源 / 根骨位移时拒绝新闪避；若持续出现，发日志，由我检查清理路径 |
| `DodgeMovementFailed` | 无法创建位移源或方向无效，发完整上下文 |
| `LeftGround` | 中途离地，停止水平闪避，正常下落 |

不要只用日志判断画面正确。若撞墙穿透、状态卡住、暂停后多走、三种帧率距离明显不同，记录按键顺序、实际 FPS 和日志；涉及运动轨迹时补 5–10 秒视频。我会先定位并提供修正代码，只有你明确说需要直接修改才接管源码。

## 12. 反馈模板

```text
M0-05C2-03B 完成 / 卡在第 __ 节
.h 声明、.cpp 第 4–8 节全部完成：是
UE 关闭后的 C++ Build：成功 / 第一条错误原文
PracticeAction 150 cm / 0.30 s，原资源与 Auto Activate：正确
角色蓝图 Compile：通过 / 错误
角色自身右侧 / 方向固定：通过 / 实际结果
平地无输入 Travel2D / Elapsed：__ cm / __ s
位移先结束、Montage 后解锁；无残留漂移：通过
按住 W / 中途松 W / 跳跃和 E 门控：通过
3 打断闪避连续五次，受击不重启：通过
位移中暂停 / 恢复，菜单不补播不卡键：通过
正对高墙 / 斜向墙壁：通过 / 实际结果
可走坡面上坡 / 下坡：通过 / 实际结果
30 上限：实际约 __ FPS，Travel2D=__
60 上限：实际约 __ FPS，Travel2D=__
120 上限：实际约 __ FPS，Travel2D=__
动作中停止 PIE、三次重开：通过
原 1/3 / E-A/B / P / 移动 / 转向 / 跳跃：正常
Lifecycle 自动回归：Success / 错误原文
源码 / 资产已保存，临时场景和帧率设置已恢复：是
实际用时：约 __ 分钟
```

## 13. 实现依据与复盘要点

本机核对入口：`GameFramework/RootMotionSource.h`、`Private/GameFramework/RootMotionSource.cpp`、`Private/Components/CharacterMovementComponent.cpp`、`Animation/AnimMontage.h`。已核对 Apply / Remove / GetByID、Finished / MarkedForRemoval、部分结束帧设置、ClampVelocity 对负 Z 的处理，以及 Montage 淡出委托签名。

官方入口：[UE 5.5 FRootMotionSource_ConstantForce](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/GameFramework/FRootMotionSource_ConstantForce?application_version=5.5)。源码核对不等于构建或运行通过。

复盘时应能解释：为何用 CharacterMovement；为何位移时长与动作状态时长分开；为什么只移除自己的位移源；为什么取消时处理水平速度却保留下落；为何同时需要结束和淡出回调；如何用帧率 / 墙壁 / 暂停测试证明实现，而不是只看一次空地播放。

## 本次清理 Commit / Push（固定放在最后）

第19批四份Source与十份Markdown，共14项；当前检查、最新差异和逐文件清单见[Cleanup末尾](Cleanup.md)。历史资产 / 测试未改，不重复制造差异。用户重新Stage后Commit / Push；Raw、报告、索引留本地，助手不操作Git索引。
