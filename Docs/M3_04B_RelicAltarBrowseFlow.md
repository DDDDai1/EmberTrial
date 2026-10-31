# M3-04B：E进入祭坛浏览页与输入恢复

2026-10-30编制，2026-10-31更新，UE5.5.4。[M3-04A已验收](Reports/M3_04A_Verification.md)。本章正在实施；第8.1节已按当次授权由助手通过computer-use完成、编译并正常保存。其余步骤与第9节运行检查仍按实际完成情况反馈，不据本次蓝图编译推定整章通过。

## 1. 看到的结果与本批边界

靠近祭坛后按E，打开复用原Pause布局的遗物浏览页，状态文字标明“遗物祭坛”。世界暂停，右侧继续使用已完成的列表 / 搜索 / 筛选 / 比较；P或原继续按钮返回游戏。普通位置按P仍开普通暂停页；离开祭坛后E仍走原A/B交互。

这是祭坛来源的正式浏览入口，**本批仍没有确认 / 卸装按钮**。C再接最终权限、版本重验与真实换装；P页继续只读，不因为复用了页面就获得装备权限。原TryEquip暂停拒绝保持。

复用Controller当前单个FailureFlow与WBP_PauseMenu，不新建第二份Flow、WidgetClass、Input Action或IMC映射。SourceAltar只用弱身份记录在该页面的Flow中；移除页面即解绑 / 清空，GI不持有旧世界祭坛。

| 工作 | 范围 | 预计 |
| --- | --- | --- |
| C++ | 七旧Source：Controller两个、Flow两个、Pause Widget两个、原Altar测试 | 70–100分钟 |
| 蓝图 | 原角色唯一IA_Interact入口小改；祭坛Label增加E标识；原Pause编译核对 | 15–25分钟 |
| 检查 | 扩原Altar专项一次；实机开关 / 焦点与一次来源销毁 | 5–10分钟 |

当前191 Source / 107cpp（Core75 / UI29 / Editor3）、32测试、299 Content / 来源80；本批不增加文件、测试注册或资产，预计数量保持。双Build与索引等待另记。

## 2. E意图与暂停归属

```text
原IA_Interact.Started
  → 当前Controller.TryRequestRelicAltarInteraction
      True：有祭坛，消费本次E，UI Flow决定是否能展示
      False：没有可用祭坛，转回原TryInteractWithCurrentTarget

Core：资格 / 最近祭坛 → 广播一次OnRelicAltarRequested
UI：重验当前身体 / 无其他页 / 未暂停 / 来源 → 复用PresentPause
关闭 / 换身体 / 死亡 / EndPlay：沿原Flow清理页面和输入；同时清来源订阅
```

True不等于创建页面成功；如果WidgetClass配置失败，本次E仍不能落到A/B激活另一件物体。普通P允许动作中暂停，祭坛E要求A已有Idle资格。仅祭坛对象结束时关闭对应祭坛页；普通P页不受其他祭坛销毁影响。切图中不恢复旧世界，失败恢复时若来源已结束便撤销祭坛页。

## 3. 两份旧Controller源码

先Save All并关闭UE。只改下面指定片段，原锁定 / 资源Exec / P绑定与生命周期保留。

### 3.1 Public/Framework/ETPlayerController.h

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETPlayerController.h`。

在`class APawn;`后新增前置声明：

```cpp
class AETRelicAltar;
```

在原`DECLARE_MULTICAST_DELEGATE(FETPauseRequested);`后新增：

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicAltarRequested, AETRelicAltar*);
```

在public原`FETPauseRequested OnPauseRequested;`后新增以下完整片段：

```cpp
    // True只表示找到祭坛并消费本次交互；页面是否成功呈现由UI层决定。
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Relic Altar")
    bool TryRequestRelicAltarInteraction();

    FETRelicAltarRequested OnRelicAltarRequested;
```

### 3.2 Private/Framework/ETPlayerController.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETPlayerController.cpp`。

在include区新增（不是generated.h）：

```cpp
#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
```

文件末尾新增这个函数，原函数全部保留：

