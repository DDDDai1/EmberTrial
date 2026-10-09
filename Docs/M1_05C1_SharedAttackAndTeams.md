# M1-05C1：共享攻击支持离线AI、阵营过滤与玩家真实受伤

下发：2026-10-08；验收：2026-10-09。固定 **UE5.5.4 / 离线单人 / 键鼠**。**本批已由用户手动实施并验收通过。**[验收报告](Reports/M1_05C1_Verification.md)区分源码 / 构建 / 直接日志与用户实测反馈；当前51Source / 25cpp / 40资产见[最终基线](Reports/Raw/M1_05C1_Baseline.json)。手动35（代码10 / 资产10 / 检查15）、等待5分钟；项目累计1486 / 等待115。助手仅复核及更新文档，未改Source / UE资产、Build或操控编辑器。以下保留已执行步骤，无需重做；C2自动转向 / BT攻击待下发。

## 1. 本批成果、分工与时间

C1完成后，玩家与敌人可使用同一个原生轻攻击实现：玩家仍左键攻击，敌人配置自己的GA资源后，通过引擎控制台请求一次攻击。敌人的拳轨迹在原8–14帧窗口命中玩家时，现有GE使玩家扣10血，自动受击并更新HUD；同一次攻击只结算一次。下一次命中可再次扣10。玩家继续使用25伤害，训练目标继续可被玩家击打。

阵营由共享身体的枚举配置决定；伤害层读取身体的公开关系查询，不读取UI或特定AIController / Blackboard。蓝图只装配阵营、GA和碰撞资源；**本批不需要新增任何Event Graph节点、临时按键或Controller测试函数**。ASC、CombatLife、CombatTrace各保持一个。

C分两批推进：

| 批次 | 内容 | 当前状态 |
| --- | --- | --- |
| **C1，已验收** | 单人权威攻击兼容AI、最小阵营过滤、敌人独立GA、玩家命中碰撞；真实受伤 | 用户全项完成；源码 / 构建记录 / 关键日志已复核，证据范围见验收报告 |
| C2 | 停步转向、攻击距离 / 节奏、原生BT攻击任务；按能力结束事件完成任务，目标失效 / 受击取消与恢复 | 后续提供代码；保持当前B2树，不提前加攻击节点 |

预算包含阅读与理解：代码30–45分钟、资源配置15–25分钟、新增与直接相关检查10–15分钟，合计约**55–85分钟**；Build / 索引等待另记。按实际反馈计时，不把预算计入已完成工时。

本批仍是原地拳击占位。近距离真实命中先证明链路，C2再根据实际拳轨迹校准自动停步与攻击范围；不能把“动画播了”当作伤害成功，也不能只扩大Trace半径掩盖朝向 / 距离问题。

## 2. 修改范围与规则

仅修改以下**三个既有文件**，没有新增C++文件 / 模块 / 依赖；预计索引仍为**25cpp**。

| 文件 | 手动修改 |
| --- | --- |
| [EmberTrialCharacter.h](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h) | 增加枚举、一个可配置阵营属性、两个公开查询声明 |
| [EmberTrialCharacter.cpp](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp) | 增加两个查询实现，其余身体 / 输入 / 生命周期保留 |
| [ETGA_LightAttack.cpp](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp) | 两个include、执行策略一行、完整替换CanActivateAbility、伤害前加阵营检查 |

`ETGA_LightAttack.h`、AIController、BT Service / Move任务、Trace、HitReact、CombatLife、AttributeSet、HUD、Build.cs保持现状。`RequestLightAttack()`与`CancelLightAttack()`已可复用，不新增第二套AI攻击入口。

### 2.1 最小阵营关系

| 来源 \ 目标 | Neutral | Player | Enemy |
| --- | --- | --- | --- |
| Neutral | 拒绝 | 拒绝 | 拒绝 |
| Player | 允许，供现有训练目标 | 拒绝 | 允许 |
| Enemy | 拒绝 | 允许 | 拒绝 |

