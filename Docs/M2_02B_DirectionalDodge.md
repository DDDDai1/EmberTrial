# M2-02B：方向闪避位移与2键正式迁移

2026-10-18清理同步：本章保留历史接入步骤、资产配置和验收依据，整文件源码已改为链接仓库当前实现。当前还包含连段恢复闪避与治疗互斥，勿把早期章节片段整批重做或覆盖现有工程。职责与当前时序见[闪避路线页](M2_02_DodgeRoadmap.md)；本次清理提交清单见[Cleanup](Cleanup.md)。

2026-10-15，固定 **UE 5.5.4**。前置：[A已验收](Reports/M2_02A_Verification.md)。**本批已按用户16:48反馈与本地证据验收通过，无待补项。**实际93Source / 48cpp / 来源51项；手动30 / 构建等待5分钟，详见[B验收](Reports/M2_02B_Verification.md)。下方完整操作保留复查，无需重复执行；[C短无敌 / 集成也已验收](Reports/M2_02C_Verification.md)，M2-02整章完成。

原下发预算（非当前待办）：手动 **100–170分钟**（代码 / 理解约70–115、资产与连线10–20、相关检查10–15、保存记录5–10），构建等待另列。可分两次完成；预算不要求做满，也不提前记为已用工时。全部C++仍由你手动录入；有编译问题发第一条真实错误，我再按当次请求处理。

## 当前测试维护边界（第24批）

方向 / 位移：三处方向误差命名为float DirectionTolerance；固定30 / 60 / 120步长、原计时舍入与1cm终点判据保持。