```cpp
bool AETPlayerController::TryRequestRelicAltarInteraction()
{
    UWorld* World = GetWorld();
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(GetPawn());
    if (!IsLocalController() || !World || World->bIsTearingDown || !IsValid(Body) ||
        Body->GetController() != this)
    {
        return false;
    }
    UETRelicAltarSubsystem* Registry = World->GetSubsystem<UETRelicAltarSubsystem>();
    AETRelicAltar* Altar = Registry ? Registry->FindAvailableAltar(Body) : nullptr;
    if (!IsValid(Altar))
    {
        return false;
    }
    UE_LOG(LogETPlayerController, Display,
        TEXT("[M3-04B] Altar intent Controller=%s Altar=%s"), *GetName(), *Altar->GetName());
    // 只广播一次意图，不在Core创建Widget / 暂停，也不直接提交装备。
    OnRelicAltarRequested.Broadcast(Altar);
    return true;
}
```

## 4. 两份旧Pause Widget源码

路径目录为`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI`。

### 4.1 Public/Widgets/ETPauseWidget.h

在public的`void ShowReady();`后新增：

```cpp
    void ShowAltarReady();
```

### 4.2 Private/Widgets/ETPauseWidget.cpp

末尾新增函数。原三绑定、焦点 / P处理保持；这只改变状态文字，不接管暂停。

```cpp
void UETPauseWidget::ShowAltarReady()
{
    ApplyState(false, FText::FromString(
        TEXT("遗物祭坛 · 选择遗物比较属性；松开 P 或选择继续游戏返回试炼。")));
}
```

## 5. 修改原FailureFlow

目录仍为`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI`。不改组件名、原三项资源配置 / 数字层级，不给Controller添加第二个组件。

### 5.1 Public/Flow/ETFailureFlowComponent.h

1. 前置声明区新增`class AETRelicAltar;`，generated.h仍是最后include。
2. 原private的`void PresentPause();`整行替换为下段：

```cpp
    void PresentPause(AETRelicAltar* SourceAltar = nullptr);
    void HandleAltarRequested(AETRelicAltar* SourceAltar);
    void ClearAltarContext();

    UFUNCTION()
    void HandleAltarEndPlay(AActor* SourceActor, EEndPlayReason::Type Reason);
```

3. 在原`FDelegateHandle PauseRequestHandle;`前新增以下字段，原字段保留：

```cpp
    // 非空（包括已失效弱身份）表示祭坛来源；移除页面时撤销。
    TWeakObjectPtr<AETRelicAltar> ActiveAltar;
    FDelegateHandle AltarRequestHandle;
```

### 5.2 Private/Flow/ETFailureFlowComponent.cpp：include / 订阅 / 结束

include区新增：

```cpp
#include "Interaction/ETRelicAltar.h"
```

在BeginPlay中找到原P请求订阅这两行：

```cpp
    PauseRequestHandle =
        Controller->OnPauseRequested.AddUObject(this, &ThisClass::HandlePauseRequested);
```

保留原文，紧接着新增：

```cpp
    AltarRequestHandle = Controller->OnRelicAltarRequested.AddUObject(
        this, &ThisClass::HandleAltarRequested);
```

在EndPlay原`if (AETPlayerController* Controller = ControllerOwner.Get())`内部，原Remove后新增；不要加到另一个相似的Controller局部变量代码块：

```cpp
        Controller->OnRelicAltarRequested.Remove(AltarRequestHandle);
```

在同一个EndPlay函数原`PauseRequestHandle.Reset();`后新增：

```cpp
    AltarRequestHandle.Reset();
```

### 5.3 完整替换PresentPause

从`void UETFailureFlowComponent::PresentPause`至配对最后一个`}`整体替换，相邻函数不动。原HandlePauseRequested仍调用不带参数的PresentPause()，由头文件默认参数表示普通P来源。

