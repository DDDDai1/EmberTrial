# M0-05C2-01：创建并挂载动作组件

日期：2026-09-27。引擎：UE 5.5.4。预计手动 **25–40 分钟**，含一次关闭编辑器后的 C++ Build 和基础回归。

这次由你创建 C++ 类、实现基础结构、挂载组件；复杂的 Montage 回调、打断与 CharacterMovement 位移由我在下一批实现并检查。**当前只做本指南，不提前替换 C1 的输入与播放函数。**

## 1. 起点与本批结果

你已修复 `TryPlayPracticeMontage` 中水平速度检查的 True / False 连线，确认播放恢复。2026-09-27 磁盘复核也确认：`速度 <= 3` 的 True 进入忙碌检查，False 进入 Return；三个 Montage 引用与基础混合参数正确。

此前闪避补修的最后一项也已通过只读检查：0 帧左上臂 Z=25，全部目标关键帧 / 范围差异为 0，重新烘焙的闪避左手首尾位置差为 0 cm。无需重复制作。

完成本批后：

- 玩家角色有且仅有一个 `ETPracticeActionComponent`。
- 每次 Play，该组件自动输出一条初始化日志。
- 蓝图能读取 C++ 动作状态，初始为 Idle。
- 原有 1/2/3、E/P、移动、转向、跳跃仍走现有逻辑。

本批组件尚未接管动作，所以**按 1/2/3 时组件状态仍是 Idle，属于预期**。下一批接入后才会随动作变化。不要为了改变显示而在蓝图增加一套可写状态。

## 2. C2 的完整安排与职责

| 批次 | 你操作 | 我负责 | 完成时可见结果 |
|---|---|---|---|
| **C2-01，本次** | 创建类、基础代码、正常 Build、挂载和回归 | 审查反射 / 所有者 / 生命周期 | 组件正常初始化，不影响 C1 |
| C2-02 | 按指南填三个 Montage、调整三条输入调用 | 实现状态转换、Montage 生命周期、输入限制、打断与清理 | 动作期间规则明确，受击可打断攻击 / 闪避，结束恢复 |
| C2-03 | 在场景测试距离、墙壁、坡面、暂停和重开 | 实现与校准 CharacterMovement 闪避、异常路径 | 闪避实际移动，碰撞有效，中断后无残留位移 |
| C3 | 组合回归和约 30 秒实机记录 | 汇总证据与 M0-05 结论 | 可复现的四动作演示 |

组件只依赖拥有它的 `ACharacter`、Mesh / AnimInstance 与 CharacterMovement，不查找 UI Widget。蓝图负责把输入意图和动画资源交给组件；AnimBP 继续负责姿态输出。ActorComponent 适合承载附加到 Actor 的独立行为，无须空间 Transform。[Epic UE 5.5 UActorComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Components/UActorComponent?application_version=5.5)

C2-02 / 03 的规则约定如下，**本批不用手动实现这些分支**：

| 当前状态 | 攻击请求 1 | 闪避请求 2 | 受击请求 3 |
|---|---|---|---|
| Idle | 满足地面 / 速度条件才接受 | 满足地面条件才接受 | 本轮地面调试接受 |
| Attacking | 拒绝 | 拒绝 | 打断攻击后受击 |
| Dodging | 拒绝 | 拒绝 | 打断闪避，并清除剩余位移 |
| HitReact | 拒绝 | 拒绝 | 拒绝，避免连按无限重启 |
| 世界暂停 / 暂停菜单中 | 拒绝 | 拒绝 | 拒绝 |

动作忙碌期间锁普通移动、跳跃与 E 交互，保留镜头与 P；恢复时只释放动作系统自己持有的限制。暂停保留正在进行的动作，关闭菜单后继续；停止 PIE / EndPlay 清理本组件的回调与位移资源。

状态由一个枚举表达；正常结束、被打断、播放失败都汇入明确的退出处理。旧动作的回调不能把新受击错误改回 Idle。动作结束以 Montage 生命周期为准，不用 `Delay(动画时长)` 猜测。

位移继续由 CharacterMovement 处理胶囊碰撞。计划优先验证其程序化 Root Motion Source；这与从 AS 根骨提取位移是两种来源，现有原地 AS 的 Enable Root Motion 保持 False。是否采用、距离和持续时间以 C2-03 实测为准，不在本批混入蓝图 SetActorLocation / Timeline 位移。相关能力见 [Epic CharacterMovement 位移说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine?application_version=5.5)。本 Demo 仍为单机，不增加联机范围。

`Practice` 名称表明这是 M0 可行性组件；正式战斗阶段会把已验证规则迁入计划中的能力系统，避免两套规则同时掌管动作。

## 3. 创建 C++ 类