全部原断言、日志 / 预期Warning、测试注册与场景顺序保留；期望值独立于生产默认值。当前源码排版 / 注释与命名以Source为准，历史完整实现不重复覆盖。本批改动和验证结果见[第24批文档](Cleanup.md#cleanup-batch-24)；三批提交清单统一见Cleanup末尾。

## 1. 本批结果与分工

按键盘上方2，使用当前移动输入对应的世界方向闪避；没有移动输入时按**角色前方**，不是相机前方。方向在开始时固定，位移期间转镜头或改WASD不改变这一段轨迹。默认150cm / 0.30世界秒 / 一次25体力；斜向先归一化，不多走一倍。

| 情况 | 本批预期 |
| --- | --- |
| W / S / A / D配2 | 对应镜头水平前 / 后 / 左 / 右 |
| W+A等斜向配2 | 固定斜向，仍150cm |
| 松开全部WASD后按2 | 按角色此刻前方，不能沿上一次输入滑走 |
| 位移完成、Montage还没结束 | 原地等待Montage结束，仍Dodging，不提前移动 / 再闪 / 恢复体力 |
| 墙 / 斜墙 / 可走坡面 | CharacterMovement真实碰撞 / 滑动；墙边实际距离可小于150cm |
| 走出可行走地面 | 位移Task中止并取消动作，不悬空持续冲刺 |
| 3 / 真实受击 / 死亡 / EndPlay取消 | 只清本Task持有的Source，不退款、不残留水平速度 |

**本批仍没有无敌。**敌人打中可以扣血并取消；C才接窗口。动画仍复用一段Quinn侧闪占位，不强行转角色对准每个方向，因此前后 / 斜向的姿势匹配只保证占位可辨认，不宣称已完成八方向动画。不同方向的正式美术改善后续安排，不为本批重烘焙。

责任：Character只读当前Enhanced Input并转换世界方向；Dodge Ability仍拥有动作、Montage和成本；新AbilityTask独占本次RMS和移动结束。位移自然结束只停Task，Montage结束才解除Dodging。任务只在活动时Tick，不新增角色 / UI常驻Tick，不新增Timer或全局移动状态。

采用专用Task，是为了沿用M0已验证的末帧比例、所有权核对、离地中止与日志规则。引擎标准ConstantForce Task本机实现按世界EndTime结束、没有本项目的离地中止 / 所有权日志；本批不直接照搬其结束方式。Runtime仍通过CharacterMovement的RMS走真实运动，不每帧SetActorLocation。

启动顺序改为：合法性 → 读取固定方向 → 启动并核对Montage → 准备RMS → CommitAbility一次 → 后续移动Tick。准备失败不扣；若提交期间取消，EndAbility清掉刚准备的RMS。同步调用内还没有运动Tick；本批没有资源回滚或第二套成本。

本机核对了GameplayTasks的EndTask / OnDestroy、RootMotionSource GetByID包含Pending源、EnhancedPlayerInput先计算Action状态再派发委托。方向读取当前MoveAction值，**不依赖Move()是否已执行，也不使用LastMovementInputVector**。官方参考：[Task清理约定](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/Abilities/Tasks/UAbilityTask/OnDestroy?application_version=5.5)、[当前输入查询](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/EnhancedInput/UEnhancedPlayerInput/GetActionValue?lang=en-US)；网页可能显示较新版本，实际签名以本机5.5.4头文件为准。任务结束先复制委托、EndTask，最后广播副本，避免结束后访问已标记清理的Task。

## 2. 文件与操作顺序

Stop PIE → Save All → 关闭UE → CLion保存。共 **三新 / 五旧Source**，不改Build.cs / Target / uproject / UI / 旧Practice与资源实现。GameplayAbilities已公开依赖GameplayTasks，本模块现有依赖足够。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Abilities\Tasks\ETAbilityTask_DodgeMove.h        新
├─ EmberTrial\Private\Abilities\Tasks\ETAbilityTask_DodgeMove.cpp    新
├─ EmberTrial\Tests\ETDodgeMovementTests.cpp                         新
├─ EmberTrial\Public\Abilities\ETGA_Dodge.h                          完整替换
├─ EmberTrial\Private\Abilities\ETGA_Dodge.cpp                       完整替换
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h                局部添加
├─ EmberTrial\Private\Characters\EmberTrialCharacter.cpp             局部添加
└─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp  一处添加
```

Tasks必须在Abilities下；Tests不是Texts。新Task是继承UAbilityTask的原生UCLASS；可直接在CLion指定路径新建h / cpp，不通过UE向导生成ActorComponent。头文件最后一个include必须是本文件的.generated.h，不手建generated文件。测试cpp不是UCLASS。

按第3–6节完成八份文件再Build；第4节仅覆盖Dodge两文件，**不能覆盖整份Character或Validator**。本批不迁移目录，不删除历史M0测试。源码编译通过后先配置 / ke小检查，再按第8节迁移2键；不要提前断开旧2。


## 3. 移动Task：当前源码与职责


### 3.1 Public/Abilities/Tasks/ETAbilityTask_DodgeMove.h

当前完整源码：[ETAbilityTask_DodgeMove.h](../EmberTrial/Source/EmberTrial/Public/Abilities/Tasks/ETAbilityTask_DodgeMove.h)。

Task只保存自己的RMS和激活快照；不会提前结束整个Dodging动作。


### 3.2 Private/Abilities/Tasks/ETAbilityTask_DodgeMove.cpp

当前完整源码：[ETAbilityTask_DodgeMove.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/Tasks/ETAbilityTask_DodgeMove.cpp)。

CharacterMovement负责积分与碰撞；结束按Source身份释放，并只广播一次完成 / 中断。


## 4. Dodge Ability：当前源码与职责

A的成本Schema、CheckCost / 幂等ApplyCost、Tag和Montage回调全部保留。新增Movement配置与Task，不把位移完成回调接到正常EndAbility。


### 4.1 Public/Abilities/ETGA_Dodge.h

当前完整源码：[ETGA_Dodge.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Dodge.h)。

配置、任务句柄和激活序号均在类内；完整声明含后续连段取消与治疗互斥。


### 4.2 Private/Abilities/ETGA_Dodge.cpp

当前完整源码：[ETGA_Dodge.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Dodge.cpp)。

当前实现包含预检、动画 / 位移准备、单次扣费、短无敌及按本次所有权收尾；时序见路线页。


## 5. Character与校验器局部添加

本节保留其余原代码，只在指定锚点添加；不删除IsCombatDead等已有函数实现。


### 5.1 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

在头文件已有friend class FETStaminaFoundationTest后加一行friend；两行都保留。仅供测试实例设置阵营，不修改资产。

找到这段（应只有一处）：

```cpp
friend class FETStaminaFoundationTest;
```

替换为：

```cpp
friend class FETStaminaFoundationTest;
    friend class FETDodgeMovementTest;
```


### 5.2 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

在public区域既有RequestDodge声明后加两个普通C++函数声明；不要把UFUNCTION标记复制到这两个函数上。

找到这段（应只有一处）：

```cpp
bool RequestDodge();
```

替换为：

```cpp
bool RequestDodge();

    FVector GetDodgeWorldDirection() const;
    static FVector MakeDodgeWorldDirection(const FVector2D& Axis, float ControlYaw, const FVector& Forward);
```


### 5.3 `EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

在cpp原EnhancedInputSubsystems.h include后新增两条include，其余不动。

找到这段（应只有一处）：

```cpp
#include "EnhancedInputSubsystems.h"
```

替换为：

```cpp
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
```


### 5.4 `EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

在cpp完整RequestDodge函数的结束大括号之后、GrantLightAttackAbility函数之前插入下方两个完整函数。下方最后一行GrantLightAttackAbility只是定位锚点；对照旧行整体替换可避免重复写函数头。

找到这段（应只有一处）：

```cpp
void AEmberTrialCharacter::GrantLightAttackAbility()
```

替换为：

```cpp
FVector AEmberTrialCharacter::MakeDodgeWorldDirection(
    const FVector2D& Axis, float ControlYaw, const FVector& Forward)
{
    if (!FMath::IsFinite(Axis.X) || !FMath::IsFinite(Axis.Y) ||
        !FMath::IsFinite(ControlYaw) || Forward.ContainsNaN())
    {
        return FVector::ZeroVector;
    }
    if (Axis.SizeSquared() <= FMath::Square(0.1f))
    {
        return Forward.GetSafeNormal2D();
    }
    const FRotationMatrix Yaw(FRotator(0.0f, ControlYaw, 0.0f));
    return (Yaw.GetUnitAxis(EAxis::X) * Axis.Y + Yaw.GetUnitAxis(EAxis::Y) * Axis.X).GetSafeNormal2D();
}

FVector AEmberTrialCharacter::GetDodgeWorldDirection() const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const UEnhancedPlayerInput* Input = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    FVector2D Axis = FVector2D::ZeroVector;
    if (IsValid(Input) && IsValid(MoveAction) && MoveAction->ValueType == EInputActionValueType::Axis2D)
    {
        Axis = Input->GetActionValue(MoveAction).Get<FVector2D>();
    }
    const float Yaw = PC ? PC->GetControlRotation().Yaw : GetActorRotation().Yaw;
    return MakeDodgeWorldDirection(Axis, Yaw, GetActorForwardVector());
}

void AEmberTrialCharacter::GrantLightAttackAbility()
```


### 5.5 `EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`

在Validator的ValidateAbility内，找到Defaults->IsA<UETGA_Dodge>()分支；在既有Cost GE检查结束后、该分支return前加入移动配置检查。不要放到后面的LightAttack分支。

找到这段（应只有一处）：

```cpp
AddIssue(Blueprint, TEXT("ET-DODGE-001"), TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."), Issues);
        }
        return;
```

替换为：

```cpp
AddIssue(Blueprint, TEXT("ET-DODGE-001"), TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."), Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidMovementConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-002"),
                TEXT("Dodge requires finite Distance 1..300, Duration .05...50, and an in-place montage longer than Duration."), Issues);
        }
        return;
```


## 6. 新建长期移动自动化测试

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETDodgeMovementTests.cpp`。完整保存下方内容；保留原CostContract与两资源测试。

新测试在临时世界真正TryActivate原生能力，播放已有Montage、应用真实成本GE、运行CharacterMovement；三步长、暂停 / 取消 / 死亡 / EndPlay的时点由测试驱动。方向数学检查与真实运动分别断言，不把键盘输入或蓝图连线冒充自动覆盖。不会修改GA资产或CDO，不生成测试场景。

当前完整源码：[ETDodgeMovementTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETDodgeMovementTests.cpp)。

长期测试保留方向数学、真实RMS运动、不同步长、暂停与取消等断言。


## 7. 正常Build与索引

确认三新 / 五旧共八文件全部保存，UE仍关闭。PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

它先正常Editor Build，再刷新CLion CMake；成功后在CLion Reload CMake，三个CodeModel应共 **48cpp**（Core32 / UI13 / Editor3）。新Task与测试属于EmberTrialCodeModel；不以CMake成功替代UBT构建。随后执行独立Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标成功后打开UE工程。不要用Live Coding替代这次新UCLASS / 属性构建，不清缓存 / 全删Intermediate。若失败，发第一条UHT / C++ / linker错误及前后10–20行；末尾UBT exit6不是根因。

## 8. 配置与2键迁移（无新资产）

### 8.1 配置GA并先用ke确认新位移

1. 打开`/Game/EmberTrial/Abilities/GA_ET_Dodge`，Class Defaults，Details搜索`Dodge`。不是角色组件PracticeAction的同名字段。
2. 确认Dodge Montage仍为`AM_M0_Dodge_01_Montage`，新增 **Dodge Distance=150.0、Dodge Duration=0.30**；Costs的Cost Gameplay Effect Class仍GE_ET_DodgeCost。EventGraph保持空。
3. 本Montage应是原地动画，无启用动画Root Motion的Sequence；不修改既有动画。新增原生检查会拒绝具有动画Root Motion的Montage，避免与Task双重位移。
4. Compile → Save → Content Browser右键GA → Asset Actions → Validate Assets，应有本项目Valid / Issues0。新规则ET-DODGE-002检查范围、时长及原地Montage；不要求再建Probe。
5. Play，在Entry空地松开WASD，Output Log执行：

   ```text
   ke BP_ThirdPersonCharacter_C RequestDodge
   ```

   应从原地动画变成**角色前方150cm**，日志Move begin → A Begin扣25 → Move end → A End。Move end的SourceTime约0.3000，平地Travel2D约150；Elapsed受Tick顺序影响，不要求恰好0.300。Montage结束晚于Move end是正确结果。若本步仍原地，先停在这里发日志，不迁移2键。

### 8.2 只改IA_PracticeDodge的Started执行线

1. Stop PIE，打开`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` → EventGraph。
2. 在图里Ctrl+F，搜索 **IA_PracticeDodge**，双击结果定位事件。找的是Enhanced Input Action事件，拥有Started / Triggered / Completed输出；不是新建Keyboard 2事件。
3. 当前旧链应为：`Started → Branch(Is Formal Action Idle, Self) → True → Request Practice Action(Target=PracticeAction, Dodging)`。先认清这条白线，其他1 / 3 / 4 / E / P不动。
4. 按住Alt单击IA_PracticeDodge事件的**Started白色输出针脚**，断开它到旧Branch的执行线。只断这条事件的Started输出，不去断E的公共门控，也不删PracticeAction组件。
5. 在事件右边空白处右键，保持Context Sensitive，搜索 **Request Dodge**。选择继承自EmberTrialCharacter的原生函数；新节点 **Target=Self**。它不是Request Practice Action，Target不能接PracticeAction组件。
6. 从 **IA_PracticeDodge.Started** 白色输出，拖到新 **Request Dodge** 左侧白色执行输入。Return Value布尔输出可以留空；日志已记录Accepted，不增加Print / Delay / Gate / 重试。
7. 最终连线仅为：

   ```text
   IA_PracticeDodge
       Started ──→ Request Dodge
                       Target = Self
                       Return Value = 不连接
       Triggered / Ongoing / Completed / Canceled = 不接任何闪避调用
   ```

8. 原旧Branch / Dodging请求如仅服务这条2路线，可以删除已孤立节点及该链独用的PracticeAction Get；如你不确定某个查询是否被E共用，保留它但不接任何执行线。最终必须没有从IA_PracticeDodge任何执行输出到旧Request Practice Action(Dodging)的可达路线。
9. Compile → Save。My Blueprint中保留原交互 / 提示函数和组件，不改其他事件。

本批复用名字带Practice的IA资产，避免重命名造成引用 / Redirector额外工作；当前职责已是正式GAS输入。**名字保留不代表仍调用M0。**不增加第二份Action或另一条键盘2事件，不把Started换成Triggered。

### 8.3 检查输入配置与保存

1. 打开`/Game/EmberTrial/Input/Actions/IA_PracticeDodge`，Value Type保持Boolean，Trigger When Paused / 在暂停时触发=False；原Triggers / Modifiers不另加Hold或Pulse。若原来就是该值，无需制造保存差异。
2. 玩家当前Default Mapping Context仍是既有IMC_Default；其中IA_PracticeDodge保持键盘**上方2**一行，不复制第二行，不改WASD / 鼠标 / E映射。暂停P已迁出旧IMC，不能补回。
3. BP_ThirdPersonCharacter Class Defaults确认Dodge Ability Class仍GA_ET_Dodge；PracticeAction原三Montage / Auto Activate / 150 / .30保持，供原型历史用途，不是正式GA的移动配置。
4. GA与玩家BP各Compile → Save → 项目Validate Assets，两份Valid / Issues0即可；Cost GE未改，不重做六资产全选。Save All后再开始检查。

## 9. 分层检查（约10–15分钟）

### 9.1 一组命令、两项Success（约2–3分钟）

不Play，Output Log执行：

```text
Automation RunTests EmberTrial.M2.Dodge
```

应有 **CostContract / Movement各一次Success**。Movement覆盖8方向数学与镜头Yaw转换、无输入前方；完整GAS启动 / 一次成本、忙碌拒绝、真实运动30 / 60 / 120固定步长各150±1cm / SourceTime.30、位移结束仍Dodging / 恢复受阻、取消不退款、墙碰撞、暂停冻结 / 恢复、真实死亡取消、活动中EndPlay清理及非法配置 / 外部RMS拒绝。

固定步长是测试世界模拟步长，不冒充硬件实际30 / 60 / 120FPS测量。蓝图2键连接、真实输入 / 相机对应、坡面视觉及UI由下组实机检查；旧Practice位移测试不代替新GA测试。资源实现与旧Practice未改，不重复跑Resources / Lifecycle或旧墙坡帧率全矩阵。失败发测试名与日志路径，我读取细节，不删断言。

### 9.2 正式2键与方向（约3–4分钟）

1. 正式地图Play，在Entry空地，生命 / 体力100。松开WASD按一次2：角色前方位移、一次扣25、日志为`[M2-02B] Move begin/end`与A Begin/End；**不能出现这次输入的`[M0-05C2] DodgeMove Begin`**。
2. 依次W+2、A+2、S+2、D+2、W+A+2；每次等动作结束并在空地操作。按移动键先维持短暂一瞬再按2，观察朝向与镜头水平坐标关系。转镜头约90度，再做W+2，应跟新镜头前方。
3. 先按A移动、松开A并站定，再按2：按当前角色前方，不沿缓存的A向量。距离看日志，不要求拿尺测屏幕。
4. 长按2只开始一次；动作期间再按2被拒绝，不补播、不多扣。动作中尝试改变WASD / 转镜头，本段方向保持固定；位移停下到动画结束前仍不能移动 / 跳跃 / 攻击 / E，镜头可转。

本批持续按W等动作结束后继续走动属于正常输入恢复；观察“无漂移”时请松开WASD。若在墙边或敌人碰撞附近，Travel不能作为平地满距样本。

### 9.3 碰撞、取消与暂停（约3–5分钟）

1. 用现有场地高墙，朝墙按2，胶囊被阻挡，实际Travel可小于150；再斜向贴墙一次，应沿墙滑动、无穿墙。不要新搭测试蓝图或改正式碰撞通道。
2. 有现成可走坡面就上 / 下各一次；没有坡面时，临时拖一个Engine Cube到远离敌人的空地，Scale约`(6,3,0.3)`、Rotation Pitch约`12°`，用移动工具把低端嵌入地面，使中段可正常走上去再按2上下各一次。若普通步行都上不去，先调整斜块位置；这不是闪避失败。试完删除临时Cube，不改导航 / 不保存临时场景。
3. 可控制台`slomo 0.1`放慢，2后按3一次，观察立即停位移 / 进入受击、体力不退款；下一次2中按P，暂停时位置 / 动画冻结，继续后完成剩余位移。只试一种继续方式，不重演所有菜单焦点。具体.1秒时点由自动化负责，手动无需赶帧。
4. 恢复`slomo 1`。本批没有无敌，实战被击中扣血 / 取消正常；不要把4开发伤害键或躲开碰撞当无敌窗口证据。死亡与EndPlay精确时序已有自动化，无需再手搭临时致死入口。

### 9.4 最后短基线与恢复（约1–2分钟）

正常Play：左键真实25、E真实交互、P一种继续、移动 / 转向 / 跳跃与体力回满；确认2为新正式闪避，生命HUD保持正常。Stop后再Play，新身体双100 / 能再次2 / 无旧回调。临时Cube删除、slomo1、正式100/100 / 伤害25与10 / Debug False，GA150 / .30 / 成本25 / 原Montage，Compile → Save All。

不做新Cook / Insights；M1包不作为本批结果。地面 / 墙坡运动与生命周期成立后，C再单独接短无敌与相关集成，不在本批扩大连击 / 锁定 / 治疗。

## 10. 最简反馈与报错

```text
M2-02B 完成 / 卡在第__节第__步
三新 / 五旧Source；关闭UE后Editor Build / 索引48cpp / Game Build：
GA150 / .30 / Cost25 / 原Montage；GA与玩家Compile / Save / Valid：
IA_PracticeDodge.Started只到RequestDodge(Self)，旧2执行线断开：
CostContract / Movement各一次Success：
上方2正式移动并扣25 / 长按单次 / 忙碌无补播：
四向与斜向 / 镜头90度 / 无输入角色前方 / 方向固定：
位移先结束、Montage后解锁 / 无漂移 / 阻断恢复：
墙与斜墙 / 可走坡面上下 / 3取消不退款 / P冻结恢复：
Stop重开双100 / 左键-E-P与移动跳跃短基线：
临时Cube删除、slomo1 / 原配置与Source资产Save All：
失败或占位动画最明显问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

Build用文字贴第一条真实错误；蓝图连线问题截IA事件到调用的完整局部图，包含白色执行线与Target；配置问题截GA Class Defaults（不是PracticeAction），同时给资产路径。运行问题写“输入 / 当时状态 / 实际现象”，日志只给大致时分或本机路径，我自行读取；不要求手摘完整Begin / End。

## 11. 本批 Commit / Push（固定放在最后）

本页历史功能已完成；第24批对应源码 / 文档的改动与验收见[Cleanup对应章节](Cleanup.md#cleanup-batch-24)。用户重新Stage最新差异、检查Staged Diff后Commit / Push，Raw / 日志 / 生成文件留本地。第24–26批一起Stage / Commit / Push，完整18项清单与建议Commit信息见[Cleanup末尾](Cleanup.md#本批-commit--push固定放在最后)；助手不操作Git索引。