```cpp
void UETFailureFlowComponent::PresentPause(AETRelicAltar* SourceAltar)
{
    const bool bFromAltar = SourceAltar != nullptr;
    AETPlayerController* Controller = ControllerOwner.Get();
    AEmberTrialCharacter* OpeningBody = Controller
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    if (!HasCurrentBody(false) || bCommandPending || bPresentationQueued ||
        IsValid(PauseWidget.Get()) || IsValid(FailureWidget.Get()) || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this) ||
        (bFromAltar && (!IsValid(SourceAltar) || !SourceAltar->CanUseAltar(OpeningBody))))
    {
        return;
    }
    if (!PauseWidgetClass || PauseWidgetClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06D] Missing concrete PauseWidgetClass Owner=%s"), *GetNameSafe(GetOwner()));
        return;
    }
    PauseWidget = CreateWidget<UETPauseWidget>(Controller, PauseWidgetClass);
    if (!IsValid(PauseWidget.Get()) || !PauseWidget->HasRequiredLayout())
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Pause layout invalid"));
        RemovePauseWidget();
        return;
    }
    PauseWidget->OnResumeIntent.AddUniqueDynamic(this, &ThisClass::HandleResumeRequested);
    PauseWidget->OnMainMenuIntent.AddUniqueDynamic(this, &ThisClass::HandleMainMenuRequested);
    if (bFromAltar)
    {
        PauseWidget->ShowAltarReady();
    }
    else
    {
        PauseWidget->ShowReady();
    }
    if (!PauseWidget->AddToPlayerScreen(PauseWidgetZOrder))
    {
        RemovePauseWidget();
        return;
    }
    // 创建 / 显示Widget可同步运行回调；在取得暂停前重验当前身体与来源。
    if (!IsValid(PauseWidget.Get()) || !HasCurrentBody(false) ||
        Controller->GetPawn() != OpeningBody || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this) ||
        (bFromAltar && (!IsValid(SourceAltar) || !SourceAltar->CanUseAltar(OpeningBody))) ||
        !UGameplayStatics::SetGamePaused(this, true))
    {
        RemovePauseWidget();
        return;
    }
    bOwnsPause = true;
    if (bFromAltar)
    {
        ActiveAltar = SourceAltar;
        SourceAltar->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleAltarEndPlay);
    }
    OpeningBody->StopJumping();
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, PauseWidget.Get(),
        EMouseLockMode::DoNotLock, true);
    PauseWidget->FocusResume();
    NotifyMenuState();
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Pause shown Owner=%s Widget=%s Paused=1"),
        *GetNameSafe(Controller), *GetNameSafe(PauseWidget.Get()));
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M3-04B] Browse shown Owner=%s Altar=%s Mode=%s"), *GetNameSafe(Controller),
        *GetNameSafe(ActiveAltar.Get()), bFromAltar ? TEXT("Altar") : TEXT("Pause"));
}
```

### 5.4 新增三个来源函数

在cpp末尾新增，别放到EndPlay的花括号内：

```cpp
void UETFailureFlowComponent::HandleAltarRequested(AETRelicAltar* SourceAltar)
{
    if (!HasCurrentBody(false) || !IsValid(SourceAltar) || SourceAltar->GetWorld() != GetWorld())
    {
        return;
    }
    PresentPause(SourceAltar);
}

void UETFailureFlowComponent::ClearAltarContext()
{
    // Get(true)仅用于解绑结束中的对象，不能拿这种对象授权装备或读取玩法状态。
    if (AETRelicAltar* Altar = ActiveAltar.Get(true))
    {
        Altar->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleAltarEndPlay);
    }
    ActiveAltar.Reset();
}

void UETFailureFlowComponent::HandleAltarEndPlay(AActor* SourceActor, EEndPlayReason::Type Reason)
{
    if (bEndingPlay || SourceActor != ActiveAltar.Get(true) || !HasCurrentContext())
    {
        return;
    }
    UE_LOG(LogETFailureFlow, Display, TEXT("[M3-04B] Altar source ended Owner=%s Reason=%d"),
        *GetNameSafe(GetOwner()), static_cast<int32>(Reason));
    // 切图中不恢复旧世界；若切图失败，RecoverActivePanel会处理失效的祭坛来源。
    if (!bCommandPending && !Session->IsTravelPending())
    {
        HandleResumeRequested();
    }
}
```

Get(true)仅用于从正在结束的源对象移除动态委托，依据本机5.5.4 WeakObjectPtrTemplates.h的接口；最终授权绝不能用该方式取得无效对象。

### 5.5 完整替换RemovePauseWidget与RecoverActivePanel

这两处确保普通关闭 / 死亡替换 / 换身体 / 切图失败都撤销来源。其余恢复输入、Failure呈现与会话函数保持。

```cpp
void UETFailureFlowComponent::RemovePauseWidget()
{
    ClearAltarContext();
    if (IsValid(PauseWidget.Get()))
    {
        const FString WidgetName = GetNameSafe(PauseWidget.Get());
        PauseWidget->OnResumeIntent.RemoveDynamic(this, &ThisClass::HandleResumeRequested);
        PauseWidget->OnMainMenuIntent.RemoveDynamic(this, &ThisClass::HandleMainMenuRequested);
        PauseWidget->RemoveFromParent();
        UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Pause removed Owner=%s Widget=%s"),
               *GetNameSafe(GetOwner()), *WidgetName);
    }
    PauseWidget = nullptr;
}
```