Neutral在本阶段表示未分配战斗方 / 训练身体。玩家对Neutral的允许是现有训练靶约定，不代表以后所有中立NPC都能受伤；有中立NPC时再细分角色与目标资格。目前生命、Ready、死亡、ASC与遮挡检查继续独立生效，阵营允许并不等于一定结算伤害。

这是**战斗伤害分类**，不等同AI Perception的GenericTeamID / affiliation；C1保留B2只感知有效本地玩家的规则。C2的自动攻击决策再使用同一关系查询。关系属于身体，不依据当前Controller是否存在推断敌友；训练身体没有Controller仍可被玩家命中。

### 2.2 单人执行边界

轻攻击改为`ServerOnly`，在本项目的Standalone世界里，玩家与AI都在同一个权威世界执行。CanActivate同时校验`NM_Standalone` / Authority，玩家分支保留本地Controller与IgnoreMoveInput，AI分支要求AAIController正确控制Enemy身体；两者保留地面 / 速度、GAS标签、Practice与Montage门控。

这不是联网实现。本批不提供客户端预测 / RPC；Editor的Play设置保持**1名玩家、Play Standalone**，不使用Play As Client / Listen Server。以后若做联网，再整体设计输入、预测、复制与服务器结算，不能直接删掉校验。

## 3. EmberTrialCharacter.h：三处新增

先保存工程资产并关闭UE，CLion可保持打开。新增反射枚举 / 属性后走正常Build，不使用Live Coding。下面都是**插入**，保留原文件其他内容；不要把片段覆盖整个头文件。

### 3.1 枚举：放在UCLASS外

找到文件里的`UCLASS(config=Game)`。在它**前面**、已有日志 / 委托声明**后面**插入：

```cpp
UENUM(BlueprintType)
enum class EETCombatTeam : uint8
{
    Neutral UMETA(DisplayName = "Neutral"),
    Player  UMETA(DisplayName = "Player"),
    Enemy   UMETA(DisplayName = "Enemy")
};
```

顺序应为：原include（`EmberTrialCharacter.generated.h`仍是最后一个include）→ 原前置声明 / 委托 → 上述枚举 → 原UCLASS / Character类。**不要放在Character类的大括号里面，不新增generated.h。**

### 3.2 private属性：紧接CombatLifeComponent

当前类在第一个`public:`以前为private。找到原有这两行末尾：

```cpp
TObjectPtr<UETCombatLifeComponent> CombatLifeComponent;

public:
```

仅在`CombatLifeComponent;`与`public:`之间插入：

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EmberTrial|Combat",
    meta = (AllowPrivateAccess = "true"))
EETCombatTeam CombatTeam = EETCombatTeam::Neutral;
```

原CombatLife的UPROPERTY保持原样；新属性有自己独立的UPROPERTY。默认Neutral，所以**Build后一定执行第6节，把正式玩家设Player、敌人设Enemy**。

### 3.3 public查询：紧接CancelLightAttack

找到public区原有的：

```cpp
UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
void CancelLightAttack();
```

在它后面、原RequestHitReact声明前插入：

```cpp
UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
EETCombatTeam GetCombatTeam() const;

bool CanDealCombatDamageTo(const AEmberTrialCharacter* TargetBody) const;
```

GetCombatTeam可供蓝图 / 后续开发工具读取；CanDealCombatDamageTo供原生战斗 / AI共享，当前不需要蓝图调用。不要改原RequestLightAttack / CancelLightAttack / Ready委托签名。

## 4. EmberTrialCharacter.cpp：增加两个完整实现

在原`CancelLightAttack()`完整函数**结束大括号之后**、`RequestHitReact()`之前插入以下两个函数。当前头文件已有足够类型依赖，**此cpp无需新增include**；尤其不要在其他函数的大括号里插入。

```cpp
EETCombatTeam AEmberTrialCharacter::GetCombatTeam() const
{
    return CombatTeam;
}

