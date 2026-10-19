# M1-03A2-01：生命属性与 GameplayReady

2026-10-19第18批清理同步：Character门控、Ready / ActorInfo与结束流程以当前Source为准，勿用早期完整函数覆盖已有连段、闪避或治疗。本轮清理由助手按授权直接执行；普通功能开发保留手动实践约定。当前检查与提交清单见[Cleanup](Cleanup.md)。

**2026-10-03已验收通过，实际手动30 / 等待5 min，A2-02也已通过。** 本页保留当时操作方案；实施结果见 [01验收](Reports/M1_03A2_01_Verification.md) / [A2总验收](Reports/M1_03A2_Verification.md)，当前功能基线已到M2-04B2，正在分批清理，状态见[Progress](Progress.md)。下文“尚未编码”等表述属于交接时状态。

原方案日期：2026-10-03，前置：[A1已验收](Reports/M1_03A1_Verification.md)。本页历史方案的原手动预算为90–120分钟，实际结果见上方报告；接口以工程固定的UE5.5.4为准。

## 1. 本批范围与规则

A2 拆成两批：本页完成 Health / MaxHealth、一次初始化、就绪查询 / 通知；下一批 A2-02 用 Gameplay Effect 验证扣血、治疗、最大生命变化与初始化不重置。Presenter / HUD 在 B 接入。

| 规则 | 本批决定 |
|---|---|
| 数值来源 | Character 持有一个 UETHealthAttributeSet，ASC 管理其运行时属性；Widget / Character 不另存实时 Health |
| 默认初值 | InitialHealth=100、InitialMaxHealth=100，可在角色 Class Defaults 配置 |
| 合法范围 | MaxHealth≥1，Health 在 [0,MaxHealth]；降低最大值时压低越界生命，增加最大值不自动治疗 |
| 初始化 | 校验配置后经 ASC 设置基础值，一次成功后重复请求只读现有状态；重新 Possess 不初始化属性 |
| 就绪 | 属性 / ASC 关联正确且初值设置完成后 Ready=True；EndPlay 撤销。Ready 表示初始化完成，不表示当前受控制或生命大于 0 |
| 事件 | 使用 **OnGameplayReadyChanged(Character,bReady)** 与 IsGameplayReady 查询；订阅后立即读查询，覆盖事件早于订阅的情况 |

本批启动值通过 ASC 的 SetNumericAttributeBase 初始化，这是初始化入口；战斗变化在 A2-02 / M1-04 使用 Gameplay Effect。初始配置不是第二份实时生命，后续统一角色配置时可迁入数据资产。现在没有伤害 / 死亡能力或血条。

官方 [属性 / AttributeSet 概览](https://dev.epicgames.com/documentation/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system-in-unreal-engine) 介绍属性职责；实现以本机 5.5.4 为准。这里只使用已存在的 GetSet 查询并校验注册，不把 GetSet 当成创建 API。

## 2. 准备与文件清单

UE Save All / 退出 PIE / 关闭编辑器；CLion 保存所有文件。

| 文件 | 操作 |
|---|---|
| `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Attributes/ETHealthAttributeSet.h` | 手动创建目录 / 文件，全文如下 |
| `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Attributes/ETHealthAttributeSet.cpp` | 手动创建目录 / 文件，全文如下 |
| [EmberTrialCharacter.h](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h) | 插入字段 / 查询 / 委托声明 |
| [EmberTrialCharacter.cpp](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp) | 创建属性集、初始化 / 查询、就绪撤销 |

没有新模块；保留 A1 插件 / Build.cs / ASC / 接口和 UI 文件。不要在 UE 再 Add 一个 AttributeSet 组件：AttributeSet 是 UObject，不是 ActorComponent。

## 3. 新建 AttributeSet 的两份文件

### 3.1 ETHealthAttributeSet.h 全文

当前源码：[ETHealthAttributeSet.h](../EmberTrial/Source/EmberTrial/Public/Attributes/ETHealthAttributeSet.h)。

仅公开属性 / 数值查询，初始化宏保持私有；运行修改仍经ASC / GE。

初始化宏留在 private，仅构造函数使用；没有公开任意写值的 Blueprint setter。属性不是普通 float，不能通过 Set 变量绕过 ASC。generated.h 必须是最后一条 include。

### 3.2 ETHealthAttributeSet.cpp 全文

当前源码：[ETHealthAttributeSet.cpp](../EmberTrial/Source/EmberTrial/Private/Attributes/ETHealthAttributeSet.cpp)。

Base / Current共用夹限，死亡Tag阻止回填生命，上限上升不自动治疗。

构造时 0/100 是尚未完成启动初始化的安全值，不是玩家初始生命。PreAttributeBaseChange 管基础值，PreAttributeChange 管当前值；最大生命下降后经 ASC 调整生命。这里不触发死亡或播放动画。

真实 Gameplay Effect / 最大生命增减的完整验证在 A2-02；现在不能把钩子已写等同于所有 Effect 边界已通过。Health 暂只采用瞬时资源修改，不设计持续 Health 加成。

## 4. Character 头文件

### 4.1 前向声明与委托类型

在现有 `class UAbilitySystemComponent;` 附近添加：

~~~cpp
class UETHealthAttributeSet;
class AEmberTrialCharacter;
~~~

在原 DECLARE_LOG_CATEGORY_EXTERN 之后、UCLASS 之前添加：

~~~cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FETGameplayReadyChangedSignature, AEmberTrialCharacter*, Character, bool, bReady);
~~~