```cpp
void UETFailureFlowComponent::RecoverActivePanel(const FText& Message)
{
    bCommandPending = false;
    if (!HasCurrentContext())
    {
        return;
    }
    if (!ActiveAltar.IsExplicitlyNull() && !ActiveAltar.IsValid())
    {
        // 请求失败但原祭坛已结束，撤销该页面，不能恢复成一个无来源的祭坛上下文。
        HandleResumeRequested();
        return;
    }
    if (!UGameplayStatics::IsGamePaused(this))
    {
        if (!UGameplayStatics::SetGamePaused(this, true))
        {
            UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Recovery pause rejected"));
            return;
        }
        bOwnsPause = true;
    }
    if (HasCurrentDeadBody() && IsValid(PauseWidget.Get()))
    {
        QueueFailure();
        return;
    }
    const FText Error =
        Message.IsEmpty() ? FText::FromString(TEXT("请求未完成，请重新选择。")) : Message;
    AETPlayerController* Controller = ControllerOwner.Get();
    Controller->bShowMouseCursor = true;
    if (IsValid(FailureWidget.Get()) && HasCurrentDeadBody())
    {
        FailureWidget->ShowError(Error);
        UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, FailureWidget.Get(),
                                                       EMouseLockMode::DoNotLock, true);
        FailureWidget->FocusRetryButton();
    }
    else if (IsValid(PauseWidget.Get()))
    {
        PauseWidget->ShowError(Error);
        UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, PauseWidget.Get(),
                                                       EMouseLockMode::DoNotLock, true);
        PauseWidget->FocusResume();
    }
    NotifyMenuState();
    UE_LOG(LogETFailureFlow, Display,
           TEXT("[M1-06D] Panel recovered Owner=%s Message=%s Paused=%d"), *GetNameSafe(Controller),
           *Error.ToString(), UGameplayStatics::IsGamePaused(this) ? 1 : 0);
}
```

## 6. 扩展原Altar测试

`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicAltarTests.cpp`。

1. include区增加`#include "Framework/ETPlayerController.h"`。
2. 将原`APlayerController* PC = World->SpawnActor<APlayerController>();`这一行替换为：

```cpp
    AETPlayerController* PC = World->SpawnActor<AETPlayerController>();
```

3. 找到`TestTrue(TEXT("Direct eligibility agrees"), Near->CanUseAltar(Body));`，保留它，在其后插入：

```cpp
    // ---- 本专项只测Core意图，不用无Viewport夹具伪装已打开UMG页面 ----
    {
        int32 IntentCount = 0;
        TWeakObjectPtr<AETRelicAltar> Received;
        const FDelegateHandle IntentHandle = PC->OnRelicAltarRequested.AddLambda(
            [&](AETRelicAltar* Altar)
            {
                ++IntentCount;
                Received = Altar;
            });
        const bool bConsumed = PC->TryRequestRelicAltarInteraction();
        PC->OnRelicAltarRequested.Remove(IntentHandle);
        TestTrue(TEXT("One request consumed and routed nearest once"),
            bConsumed && IntentCount == 1 && Received.Get() == Near);
    }
```

4. 在原下面四条断言的**各自下一行**补充对应的新断言。旧断言保留，每行只加在该处一次：

| 原断言文字 | 新增下一行 |
| --- | --- |
| Leaving range clears query | `TestFalse(TEXT("Outside does not consume legacy interaction"), PC->TryRequestRelicAltarInteraction());` |
| Busy rejected | `TestFalse(TEXT("Busy publishes no altar intent"), PC->TryRequestRelicAltarInteraction());` |
| Paused opening rejected | `TestFalse(TEXT("Paused request not consumed"), PC->TryRequestRelicAltarInteraction());` |
| No old altar retained | `TestFalse(TEXT("Destroyed sources cannot consume request"), PC->TryRequestRelicAltarInteraction());` |

这是一次Core意图的真实路由测试，没有Viewport，所以不声称它已验证页面显示。实际焦点 / UMG / 暂停恢复留第9节，不额外新增测试文件。

## 7. 正常双Build与索引

七个旧文件保存、UE保持关闭。PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后CLion Reload CMake，确认Core / UI / Editor三目标索引正常。再运行Game Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功后开UE。不要使用旧进程Live Coding新增反射函数；没有新Build.cs / Target / 插件改动。失败发第一条error及前后约10行。

## 8. 蓝图连接（只改原入口）

### 8.1 BP_ThirdPersonCharacter的IA_Interact