1. 停止 Play，保存当前地图和蓝图。
2. UE 菜单 **Tools → New C++ Class**。
3. 父类选 **Actor Component**。若列表里没有，勾选 **Show All Classes** 后搜索 `ActorComponent`。不要选择 Actor / Scene Component。
4. 名称填 **ETPracticeActionComponent**，不手动加 U 前缀；生成的类型应为 `UETPracticeActionComponent`。
5. 模块选 **EmberTrial**。沿用现有平铺 Source 目录，预期文件为：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.cpp
```

6. 若向导自动编译，等待结束。随后保存并**关闭 UE 编辑器**，在 Visual Studio 编辑新类。这一轮会增加 UENUM / UCLASS / UFUNCTION，采用正常 Build，不依赖 Live Coding 更新类布局。

若向导使用 Public / Private 子目录，可以保留，反馈实际路径即可。不要重复创建同名类，也不需要重新创建工程或调整模块依赖；现有 Core / CoreUObject / Engine 已覆盖本批代码。

## 4. 实现基础结构

可以按下面完整内容手写或替换向导生成的这两个新文件。不要把整段追加到文件末尾，避免构造函数 / BeginPlay 重复定义。向导生成的 TickComponent 声明和定义都不保留；本批没有逐帧任务。

### 4.1 ETPracticeActionComponent.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETPracticeActionComponent.generated.h"

UENUM(BlueprintType)
enum class EETPracticeActionState : uint8
{
    Idle,
    Attacking,
    Dodging,
    HitReact
};

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETPracticeActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETPracticeActionComponent();

    UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
    EETPracticeActionState GetPracticeActionState() const;

    UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
    bool IsPracticeActionIdle() const;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient,
        Category = "M0|Practice Action", meta = (AllowPrivateAccess = "true"))
    EETPracticeActionState CurrentState = EETPracticeActionState::Idle;
};
```

注意 `ETPracticeActionComponent.generated.h` 必须是本头文件最后一个 include。不要再创建一个同名 Blueprint Enum；蓝图将直接使用这个 C++ 枚举。

### 4.2 ETPracticeActionComponent.cpp

```cpp
#include "ETPracticeActionComponent.h"

#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPracticeAction, Log, All);

UETPracticeActionComponent::UETPracticeActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UETPracticeActionComponent::BeginPlay()
{
    Super::BeginPlay();

    const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
    if (!IsValid(CharacterOwner))
    {
        UE_LOG(LogETPracticeAction, Error,
            TEXT("[M0-05C2] Component requires an ACharacter owner: %s"),
            *GetNameSafe(GetOwner()));
        return;
    }

    UE_LOG(LogETPracticeAction, Display,
        TEXT("[M0-05C2] Component ready Owner=%s State=Idle"),
        *GetNameSafe(CharacterOwner));
}

EETPracticeActionState UETPracticeActionComponent::GetPracticeActionState() const
{
    return CurrentState;
}

bool UETPracticeActionComponent::IsPracticeActionIdle() const
{
    return CurrentState == EETPracticeActionState::Idle;
}
```

### 4.3 这几处代码的作用

| 代码 | 目的 |
|---|---|
| UENUM(BlueprintType) | C++ 与蓝图共用相同的状态类型 |
| BlueprintSpawnableComponent | 允许在角色蓝图的 Add 菜单装配此组件 |
| private + BlueprintReadOnly | 蓝图可以查询，状态的写入规则留在组件内部 |
| Transient | 状态属于本次运行，不作为需要保存的角色配置 |
| BlueprintPure + const | 两个函数只查询数据，不启动动作，也不改变状态 |
| Super::BeginPlay() | 保留父类的组件生命周期行为 |
| Cast<ACharacter>(GetOwner()) | 核对组件装在角色上；不依赖具体 BP 名称 |
| 关闭 Tick | 当前只需初始化日志与按需查询，没有逐帧轮询 |

`Idle` 在这里表示“动作组件没有占用角色”，不是“角色水平速度为零”。速度和是否在地面属于接受请求时的额外条件，不能与动作状态混成一个值。

本批未注册委托、定时器或位移源，所以不添加空的 EndPlay 清理。下一批引入这些资源时由我一起实现释放逻辑。

## 5. 正常 Build

1. 确认 UE 编辑器已经关闭，VS 保存全部文件。
2. 打开现有 `EmberTrial.sln`，配置 **Development Editor**，平台 **Win64**。
3. 右键 **EmberTrial** 项目 → **Build**。不用 Rebuild 整个解决方案。
4. 看 **Output → Build** 的最终结果；不能仅凭 IntelliSense 红线判断编译失败。
5. 成功后，使用现有 `EmberTrial.uproject` 重新打开 UE 5.5.4。

若 solution 没有更新，可关闭 VS，使用 `.uproject` 右键的 **Generate Visual Studio project files**，再打开生成的 solution。若文件菜单没有此项，直接反馈，不必重新安装引擎。