这是委托类型声明，不能写进 GENERATED_BODY 或函数中。

### 4.2 私有字段

在 A1 的 `bAbilitySystemShuttingDown` 附近、public: 之前添加，保留 A1 原字段 / 函数：

~~~cpp
    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Health")
    TObjectPtr<UETHealthAttributeSet> HealthAttributes;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "1.0"))
    float InitialMaxHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "0.0"))
    float InitialHealth = 100.0f;

    bool bStartupAttributesInitialized = false;
    bool bGameplayReady = false;

    void SetGameplayReady(bool bNewReady);
~~~

ClampMin 只是编辑器辅助，真正的配置合法性仍由 C++ 判断。私有 bool 只记录初始化 / 就绪，不存生命。

### 4.3 public 查询 / 命令 / 通知

在 GetAbilitySystemComponent 的声明之后插入：

~~~cpp
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool IsGameplayReady() const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool GetGameplayHealthSnapshot(float& OutHealth, float& OutMaxHealth) const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Gameplay")
    bool EnsureGameplayInitialized();

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Gameplay")
    FETGameplayReadyChangedSignature OnGameplayReadyChanged;
~~~

Ensure 是受控初始化命令，不是补血命令。Snapshot 返回值 False 时输出 0/0 表示无有效快照，不能拿这两个占位数值显示成玩家生命。后续 Presenter 使用查询 / 通知，实际数值仍来自属性集。

## 5. Character .cpp

### 5.1 include / 构造函数

在现有 include 中添加（其他正确的 A1 include 保留）：

~~~cpp
#include "Attributes/ETHealthAttributeSet.h"
~~~

在构造函数 A1 的 SetIsReplicated(false) 后插入：

~~~cpp
    HealthAttributes = CreateDefaultSubobject<UETHealthAttributeSet>(TEXT("HealthAttributes"));
~~~

ASC 的 InitializeComponent 会搜索同一 Actor 的原生属性子对象并注册。不要再 NewObject / 手动 Add 第二份同类型 Set；下方 GetSet 会核对这同一份实例。

### 5.2 BeginPlay 增加一行

在已有 `RefreshEmberAbilityActorInfo(TEXT("BeginPlay"));` **之后**添加：

~~~cpp
    EnsureGameplayInitialized();
~~~

**不在 PossessedBy / UnPossessed 里增加这行，不重置 bStartupAttributesInitialized。** 保留现有 Super / PracticeAction 查找 / 结束保护。

### 5.3 当前查询与一次初始化

完整实现见[Character.cpp](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)，声明见[Character.h](../EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h)；不再保留只检查Health的旧函数，避免覆盖M2体力接入。

- IsGameplayReady检查成功标记、结束标记及ASC / 两属性集；不按生命、控制权或动作过滤。
- 两种Snapshot不可用先清空输出，可用时读取当前属性，不返回出生配置。
- SetGameplayReady对相同状态去重，先提交标记再通知，允许观察者同步查询。
- EnsureGameplayInitialized先核对ActorInfo与两个已注册属性集；已成功时只读。两组初值全部合法后，分别先上限后当前值，完成才提交成功标记 / Ready。

Ready目前表示生命 / 体力初始化成功。能力在BeginPlay后续步骤授予，不能据此认为所有玩法内容已加载；未来异步就绪条件需要单独设计与验证。

### 5.4 EndPlay 撤销就绪

在现有 EndPlay 中，紧接 `bAbilitySystemShuttingDown = true;` 后添加：

~~~cpp
    SetGameplayReady(false);
~~~

后面的 CancelAllAbilities / RemoveAllGameplayCues / ClearActorInfo / Super 全部保留。先通知不可用，再清理数据关联；已经随 Controller 结束的订阅者可能收不到事件，所以后续 UI 也有自己的 EndPlay / Pawn 解绑流程。

## 6. 正常构建与索引

四份文件保存、UE 保持关闭：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

不加 -SkipBuild。成功后 CLion Reload CMake Project，两个新文件应属于 EmberTrialCodeModel，AttributeSet.h / 本项目新头文件能导航。再执行 Game Build：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

不使用 Live Coding 替代。错误先发第一个具体 error、文件 / 行号和上下文，不只发 UBT exit code。

## 7. 初始配置与临时查询

### 7.1 Class Defaults

重开 UE / BP_ThirdPersonCharacter，父类不变；Class Defaults 搜索 Initial：**Initial Max Health=100、Initial Health=100**。AbilitySystem 仍只有一个，Default Starting Data 留空，不重复添加本属性集。Compile / Save。