1. 打开原角色蓝图Event Graph，用Ctrl+F找**IA_Interact**，定位原Enhanced Input事件；**保留唯一事件与Started，不新建第二个事件**。原映射依然E，不改IMC / Trigger When Paused。
2. 原Started白线通往`TryInteractWithCurrentTarget`。只断开这条入口白线，保留该调用节点，以及函数内部原暂停 / Idle / 候选逻辑。不要删函数或CurrentTarget。
3. 从空白处添加**Get Controller**（Target=Self），从它的Return Value拖线搜索**Cast To ETPlayerController**。选带白色执行引脚的Cast，Object必须是Get Controller.Return Value，不使用Get Player Controller 0。
4. 白线：**IA_Interact.Started → Cast To ETPlayerController**。Cast成功输出白线接**Try Request Relic Altar Interaction**；此函数节点Target接蓝色`As ETPlayerController`输出，不是Self。
5. 从新请求节点执行输出接一个**Branch**；Condition接它的红色**Return Value**。Branch.True不接任何旧交互节点，表示本次E已由祭坛消费；Branch.False接原`TryInteractWithCurrentTarget(Target=Self)`。
6. Cast Failed输出接同一个旧函数的另一份调用节点（Target=Self）；两个分支互斥。若编辑器允许把两路白线汇入同一个原调用输入，也可共用那一份调用。
7. 完整执行骨架：

```text
IA_Interact.Started → Cast ETPlayerController
  Cast Success → TryRequestRelicAltarInteraction → Branch(Return Value)
                    True  → 结束（祭坛消费，不再激活A/B）
                    False → TryInteractWithCurrentTarget(Self)
  Cast Failed → TryInteractWithCurrentTarget(Self)

蓝线：GetController(Self).ReturnValue → Cast.Object
      Cast.AsETPlayerController → 新请求.Target
红线：新请求.ReturnValue → Branch.Condition
```

8. Compile绿色后Save。检查Branch.Condition确实有红线，不能悬空沿用默认True；Started之外的Triggered / Completed / Canceled不接本批逻辑。新请求不接回角色RequestLightAttack / E原目标的TryInteract Message。

**8.1实机操作记录（2026-10-31，已完成）：**

- 接手时入口未接通，Cast.Object为空，Cast成功仍直连旧交互。现已补齐上方骨架中的全部白线、Controller蓝线与Return Value红线。
- True保持空接；False与Cast Failed共用原TryInteractWithCurrentTarget(Self)调用。编辑器允许这两条互斥白线汇入同一输入，无需复制函数调用。
- 已整理节点；其余输入执行引脚保持空接。原函数内部、候选注册 / 清理及输入映射未改。
- Compile显示绿色Good to go；正常Save后为All Saved。14:56:26日志确认保存到正式Content路径，项目资产校验Valid / Issues=0。
- 本记录只确认8.1接线 / 编译 / 保存，第9节运行检查仍待执行。**从8.2继续，无需重接8.1。**

### 8.2 祭坛标识与原页面

1. 打开BP_ETRelicAltar，Components选Label，Details → Text设为**RELIC ALTAR [E]**。只改该组件文字，不添加E / Overlap事件。Compile → Save；单实例RelicAltar_A及180 / 180 / 80、Scale1和碰撞保持。
2. 打开原WBP_PauseMenu：父类仍ETPauseWidget、三必需绑定保持、右侧原RelicPanel仍嵌入；Compile → Save。没有新绑定、不Reparent、不复制新Widget，不改ListEntry Auto / 两面板Never。
3. BP_ETPlayerController只核对：原单FailureFlow、原PauseWidgetClass / FailureWidgetClass / MainMenuMap保持。**不添加新组件或新WidgetClass字段。**本批全部由已有Flow订阅新的Core意图。
4. 返回正式地图Save All。菜单状态事件桥接原E提示仍沿用：NotifyMenuState调用原OnGameMenuStateChanged，关闭后原提示可恢复。

## 9. 分组检查（约5–10分钟）

### 9.1 一个既有专项

非PIE时Output Log执行一次：

```text
Automation RunTests EmberTrial.M3.Interaction.RelicAltar
```

应Success。保留A全部断言，新增最近祭坛一次路由与无候选 / 忙碌 / 暂停 / 已销毁不消费。其他31项不重复，也不新增测试图。

### 9.2 实际页面与恢复