本指南提供参考代码，**尚未在你的工程中创建或编译这个新类**。以你本轮正常 Build 和后续日志为验收依据。

## 6. 将组件挂到玩家蓝图

1. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。
2. 左上 **Components → Add**，搜索 `ETPracticeActionComponent` 或 `ET Practice Action`。
3. 添加刚创建的 C++ 组件，实例名称改为 **PracticeAction**。
4. 确认 Components 列表只有一个 PracticeAction。它没有 Location / Rotation / Scale 是正常的：ActorComponent 不需要空间变换。
5. Compile、Save。

当前由 Blueprint 装配这一个组件；不要同时在 Character 构造函数里再 CreateDefaultSubobject，也不在 BeginPlay 调用 Add Component。

### 6.1 检查蓝图查询入口

1. 将 Components 列表中的 **PracticeAction** 拖到 Event Graph 空白处，得到组件引用 Get 节点。
2. 从它的蓝色输出拖线，搜索 **Get Practice Action State**，放置节点；Target 应自动接到 PracticeAction。
3. 再从 PracticeAction 拖线，搜索 **Is Practice Action Idle**。
4. 这两个都是绿色纯查询节点，没有白色执行引脚。它们的输出暂时不接已有事件，放在 Comment `M0-05C2 Component Queries` 内即可。
5. Compile、Save。下一批会真正使用这些查询；当前无需给 BeginPlay 加 Sequence，也无需重新找 EndPlay。

如果搜索不到：先确认新 C++ Build 成功、已重开编辑器、从组件引用拖线且勾选 Context Sensitive。仍没有就反馈截图，我接手核查；不要改成在 Actor 的 Self 上调用。

## 7. Play 与回归

### 7.1 组件初始化

回 ThirdPersonMap，Play。Output Log 搜索 `[M0-05C2]`，应出现一条：

```text
LogETPracticeAction: Display: [M0-05C2] Component ready Owner=BP_ThirdPersonCharacter_C_... State=Idle
```

Owner 的实例后缀可能不同，不要求逐字一致。这里是组件的 BeginPlay 自动执行，不需要你在角色蓝图再调用一次。

停止再 Play，一次新增一条；连续三次应对应三条新的初始化记录。若一次 PIE 有两条同角色初始化，检查是否重复装配；本轮使用单玩家，不增加 Number of Players。

### 7.2 保持现有功能

站定分别按 1/2/3，动作应仍播放并回到待机。现有 Montage 实际名称带 `_Montage` 后缀，日志例如 `AM_M0_Attack_01_Montage` 是正确结果，无需为对齐旧示例重命名。

复验 E、P、移动 / 转向 / 跳跃。若 C1 的长按单次、动作期间不重启、移动 / 空中拒绝、暂停恢复尚未测完，在此补做 [C1 第 6 节](M0_05C1_MontagePlayback.md#6-分组验收)。不要只以“动画能播”替代所有边界项。

本次不要切断 `IA_PracticeAttack / Dodge / Hit → TryPlayPracticeMontage` 的原连线，也不改 ABP_Manny、AS 或 IMC。新组件接管输入将在 C2-02 一次切换，避免一个按键同时走两套播放逻辑。

## 8. 反馈与出错时提供什么

```text
M0-05C2-01 完成 / 卡在第 __ 节
新类文件路径：默认路径 / 实际路径
UE 关闭后的 C++ Build：成功 / 错误原文
BP_ThirdPersonCharacter 仅挂一个 PracticeAction：是 / 实际结果
两个 BlueprintPure 查询节点：能找到 / 实际结果
角色蓝图 Compile：通过 / 错误原文
每次 Play 一条 Component ready，连续三次：通过 / 实际日志
原 1/2/3、E/P、移动/转向/跳跃：正常 / 实际结果
C1 边界回归：全部通过 / 未通过的具体项目
资产与源码已保存：是 / 否
实际用时：约 __ 分钟
```

| 问题 | 提供材料 |
|---|---|
| C++ Build 失败 | Output → Build 中第一条真实错误及前后约 20 行文字；附 .h/.cpp 路径。不要只截 Error List 或只发最后的 MSB3073 |
| Add 菜单找不到组件 | Add 搜索截图、是否重开过 UE、最后一次 Build 结果 |
| 查询节点找不到 / 蓝图 Compile 失败 | 组件引用到搜索菜单的截图，或完整编译错误文字 |
| 初始化零条 / 重复日志 | `[M0-05C2]` 相关文字 + Components 列表截图，注明一次还是多次 Play |
| 动画 / 暂停 / 输入有时才异常 | 按键顺序文字；必要时 5–10 秒短视频 |

基础步骤卡住约 15–20 分钟就反馈，不必反复删类重建。我会直接检查本地代码和日志；跨系统问题由我处理。

完成本批后把结果发来，我会接手 C2-02 的内部实现，再给你具体的资源赋值与输入改线步骤。