bool AEmberTrialCharacter::CanDealCombatDamageTo(
    const AEmberTrialCharacter* TargetBody) const
{
    if (!IsValid(TargetBody) || TargetBody == this ||
        IsActorBeingDestroyed() || TargetBody->IsActorBeingDestroyed())
    {
        return false;
    }

    switch (CombatTeam)
    {
    case EETCombatTeam::Player:
        return TargetBody->GetCombatTeam() == EETCombatTeam::Enemy ||
            TargetBody->GetCombatTeam() == EETCombatTeam::Neutral;
    case EETCombatTeam::Enemy:
        return TargetBody->GetCombatTeam() == EETCombatTeam::Player;
    default:
        return false;
    }
}
```

这个查询只做目标身份 / 阵营判定，**不扣血、取消技能、初始化属性或写黑板**。GetGameplayHealthSnapshot / CombatLife继续负责其已有的状态规则。

## 5. ETGA_LightAttack.cpp：四处修改

### 5.1 新增两个include

在文件头原include区域中插入下面两行，原include全部保留。可放在`GameFramework/PlayerController.h`附近；不要放在DEFINE_LOG_CATEGORY或函数内部。

```cpp
#include "AIController.h"
#include "Engine/World.h"
```

这是引擎AAIController头文件，**不是**本工程的`AI/ETAIController.h`；GA不依赖具体敌人Controller。核心已有AIModule依赖，无需改Build.cs。

### 5.2 构造函数：仅替换执行策略一行

在`UETGA_LightAttack::UETGA_LightAttack()`内，将：

```cpp
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
```

替换为：

```cpp
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
```

InstancingPolicy=InstancedPerActor、ReplicationPolicy=ReplicateNo、bRetriggerInstancedAbility=False、全部Asset / Owned / Blocked Tags保持原样。**蓝图GA默认值还需要第6.1 / 6.2节分别核对，不能假设旧资产一定自动跟随原生默认值。**

### 5.3 完整替换CanActivateAbility

从原`bool UETGA_LightAttack::CanActivateAbility(...)`函数签名开始，到它自己的结束`}`为止，整体替换为下列函数。下一个`ActivateAbility()`函数不改。

```cpp
bool UETGA_LightAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !Super::CanActivateAbility(
        Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    AEmberTrialCharacter* BodyCharacter =
        Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    if (!IsValid(BodyCharacter) || BodyCharacter->IsActorBeingDestroyed() ||
        BodyCharacter->IsCombatDead() || !BodyCharacter->IsGameplayReady() ||
        ActorInfo->OwnerActor.Get() != BodyCharacter ||
        BodyCharacter->GetAbilitySystemComponent() !=
            ActorInfo->AbilitySystemComponent.Get())
    {
        return false;
    }

    UWorld* World = BodyCharacter->GetWorld();
    if (!IsValid(World) || World->GetNetMode() != NM_Standalone ||
        !BodyCharacter->HasAuthority() ||
        UGameplayStatics::IsGamePaused(BodyCharacter))
    {
        return false;
    }

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) ||
        MaxHealth < 1.0f || Health <= 0.0f || Health > MaxHealth)
    {
        return false;
    }

    AController* BodyController = BodyCharacter->GetController();
    if (!IsValid(BodyController) || BodyController->GetPawn() != BodyCharacter)
    {
        return false;
    }
    if (const APlayerController* PlayerController =
        Cast<APlayerController>(BodyController))
    {
        if (!PlayerController->IsLocalController() ||
            PlayerController->IsMoveInputIgnored() ||
            BodyCharacter->GetCombatTeam() != EETCombatTeam::Player)
        {
            return false;
        }
    }
    else if (!Cast<AAIController>(BodyController) ||
        BodyCharacter->GetCombatTeam() != EETCombatTeam::Enemy)
    {
        return false;
    }

    UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement();
    const FVector Velocity = BodyCharacter->GetVelocity();
    if (!IsValid(Movement) || !Movement->IsMovingOnGround() ||
        Velocity.ContainsNaN() || !FMath::IsFinite(AttackMaxHorizontalSpeed) ||
        AttackMaxHorizontalSpeed < 0.0f ||
        Velocity.SizeSquared2D() > FMath::Square(AttackMaxHorizontalSpeed))
    {
        return false;
    }

    const UETPracticeActionComponent* Practice =
        BodyCharacter->FindComponentByClass<UETPracticeActionComponent>();
    if (Practice && !Practice->IsPracticeActionIdle())
    {
        return false;
    }

    if (!HasValidDamageConfiguration())
    {
        UE_LOG(LogETLightAttack, Warning,
            TEXT("[M1-04B2] Invalid damage config Character=%s Effect=%s Damage=%.1f"),
            *GetNameSafe(BodyCharacter), *GetNameSafe(DamageEffectClass.Get()),
            AttackDamage);
        return false;
    }

    const UETCombatTraceComponent* Trace =
        BodyCharacter->FindComponentByClass<UETCombatTraceComponent>();
    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    return IsValid(AttackMontage) && IsValid(AnimInstance) &&
        IsValid(Trace) && Trace->CanStartAttack() &&
        AnimInstance->GetCurrentActiveMontage() == nullptr;
}
```

玩家分支仍校验暂停、输入锁、地面、速度与Practice；AI不使用PlayerController的本地输入锁，但同样需要空闲标签 / 地面 / 速度 / Montage合法。移动中直接调敌人RequestLightAttack被拒绝是正确的；第7节先让敌人停步再请求。

### 5.4 HandleTraceHit：插入阵营拒绝分支

在`HandleTraceHit(...)`内找到原有以下部分：

```cpp
AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Hit.GetActor());
if (!IsValid(Target) || Target == Source)
{
    Reject(TEXT("UnsupportedTarget"));
    return;
}