Play 新地图：先有 A1 的 ActorInfo ready，然后 Health initialized=100/100、Ready changed Ready=1 各一条。状态面板仍只显示 Pawn，不应期待本批出现血条。Stop：Ready=0 在 ActorInfo cleared 前。

### 7.2 Controller 临时数字 8：只读快照

在 BP_ETPlayerController 放顶部 8，使用 Released：

~~~text
8 Released → Is Valid（Input Object=Get Controlled Pawn，Target=Self）
  Is Not Valid → Print String：No current Pawn
  Is Valid → Cast To BP_ThirdPersonCharacter（Object=同一个当前 Pawn）
            成功 → Branch（Condition=Get Gameplay Health Snapshot 的 Return Value）
              True → Print String：有效快照
              False → Print String：[M1-03A2-01] Snapshot unavailable
~~~

Get Gameplay Health Snapshot 是纯节点，**Target 接 Cast 的 As BP Third Person Character**。有效快照 Print 的 In String 接 Format Text：

~~~text
[M1-03A2-01] Snapshot Ready=True Health={Health} MaxHealth={MaxHealth}
~~~

Format Text 的 Health 接 Snapshot 的 Out Health，MaxHealth 接 Out Max Health；Text → String 按自动转换连接。各 Print String 的 Print to Log=True。Cast Failed 可 Print `Unexpected Pawn type`；没有 Tick、Bind Text 或直接 Set Health。

### 7.3 Controller 临时数字 9：重复初始化请求

`9 Released → Is Valid（当前 Pawn）→ Cast To BP_ThirdPersonCharacter → Ensure Gameplay Initialized → Print String`。

Ensure 的 Target 接 Cast 输出。Print 可以只写 `[M1-03A2-01] Ensure requested; press 8 for snapshot`，Print to Log=True；成功 / 失败以 Ensure Return Value、原生日志和随后 8 的查询为准。无效 / Cast Failed 不调用 Ensure。

Compile / Save；不要在 Blueprint BeginPlay 再调用 Ensure，不重复创建 ASC / 属性集。

## 8. 分组验收

1. **默认值**：100/100，新地图 Play 后按 8 正确；按 9 三次，只出现 Already initialized，Health initialized / Ready=1 不重复；再按 8 仍是有效快照。
2. **可配置值**：Stop，角色 Class Defaults 改 Initial Health=40 / Initial Max Health=80，Compile / Save；新 Play / 8 为 40/80，9 不改变它。Stop。
3. **拒绝非法配置**：改 Initial Health=100 / Initial Max Health=60，Compile / Save；新 Play 出现 **Invalid startup health** Warning，**没有 Health initialized / Ready=1**；按 8 Snapshot unavailable，按 9 仍拒绝。A1 / 原型仍可能可运行，不能因此称 GameplayReady 成功。这个 Warning 是本用例的预期结果，不是需要绕过的报错。
4. **恢复配置**：Stop，恢复 100/100、Compile / Save；连续三次 Play / Stop，出生正确、每轮 Ready=1 / Ready=0 各一次，清理后不重新就绪，无 Accessed None / Missing gameplay dependencies。

本批的重复 9 验证通知 / 初始化入口幂等；**属性受到扣血后是否不被重置、真实 Effect 的范围和最大生命策略在 A2-02 再测**。不凭初值相同宣称所有动态边界已验证。GameplayReady 的真实 UI 订阅 / 解绑在 B 验证，本批不要求额外搭一份 HUD。

## 9. 清理、回归与反馈

全部通过后 Stop，Controller 删除 8/9 临时事件及相关 Cast / Print 节点，Compile / Save；角色 Initial 保持100/100，保留 ASC / 属性集 / 就绪代码。回新地图简测原 1/2/3、2→3、E-A/B、P 鼠标 / 键盘 / 背景关闭、无补播 / 卡键、移动 / 转向 / 跳跃。Stop / Save All。

~~~text
M1-03A2-01 完成 / 卡在第 __ 节
AttributeSet 两文件 / 角色四处接入与新函数已保存：是
关闭 UE 后 Editor Build / 索引刷新 / Game Build：成功
两个新文件目标 / 新头文件导航：通过
单个 ASC / 无重复属性集，初始配置100/100：是
出生 Health initialized / Ready=1 各一次；8 快照：通过
重复9三次：Already initialized，无重复初始化 / Ready=1：通过
配置40/80快照：通过
非法100/60拒绝，Warning符合、快照不可用：通过
恢复100/100，三次PIE与Ready撤销 / 清理：通过
Controller临时8/9已删除，Compile / Save：是
最终原1/2/3、2→3、E/P、移动 / 转向 / 跳跃：正常
源码 / 资产已保存：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

报错先给章节 / 操作顺序 / 错误文字；连线问题截包含执行线和数据引脚的图。我会读本机日志复核，只有明确要求时才直接改源码。

## 本次清理 Commit / Push（固定放在最后）

本页历史功能已完成，不重做旧步骤。第18批为Character两份Source与八份Markdown，共十项；最新差异、检查结果与逐文件清单见[Cleanup末尾](Cleanup.md)。用户重新Stage后Commit / Push，Raw / 索引 / 日志留本地；助手不操作Git索引。