| 组 | 操作顺序 | 预期 |
| --- | --- | --- |
| 入口 | 靠近祭坛180cm内，松开移动后按E；长按E约1秒，松E | 一份页面、状态文字标明遗物祭坛；世界暂停，没有反复重建 / A/B成功日志 |
| 交互 | 搜索一次；输入框内P是字符；Esc离开后P关闭 | 沿用已完成输入规则，关闭后无需点击视口即可移动 / 转向；没有补执行E |
| 第二轮 | 再E打开，鼠标继续或Enter继续；按住W打开、页内松W再继续 | 无卡键 / 无补跳，单份页面与一次来源解绑 |
| 原入口 | 离开祭坛，在原A/B范围按E；范围外按E；普通P开关 | A/B功能保持；普通P状态文字仍是原暂停提示，不冒充祭坛上下文 |
| 忙碌 | 攻击 / 闪避中按E；站定后重新按E | 忙碌不打开 / 不排队；重新按才打开 |

### 9.3 一次来源清理与Stop

1. 仅在PIE中打开祭坛页，按控制台键`~`，输入引擎现有作弊命令`DestroyAll ETRelicAltar`，Enter执行后关闭控制台。命令只针对本轮World的祭坛类；不在编辑器场景中删除持久资产。该命令已核对本机CheatManager声明；若本机控制台提示不识别，回报这一项即可，不另搭Level Blueprint测试图。
2. 来源结束后应关闭祭坛页并恢复游戏，长日志有Altar source ended / Pause removed / Pause resumed；原P仍能使用。Stop后重新Play，地图中保存的祭坛恢复。
3. 再E打开，使用原返回主菜单，再Continue，靠近祭坛E可打开新页；原生命 / 装备会话语义沿用。Stop时无旧页面 / 旧来源错误。
4. 最后左键 / 2 / E-A/B与移动短查，默认数值、slomo、Debug保持，Save All。**B仍只读，没有真实确认按钮是预期**，不重验完整战斗 / Retry矩阵。

普通P页不会因祭坛销毁而关闭；祭坛死亡 / 世界结束的暂停归属沿原Flow，不通过Timer等待一个已暂停世界。范围 / Enabled变化的最终写权限由C在确认时重验，本章不宣称浏览期间持续监视所有条件。

## 10. 简短反馈

```text
M3-04B 完成 / 卡在第__节第__步
七旧Source；关闭UE后Editor Build / 三索引 / Game Build：
原IA_Interact唯一Started / 当前Controller Cast / True消费-False回退；Compile-Save：
祭坛[E]文字、原单Flow与两WidgetClass-Map配置 / Pause Compile-Save：
Interaction.RelicAltar一次：
祭坛E单页 / 长按 / P字符-Esc / 鼠标键盘继续 / 无补播不卡键：
原E-A/B与范围外、普通P / 忙碌不排队：
来源销毁自动关闭 / 菜单Continue新源 / Stop清理：
默认值 / Source资产Save All、失败或最明显UI问题（无填无）：
完成时分 / 手动代码 / 蓝图配置 / 检查 / 构建等待：
```

## 11. 本批 Commit / Push（固定放在最后）

04A实现已在本地522edb9；04B正在实施，8.1角色蓝图已正常保存。下列源码 / 资产须完成本章检查后再作为实现提交；本地候选不Stage。未联网核验远端。

- 当前文档英文：`docs: verify altar discovery and prepare altar browsing flow`
- 中文译文：`docs: 完成祭坛发现验收并准备祭坛浏览流程`

实施并检查后提交以下七旧Source、实际资产差异和本章验收 / 清单 / 进度：

```text
EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h
EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicAltarTests.cpp
EmberTrial/Source/EmberTrialUI/Public/Flow/ETFailureFlowComponent.h
EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp
EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset
EmberTrial/Content/EmberTrial/Blueprints/Interaction/BP_ETRelicAltar.uasset
EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset（仅确有差异）
Docs/M3_04B_RelicAltarBrowseFlow.md
Docs/AssetManifest.csv（按实际资产SHA更新）
当次Reports / Progress / Backlog / 路线等实际文档差异
```

- 实现英文：`feat: route altar interaction through the existing pause flow`
- 中文译文：`feat: 通过既有暂停流程接入祭坛交互与浏览页面`

CLion刷新Changes → Stage最新修改 → Review → Commit / Push由你操作。已Stage后继续修改须再次Stage；Raw / 候选 / 日志 / 缓存 / 包留本地，不Force Add。中英文可择一作标题。