if (Target->IsCombatDead())
```

只在`UnsupportedTarget`的if结束之后、`if (Target->IsCombatDead())`之前插入：

```cpp
if (!Source->CanDealCombatDamageTo(Target))
{
    Reject(TEXT("CombatTeam"));
    return;
}
```

后面的TargetDead、Ready / Health、ASC归属、Spec / Context、SetByCaller、Apply、Current / Base日志与弱引用保护全部保留。不能将这个if放在Apply之后；那样伤害已经发生。`SeenTargets`仍由Trace负责本次攻击去重，伤害层只判定是否合法，不重写Sweep。

## 6. 正常Build与Blueprint资源配置

### 6.1 Build、索引和旧玩家GA

1. 确认UE已关闭，三个源文件保存。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

该脚本先做一次正常Editor Build，再生成CLion索引，不另重复Editor Build。返回成功后，在CLion Reload CMake Project；两个CodeModel仍可用，项目cpp数仍25。

2. 同一PowerShell执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

3. 两个目标成功后再打开UE。原`BP_ThirdPersonCharacter`父类保持，`Combat Team`字段 / `Get Combat Team`查询应可见；本批无需创建查询图。失败发**第一条具体UHT / C++错误及前后5–10行文本**，不要只发exit code 6，不先清缓存。
4. 打开`/Game/EmberTrial/Abilities/GA_ET_LightAttack`，点击**Class Defaults**。清空Details搜索，展开Advanced；搜索`Net Execution Policy`也可。设**Server Only / ServerOnly**，不能保留Local Only / Local Predicted。本机5.5.4该属性是Advanced / EditDefaultsOnly，位置已从引擎头文件核对。
5. 同一旧玩家GA确认：Attack Montage=`AM_M0_Attack_01_Montage`、Attack Damage=**25**、Damage Effect Class=`GE_ET_LightAttackDamage`、Attack Max Horizontal Speed=**3**；Instancing Policy=Instanced Per Actor、Replication Policy=Do Not Replicate、Retrigger Instanced Ability=False。不新增蓝图ActivateAbility或CanActivateAbility覆盖，不改原Tags。**Compile → Save**。

### 6.2 创建敌人独立GA，原生父类直接新建

1. Content Browser打开`/Game/EmberTrial/Abilities`。空白处右键 → **Blueprint Class → All Classes** → 搜索并选择原生**ETGA_LightAttack**，命名`GA_ET_EnemyLightAttack`。不要选已有`GA_ET_LightAttack`当父类。
2. 打开新GA，Class Defaults逐项配置：

| 属性 | 配置 |
| --- | --- |
| Parent Class | ETGA_LightAttack |
| Attack Montage | `/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Attack_01_Montage` |
| Damage Effect Class | `/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage` |
| Attack Damage | **10** |
| Attack Max Horizontal Speed | **3 cm/s** |
| Net Execution Policy | **Server Only** |
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Do Not Replicate |
| Retrigger Instanced Ability | False |

3. Event Graph不加ActivateAbility / CanActivateAbility / Delay / 扣血节点，Tags使用原生继承配置。**Compile → Save**。玩家 / 敌人引用同一个Montage资产可以各自播放，每个身体的AnimInstance、Ability实例与Trace序号独立。

### 6.3 玩家Class Defaults与命中碰撞

1. 打开**原`BP_ThirdPersonCharacter`**，Class Defaults搜索`Combat Team`，设为**Player**。Light Attack Ability Class仍为旧玩家`GA_ET_LightAttack`；100/100、原输入与Practice保持。
2. Components选中**继承CapsuleComponent**，清空Details搜索，展开Collision。先记录目前响应表，再将Collision Presets设为**Custom**。
3. 核对以下最终配置；**不是先把全部通道Ignore**：

| 属性 | 最终值 |
| --- | --- |
| Collision Enabled | Query and Physics |
| Object Type | Pawn |
| WorldStatic / WorldDynamic / Pawn | Block / Block / Block |
| **Visibility** | **Overlap**，这是本批新增命中入口 |
| Camera | Ignore |
| PhysicsBody / Vehicle / Destructible | Block，其他自定义项保留原值 |
| Simulate Physics | False |
| Capsule Radius / Half Height | 保留原值；模板当前42 / 96 |
| Generate Overlap Events | 保留原值，不以此事件驱动伤害 |

4. 玩家Mesh保持原SKM_Quinn_Simple / ABP_Quinn、相对Transform与**NoCollision**；命中只靠胶囊，不让Mesh再产生一份候选。Visibility改Overlap不取消胶囊对地面、墙和身体的物理阻挡，也不需要制作BeginOverlap蓝图。
5. 继承CombatTrace确认原`hand_r`、Trace Radius=25、Trace Local Offset原值、Draw Debug=False；CombatLife的GA_ET_HitReact保留，两个组件各一个。**Compile → Save**。

### 6.4 敌人Class Defaults

1. 打开原`BP_ETMeleeEnemy`，Class Defaults设**Combat Team=Enemy**、**Light Attack Ability Class=GA_ET_EnemyLightAttack**；Health / MaxHealth=100/100。
2. 单ASC / CombatTrace / CombatLife、GA_ET_HitReact、Quinn / ABP、Mesh NoCollision、Visibility Based Anim Tick Option=Always Tick Pose and Refresh Bones、Capsule Visibility=Overlap均保留；不要Add第二个组件或Practice。
3. AI Controller Class仍BP_ETMeleeAIController，Auto Possess配置保持；CharacterMovement仍MaxWalkSpeed220、Orient Rotation to Movement=True / Use Controller Desired Rotation=False。本批不改朝向策略。
4. **Compile → Save**。原Controller、黑板和B2树保留，**不添加自动攻击节点**。

### 6.5 训练身体与地图

打开`BP_ETCombatTrainingTarget`，Class Defaults确认Combat Team=**Neutral**、Light Attack Ability Class=None；原100/100、受击资源与胶囊保留，Compile → Save。其他Tests身体可保持默认Neutral，本批不运行换控 / UI重建旧矩阵。

正式地图仍一个敌人、一份训练身体、一个导航体，默认Pawn / GameMode不变；不移动地图中的PlayerStart、不缩导航、不放临时墙。本批没有新增关卡对象。**Save All**后进入第7节。

## 7. 四组新增 / 受影响检查（总约10–15分钟）

用[方案B](AcceptanceWorkflow_Options.md)，不开新的Controller测试图。Output Log允许Display级；日志仍沿用`M1-04A / B1 / B2 / C1`与`M1-05B2`，本批没有把旧日志前缀批量改名。

**控制台入口：**PIE视口有输入焦点时按`~`打开**游戏内控制台**，输入命令 → Enter → `~`关闭。不要在CLion终端 / PowerShell或未运行PIE的编辑器命令框里输入`ke`。每条命令只作用于本轮单个正式敌人；若当前无单敌人，先恢复场景，不对多实例批量发命令。

### 7.1 玩家攻击短回归（约2分钟）

**准备：**正式Player / Enemy阵营、两GA ServerOnly、默认100/100。新Play，HUD100。敌人目前只追击，不会自动攻击。

**操作：**站定朝训练目标左键命中一次，训练目标100→75并受击；再对敌人左键命中一次，敌人100→75。简单长按左键确认一次开始，释放后可再请求；走动时点击一次确认拒绝，站定重新点击能开始。

**预期：**旧玩家GA25保持，训练Neutral允许、Enemy允许。每次完整Begin / End、Trace Arm / Window / Disarm；敌人受伤不改变玩家HUD。此组不用打到死亡或重复限帧 / MaxHealth矩阵。

**清理：**Stop PIE，下一组新Play恢复两身体100。

### 7.2 敌人一次真实攻击、玩家受伤与空挥（约3–4分钟）

**准备：**新Play，让敌人看见你并追近至约150cm停步。保持在它正前方，沿它面向的直线再向它靠近少量，直至身体接近但胶囊仍阻挡，松开移动键。这样先用约84–110cm的近处样本证明拳击命中；不要停在拳头打不到的侧后方。身体由B2自然移动到近处，地图坐标不用改。

**操作：**确认敌人已停止行走，打开游戏控制台，只输入一次：

```text
ke BP_ETMeleeEnemy_C RequestLightAttack
```

Enter并关闭控制台，观察它原地播放攻击、玩家受击、HUD**100→90**。等敌人完整收招 / End后，再请求一次；合法命中HUD**90→80**。不连续发送命令造成死亡。

**预期日志：**

```text
[M1-04A] Granted ... Character=BP_ETMeleeEnemy... Class=GA_ET_EnemyLightAttack_C
[M1-04A] Begin Character=BP_ETMeleeEnemy... Montage=AM_M0_Attack_01_Montage
[M1-04A] Request Character=BP_ETMeleeEnemy... Accepted=1
[M1-04B1] Candidate Owner=BP_ETMeleeEnemy... Target=BP_ThirdPersonCharacter...
[M1-04B2] Damage ... Source=BP_ETMeleeEnemy... Target=BP_ThirdPersonCharacter... Requested=10.0 Applied=1 ... Before=100.0 After=90.0 BaseAfter=90.0
[M1-04C1] Hit begin Character=BP_ThirdPersonCharacter...
[M1-04A] End Character=BP_ETMeleeEnemy... Cancelled=0 AttackingTag=0
```

Begin可以早于Request打印，多个事件交错也正常；依据身体名分辨敌人攻击与玩家受击，不按纯日志行顺序猜所有异步行为。`Called ... 1 succeeded`只代表`ke`调用成功，真正接受看Accepted / Begin，伤害看Damage和HUD。Instant GE成功时ActiveHandle可为0，依据Applied与Current / Base变化判断。

随后向侧面走到敌人拳轨迹之外、双方仍在近处停步范围内，让敌人站定；只请求一次。**能播放但没有Candidate / Damage、HUD不变**是正确空挥。本批没有停步自动转向，不把空挥当AI朝向Bug；C2再接自动转向。

**清理：**Stop PIE，下一组新Play恢复100。若有Begin / Window却无伤害，按第8节定位碰撞或轨迹，不反复修改伤害值。

### 7.3 同阵营拒绝（约2–3分钟）

**准备：**Stop PIE。在玩家`BP_ThirdPersonCharacter`的**Class Defaults**临时把Combat Team从Player改为**Enemy**，Compile即可；记录这项临时配置，本组后必须恢复。敌人保持Enemy / Damage10。

**操作：**新Play，走到与7.2相同的敌人正前方近处，敌人停步后只请求一次RequestLightAttack。此轮玩家标签被临时设Enemy，但B2感知仍识别本地玩家；不调用玩家攻击来判断效果。

**预期：**敌人攻击Begin、Trace对玩家产生Candidate，但伤害层输出`Damage skipped ... Reason=CombatTeam`；玩家HUD保持100，没有由这次命中引起的HitReact。同方不可被扣血。必须有Candidate再有拒绝，单纯打空不能证明阵营检查。

**清理：**Stop，立即恢复玩家Class Defaults **Combat Team=Player** → Compile → Save。下一组新Play确认出生100、玩家攻击重新正常。不要把临时Enemy阵营保存为最终状态。

### 7.4 共享取消、暂停鼠标边界与结束（约3–5分钟）

**准备：**已恢复Player阵营、新Play。世界正常导航、单敌人。

**A，敌人受击取消自己的攻击：**先在游戏控制台输入`slomo 0.05`，敌人在近处站定。依次执行以下两条，每条单独Enter，第二条紧跟第一条；按30fps动画，前8帧在慢速下约5.3秒现实时间，足够粘贴操作：

```text
ke BP_ETMeleeEnemy_C RequestLightAttack
ke BP_ETMeleeEnemy_C RequestHitReact
```

看到攻击Cancelled=1、Trace关窗 / Disarm Tick0、敌人Hit begin；受击不能留下攻击检测。若恰好已进入窗口并合法扣10血，保留这次既有伤害，取消后不再扣；不用为了“窗口前”重测。等受击结束后输入`slomo 1`，再请求一次，能正常攻击。角色忙碌过程中反复Request不应重启或排队。

**B，玩家受影响入口：**正常速度下，玩家站定左键攻击后按3，原攻击取消并播放受击；再用2→3一次确认旧侧移被受击中止。只做各一次，不重复三帧率 / 五轮边界。

**C，暂停鼠标边界：**玩家动作中P打开暂停，世界冻结；菜单中左键继续按钮恢复后不补播攻击，Enter / Space继续任选一次，恢复后移动转向正常。敌人C1没有自动攻击循环，此项确认共享玩家输入入口仍正确。

**D，结束与短基线：**一次敌人攻击开始后Stop PIE；下一次新Play玩家 / 敌人100/100，无旧Montage / Trace / AttackingTag残留。最后E-A/B提示 / 激活、P关闭、移动转向跳跃各一个短流程；这轮不重做UI生命周期 / 打包矩阵。

**最终清理：**`slomo 1`；玩家Team=Player / GA25，敌人Team=Enemy / GA10，训练Team=Neutral；两GA ServerOnly，Draw Debug=False，无新临时按键 / 对象。所有改过的Blueprint **Compile → Save**，地图Save All，Stop PIE。

## 8. 报错与未命中如何反馈

| 现象 | 先核对 / 反馈材料 |
| --- | --- |
| UHT / C++失败 | 第一条具体错误文本和前后5–10行；特别核对枚举在UCLASS外、属性private / 查询public、实现没有插进旧函数 |
| 玩家左键完全不开始 | 玩家Team=Player、旧GA ServerOnly、LightAttack Class / 单人Standalone；Request Accepted0附近日志、GA默认值截图 |
| 敌人Request Accepted0 | GA是否授予 / Class，Team=Enemy、ServerOnly、已站定 / 地面、无HitReact或旧Montage；不能只看ke的1 succeeded |
| Begin有、Window没有 | 敌人ABP_Quinn / DefaultSlot、原Montage是否带已验收通知；GA是否引用正确Montage；Begin至End日志 |
| Window有、Candidate没有 | 玩家Capsule Visibility=Overlap / QueryAndPhysics、Mesh NoCollision、拳头实际距离与方向；先靠近到胶囊阻挡附近。可临时把**敌人**CombatTrace Draw Debug=True，拍短视频后恢复False；不擅自扩大半径 / 移Notify窗口 |
| Candidate有、Damage没有 | `Damage skipped Reason`文本、两身体Combat Team、Source / Target名字；保留日志中的真实拒绝理由 |
| 同方测试没有CombatTeam拒绝 | 首先确认本轮确有Candidate，不把空挥算过滤通过；再核对临时改的是玩家Class Defaults而非训练目标 / PIE旧对象 |
| 扣血但无受击 / HUD变化 | 玩家Life observing / Hit request / Presenter快照片段，单CombatLife及GA_ET_HitReact配置；不用重建HUD测试函数 |
| 时序难表达 | 10–20秒短视频；连线问题用完整局部截图。本批没有新增蓝图函数图，不需要为了反馈临时搭图 |

日志由助手根据**完整时分**自行读取，你无需手算时间或整理长日志。需要助手改源码 / 实际操作编辑器时另给明确授权；普通错误仍先给局部修正步骤。

## 9. 完成反馈

```text
M1-05C1 完成
三个既有源码 / Editor Build / 索引25cpp / Game Build：
两GA ServerOnly / 玩家25-Player / 敌人10-Enemy / 训练Neutral：
玩家胶囊Visibility Overlap、Mesh NoCollision，单Trace-Life：
玩家左键对训练 / 敌人各25，移动拒绝与长按单次：
敌人两次真实命中、玩家100→90→80 / 受击 / HUD同步：
侧方空挥不扣血 / 同Enemy阵营Candidate后CombatTeam拒绝：
敌人受击取消与结束可再攻击 / 玩家1→3、2→3：
暂停鼠标 / 键盘恢复无补播 / 动作中停止重开 / 短基线：
最终阵营 / GA数值 / slomo1 / Debug False，全部保存：
失败或特别说明（命中距离 / 方向若有调整）：
大致完成时间（完整时分，例如21:18）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

报告后核对本批新规则与受影响入口，不再要求设计旧Controller临时验收函数。新GA创建后才登记资产，实际Build / 实机通过后才标验收完成。

## 10. 静态核查依据与C2交接

已读取本机5.5.4的`AbilitySystemComponent_Abilities.cpp`：LocalOnly对非本地角色的请求与ServerOnly对Authority的限制在调用CanActivate前就可能生效，所以不能只放宽PlayerController检查而保持旧策略。`GameplayAbility.h`中NetExecutionPolicy为Advanced / EditDefaultsOnly，故旧玩家GA与新敌人GA都需核对资源默认值。`World.h`提供GetNetMode；现有核心Public AIModule已满足AAIController依赖。以上是本机固定版本的静态接口核查，**不是新代码已编译或运行**。

同一Actor已有独立Spec / Montage / Trace生命周期；HitReact原生CancelAbilitiesWithTag会取消轻攻击，CombatLife死亡仍优先封锁 / 关窗，不增加重复逻辑。本批阵营是轻量身体数据，不导入完整队伍服务或GameInstance缓存，不让Ability依赖具体AIController / UMG。

C2将使用已经验证的Enemy GA与Damage10，新增原生BT攻击任务和朝向 / 范围决策。任务按自己启动的Spec结束事件完成，处理同步结束、订阅解绑、Abort时取消自己动作；不靠固定1秒Delay猜Montage结束，也不把整个AAIController的Busy标签直接接成会取消自身攻击任务的Decorator。攻击中固定朝向、收招后重判目标，目标失效 / 死亡与自身受击可中止；失败 / 冷却有明确等待，不能重新出现每帧激活风暴。C2目前仅为后续设计边界，代码尚未下发，不提前修改B2树。

## 11. 实施收尾（2026-10-09）

21:06全项完成、无失败；手动35 / 等待5。三Source / 八个新片段一致，48份未改，双receipt / 25cpp核对；五轮PIE / 11组Begin-End、敌人真实10点伤害及HUD、同方Candidate后拒绝、受击取消 / 暂停 / Stop清理有直接证据。第二次90→80和玩家对敌人25按用户实测通过记录，不冒称现存日志直接覆盖；最终配置已确认恢复。新敌人GA登记后清单40，[C1验收](Reports/M1_05C1_Verification.md)通过、无待补项；无需重复本指南检查。下一批C2尚未下发，继续文档代码 / 手动实现与方案B。
