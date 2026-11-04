# M3-04C1：祭坛换装上下文与真实事务门控

2026-11-04，UE5.5.4。[M3-04B已验收](Reports/M3_04B_Verification.md)，来源销毁窄补验通过。本章已手动完成，[验收报告](Reports/M3_04C1_Verification.md)确认双Build / 两Success及短查通过、无待补。下一章[C2](M3_04C2_RelicConfirmationUI.md)接正式页面确认与卸装；以下保留完整实现供学习。

## 1. 本批交付与时间

这一批建立“先取得祭坛上下文，再在暂停页面内确认”的Core契约。原有非暂停TryEquip与正式暂停确认共同使用一份事务实现；当前身体、原祭坛、当前世界、会话与库存版本都要有效。

**本章不出现新按钮或新实机装备入口，正常。** C2再由现有FailureFlow独占上下文，页面成功呈现后才启用Presenter的提交回调，Presenter不持有Access本体；普通P页没有上下文，关闭 / 死亡 / 换身体 / 切图即撤销。C1不声称已经验证实际页面的权限发放 / 撤销；这些在C2与按钮一起落地。

| 工作 | 本批范围 | 预计手动时间 |
| --- | --- | --- |
| 新Core | ETRelicAltarAccess.h / .cpp，原生C++类 | 15–25分钟 |
| 旧Core | AltarSubsystem两文件、Equipment两文件、原Altar测试 | 35–55分钟 |
| 检查 | 原Altar扩展 + 原EquipmentTransactions各一次，实机短查 | 5–10分钟 |

完成后实际193 Source / 108cpp（Core76 / UI29 / Editor3）、32测试 / 299 Content / 来源80。用户代码50 / 阅读5 / 检查2，共57，等待2；原预算保留用于估计对比。

## 2. 先理解三条边界

```text
开页前：TryOpen(当前Controller, 原祭坛) → 独占的FETRelicAltarAccess
页面暂停后：Equipment.TryEquipAtAltar(Access, 槽, ID, 看过的版本)
  → 重新验证身体 / 世界 / 距离-高差-遮挡-Enabled / 会话
  → 原TryMakePlan重验版本、拥有与槽位规则
  → 原Inventory.TryCommitPlan + 同一自有GE更新
关页：Access.Close()，旧上下文无法再用；新页重新TryOpen
```

- `IsEligibleBody`只是共享身体资格，单独调用它没有写权限；`IsEligiblePlayer`仍拒绝暂停中开页。
- Access不是UObject / USTRUCT，不暴露蓝图，不强持有世界对象；TryOpen只在非暂停且满足原资格时成功，副作用仅创建一个小型原生对象。Close后不可重新启用同一个对象，禁止复制 / 移动Access本体。
- 暂停所有权属于UI Flow，Core无法独立证明一个Widget仍在屏幕上。C2必须把Access生命周期绑定到那一份页面，并在提交时核对Flow自己的暂停与页面身份；**这批的类型化上下文不能代替下一批的Flow检查。** 不新增全局AllowPaused开关或假装靠一个bool即可证明安全。

活体换装仍不免费治疗；上限下降只夹限，重复装备NoChange，旧版本StalePlan，重入Busy。读取 / 比较不改库存或ASC。拆入口只减少重复提交代码，不改变GAS事件顺序。

## 3. 准备与创建两个文件

1. UE中Save All并关闭UE。CLion保留打开，先不Build。
2. 本批是**普通C++类**，不要用UE的UObject / Actor向导。CLion在已有`Public/Interaction`与`Private/Interaction`中创建两个精确文件名：
   - `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Interaction\ETRelicAltarAccess.h`
   - `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Interaction\ETRelicAltarAccess.cpp`
3. 文件名不带F，类名带F；不创建新模块 / Build.cs，不添加generated.h、UCLASS或GENERATED_BODY。头文件与实现不能放在同一个Public目录。

### 3.1 完整头文件

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Templates/UniquePtr.h"

class APlayerController;
class AEmberTrialCharacter;
class AETRelicAltar;
class UETRelicEquipmentComponent;
class UETRunSessionSubsystem;
class UWorld;

// 单次页面上下文：Core只验证身份 / 空间，暂停所有权由页面Flow负责。
// C2由Flow在暂停前创建，失败 / 关闭即撤销；普通P页不创建。
class EMBERTRIAL_API FETRelicAltarAccess
{
public:
    static TUniquePtr<FETRelicAltarAccess> TryOpen(
        APlayerController* Controller, AETRelicAltar* Altar);

    bool CanCommitFor(const UETRelicEquipmentComponent* Equipment) const;
    void Close();
    ~FETRelicAltarAccess() = default;

    FETRelicAltarAccess(const FETRelicAltarAccess&) = delete;
    FETRelicAltarAccess& operator=(const FETRelicAltarAccess&) = delete;
    FETRelicAltarAccess(FETRelicAltarAccess&&) = delete;
    FETRelicAltarAccess& operator=(FETRelicAltarAccess&&) = delete;

private:
    FETRelicAltarAccess() = default;

    TWeakObjectPtr<UWorld> BoundWorld;
    TWeakObjectPtr<APlayerController> BoundController;
    TWeakObjectPtr<AEmberTrialCharacter> BoundBody;
    TWeakObjectPtr<AETRelicAltar> BoundAltar;
    TWeakObjectPtr<UETRunSessionSubsystem> BoundSession;
    bool bOpen = false;
};
```

### 3.2 完整实现文件

```cpp
#include "Interaction/ETRelicAltarAccess.h"
#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Kismet/GameplayStatics.h"

TUniquePtr<FETRelicAltarAccess> FETRelicAltarAccess::TryOpen(
    APlayerController* Controller, AETRelicAltar* Altar)
{
    // 必须先以原来的非暂停资格打开；不能在普通P页临时制造许可。
    AEmberTrialCharacter* Body = IsValid(Controller)
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    UWorld* World = IsValid(Controller) ? Controller->GetWorld() : nullptr;
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UETRunSessionSubsystem* Session = IsValid(GI)
        ? GI->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!IsValid(World) || World->bIsTearingDown || !IsValid(Controller) ||
        Controller->IsActorBeingDestroyed() || !IsValid(Altar) ||
        Altar->GetWorld() != World || !Altar->CanUseAltar(Body) || Body->GetController() != Controller ||
        !IsValid(Session) || Session->IsTravelPending())
    {
        return nullptr;
    }

    // 私有构造只能在此调用；MakeUnique模板本身没有私有构造访问权。
    TUniquePtr<FETRelicAltarAccess> Access(new FETRelicAltarAccess());
    Access->BoundWorld = World;
    Access->BoundController = Controller;
    Access->BoundBody = Body;
    Access->BoundAltar = Altar;
    Access->BoundSession = Session;
    Access->bOpen = true;
    return Access;
}

bool FETRelicAltarAccess::CanCommitFor(const UETRelicEquipmentComponent* Equipment) const
{
    UWorld* World = BoundWorld.Get();
    APlayerController* Controller = BoundController.Get();
    AEmberTrialCharacter* Body = BoundBody.Get();
    AETRelicAltar* Altar = BoundAltar.Get();
    UETRunSessionSubsystem* Session = BoundSession.Get();
    if (!bOpen || !IsValid(Equipment) || !IsValid(World) || World->bIsTearingDown ||
        !IsValid(Controller) || Controller->IsActorBeingDestroyed() ||
        !IsValid(Body) || !IsValid(Altar) || !IsValid(Session))
    {
        return false;
    }

    // 每次确认都重新读当前身份、距离 / 高差 / Enabled / Visibility与会话。
    // 只把“暂停检查”换成已打开页面的契约，其他运行态资格保持相同。
    return Controller->GetWorld() == World && Body->GetWorld() == World &&
        Altar->GetWorld() == World && Controller->GetPawn() == Body &&
        Body->GetController() == Controller && Equipment->GetOwner() == Body &&
        Equipment->GetWorld() == World && Session->GetGameInstance() == World->GetGameInstance() &&
        !Session->IsTravelPending() && UGameplayStatics::IsGamePaused(Body) &&
        UETRelicAltarSubsystem::IsEligibleBody(Body) && Altar->IsInReach(Body);
}

void FETRelicAltarAccess::Close()
{
    // Close后不可重新发放同一对象；新页面必须经TryOpen创建新上下文。
    bOpen = false;
    BoundSession.Reset();
    BoundAltar.Reset();
    BoundBody.Reset();
    BoundController.Reset();
    BoundWorld.Reset();
}
```

## 4. 原AltarSubsystem：提取共享资格

### 4.1 Public/Interaction/ETRelicAltarSubsystem.h

在public既有`static bool IsEligiblePlayer(...)`声明后添加以下声明；原声明、UFUNCTION与其他内容保留：

```cpp
    // 共用身体资格；不检查暂停，本身不是换装授权。
    static bool IsEligibleBody(const AEmberTrialCharacter* PlayerBody);
```

### 4.2 Private/Interaction/ETRelicAltarSubsystem.cpp

找到完整`IsEligiblePlayer`函数，从该函数签名到它的右大括号替换为下面**两个完整函数**。后面的RegisterAltar / UnregisterAltar / FindAvailableAltar / Deinitialize均保留；不要另留旧同名函数，也不要删除文件顶部原include：

```cpp
bool UETRelicAltarSubsystem::IsEligiblePlayer(const AEmberTrialCharacter* PlayerBody)
{
    return IsEligibleBody(PlayerBody) && !UGameplayStatics::IsGamePaused(PlayerBody);
}

bool UETRelicAltarSubsystem::IsEligibleBody(const AEmberTrialCharacter* PlayerBody)
{
    if (!IsValid(PlayerBody) || PlayerBody->IsActorBeingDestroyed() ||
        !PlayerBody->HasActorBegunPlay() || !PlayerBody->HasAuthority() ||
        PlayerBody->GetCombatTeam() != EETCombatTeam::Player ||
        !PlayerBody->IsGameplayReady() || PlayerBody->IsCombatDead() ||
        !PlayerBody->IsFormalActionIdle())
    {
        return false;
    }
    const UWorld* World = PlayerBody->GetWorld();
    const APlayerController* PC = Cast<APlayerController>(PlayerBody->GetController());
    const UETRelicEquipmentComponent* Equipment =
        PlayerBody->FindComponentByClass<UETRelicEquipmentComponent>();
    return World && !World->bIsTearingDown && World->GetNetMode() == NM_Standalone &&
        IsValid(PC) && !PC->IsActorBeingDestroyed() && PC->GetWorld() == World &&
        PC->IsLocalController() && PC->GetPawn() == PlayerBody &&
        Equipment && Equipment->IsEquipmentReady();
}
```

## 5. 原Equipment：两种入口共用一份提交

### 5.1 Public/Inventory/ETRelicEquipmentComponent.h

1. 在`class AEmberTrialCharacter;`附近添加前置声明：

```cpp
class FETRelicAltarAccess;
```

2. public保留原TryEquip声明，把其上方两条“后续入口 / 本章唯一调用者”旧注释改为“原PIE开发路径仍要求非暂停；不暴露BlueprintCallable”。在其声明后添加：

```cpp
    // 正式页面路径：只有仍有效的祭坛上下文可在暂停中提交，不能传bool绕过。
    FETRelicInventoryReply TryEquipAtAltar(const FETRelicAltarAccess& Access,
        EETRelicSlot Slot, FName RelicId, uint64 ExpectedRevision);
```

3. private中、原IsCurrentPlayerBody声明之前添加：

```cpp
    // 两个入口复用同一提交点；nullptr仅指原非暂停入口。
    FETRelicInventoryReply TryEquipInternal(EETRelicSlot Slot, FName RelicId,
        uint64 ExpectedRevision, const FETRelicAltarAccess* Access);
```

原friend、属性、委托和TryBuildPreview全部保留。三处新增均不加UFUNCTION。

### 5.2 Private/Inventory/ETRelicEquipmentComponent.cpp

1. 顶部原自身头文件include后增加：

```cpp
#include "Interaction/ETRelicAltarAccess.h"
```

2. 文件末尾找到注释“写入仍要求当前身体Idle / 非暂停”和其下完整TryEquip函数；将**这段注释与整个旧函数**替换为下面三函数。不要覆盖前面的TryMakePlan / TryBuildPreview、恢复 / GE所有权或事件实现；末尾只保留一份TryEquip：

```cpp
// ---- 两种入口，一份事务：普通非暂停 / 祭坛上下文内暂停确认 ----
FETRelicInventoryReply UETRelicEquipmentComponent::TryEquip(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision)
{
    return TryEquipInternal(Slot, RelicId, ExpectedRevision, nullptr);
}

FETRelicInventoryReply UETRelicEquipmentComponent::TryEquipAtAltar(
    const FETRelicAltarAccess& Access, const EETRelicSlot Slot, const FName RelicId,
    const uint64 ExpectedRevision)
{
    return TryEquipInternal(Slot, RelicId, ExpectedRevision, &Access);
}

FETRelicInventoryReply UETRelicEquipmentComponent::TryEquipInternal(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision,
    const FETRelicAltarAccess* Access)
{
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    if (bChanging || (IsValid(Inventory) && Inventory->bChanging))
    {
        return {EETRelicInventoryResult::Busy};
    }
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    if (!IsValid(Body) || !Body->IsFormalActionIdle() ||
        (Access ? !Access->CanCommitFor(this) : UGameplayStatics::IsGamePaused(this)))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    FETRelicInventoryPlan Plan;
    const FETRelicInventoryReply Reply = TryMakePlan(Slot, RelicId, ExpectedRevision, Plan);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return Reply;
    }
    // 方案预检后、产生任何副作用前再次核对页面上下文的实时条件。
    if (Access && !Access->CanCommitFor(this))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    return Inventory->TryCommitPlan(Plan,
        [this](const FETRelicLoadoutPlan& Committed, const uint64 Revision)
        {
            {
                TGuardValue<bool> ChangeGuard(bChanging, true);
                AppliedLoadout = Committed.After;
                AppliedRevision = Revision;
                WriteModifiersUnchecked(Committed.AfterModifiers);
            }
            OnEquipmentChanged.Broadcast(this);
        });
}
```

提交回调仍在库存已推进版本之后写同一GE，bChanging撤销后广播EquipmentChanged。没有复制一套GAS写入，也没有先解暂停提交再重新暂停。

## 6. 扩既有ETRelicAltarTests.cpp

位置仍为`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicAltarTests.cpp`，不要放到UI模块，不新建测试文件或删旧断言。

### 6.1 顶部include

保留最外层`#if WITH_DEV_AUTOMATION_TESTS`，在现有include组加入以下五条；已存在的不要重复：

```cpp
#include "Interaction/ETRelicAltarAccess.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Engine/GameInstance.h"
```

### 6.2 一处插入完整代码块

在RunTest末尾、原`Fixture.EndPlayInTestWorld();`**紧前面**插入下面整块。旧Near / Far销毁断言在它前面，清理 / ForwardErrorMessages / return与末尾#endif仍在后面。代码复用既有World / Body / PC / ASC / Pauser / Box / Spawn；不移到RunTest函数外，也不复制旧夹具。

```cpp
    // ---- C1：真实暂停中的类型化上下文；原非暂停入口继续拒绝 ----
    {
        using EResult = EETRelicInventoryResult;
        UETRelicInventorySubsystem* Inventory = World->GetGameInstance()
            ->GetSubsystem<UETRelicInventorySubsystem>();
        UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
        AETRelicAltar* Source = World->SpawnActor<AETRelicAltar>(
            FVector(100, 0, 200), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Access inventory"), Inventory) ||
            !TestNotNull(TEXT("Access equipment"), Equipment) ||
            !TestNotNull(TEXT("Access source"), Source)) return false;

        const FName LifeId(TEXT("life_ember"));
        const FName EdgeId(TEXT("keen_edge"));
        const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Max = UETHealthAttributeSet::GetMaxHealthAttribute();
        const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
        auto Access = FETRelicAltarAccess::TryOpen(PC, Source);
        if (!TestTrue(TEXT("Eligible nonpaused context opens"), Access.IsValid())) return false;
        TestFalse(TEXT("Context does not authorize unpaused commit"), Access->CanCommitFor(Equipment));
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        TestFalse(TEXT("No new context manufactured inside ordinary pause"),
            FETRelicAltarAccess::TryOpen(PC, Source).IsValid());
        TestTrue(TEXT("Opened context permits paused current body"), Access->CanCommitFor(Equipment));
        auto EquipAt = [&](EETRelicSlot Slot, FName Id)
        {
            return Equipment->TryEquipAtAltar(*Access, Slot, Id, Inventory->GetSnapshot().Revision).Result;
        };
        const uint64 BeforeRevision = Inventory->GetSnapshot().Revision;
        TestTrue(TEXT("Legacy paused write still rejected"), Equipment->TryEquip(
            EETRelicSlot::First, LifeId, BeforeRevision).Result == EResult::NotReady);

        ASC->SetNumericAttributeBase(Health, 75.0f);
        EResult Nested = EResult::Success;
        const FDelegateHandle ReentryHandle = ASC->GetGameplayAttributeValueChangeDelegate(Max).AddLambda(
            [&](const FOnAttributeChangeData&) { Nested = EquipAt(EETRelicSlot::Second, EdgeId); });
        TestTrue(TEXT("Paused life equip commits once"), EquipAt(EETRelicSlot::First, LifeId) == EResult::Success);
        ASC->GetGameplayAttributeValueChangeDelegate(Max).Remove(ReentryHandle);
        TestTrue(TEXT("Typed entry keeps reentrant Busy protection"), Nested == EResult::Busy);
        TestEqual(TEXT("Life equip max120"), ASC->GetNumericAttribute(Max), 120.0f);
        TestEqual(TEXT("Life equip does not heal"), ASC->GetNumericAttribute(Health), 75.0f);
        TestTrue(TEXT("Only one revision advanced"), Inventory->GetSnapshot().Revision == BeforeRevision + 1);
        TestTrue(TEXT("Old viewed version rejected"), Equipment->TryEquipAtAltar(
            *Access, EETRelicSlot::Second, EdgeId, BeforeRevision).Result == EResult::StalePlan);
        const uint64 StableRevision = Inventory->GetSnapshot().Revision;
        TestTrue(TEXT("Same equipped ID is NoChange"), EquipAt(EETRelicSlot::First, LifeId) == EResult::NoChange);
        TestTrue(TEXT("NoChange leaves version intact"), Inventory->GetSnapshot().Revision == StableRevision);
        TestTrue(TEXT("Duplicate other slot rejected"), EquipAt(EETRelicSlot::Second, LifeId) == EResult::RuleRejected);
        TestTrue(TEXT("Unowned ID rejected"), EquipAt(EETRelicSlot::Second, FName(TEXT("missing"))) == EResult::RuleRejected);
        TestTrue(TEXT("Second legal item commits"), EquipAt(EETRelicSlot::Second, EdgeId) == EResult::Success);
        TestEqual(TEXT("Real attack bonus5"), ASC->GetNumericAttribute(Bonus), 5.0f);

        const uint64 RejectedRevision = Inventory->GetSnapshot().Revision;
        auto RejectCurrent = [&]()
        {
            TestFalse(TEXT("Changed context not usable"), Access->CanCommitFor(Equipment));
            TestTrue(TEXT("Changed context write rejected"), EquipAt(EETRelicSlot::First, NAME_None) == EResult::NotReady);
            TestTrue(TEXT("Rejected context has no revision change"), Inventory->GetSnapshot().Revision == RejectedRevision);
            TestEqual(TEXT("Rejected context preserves max120"), ASC->GetNumericAttribute(Max), 120.0f);
        };
        Source->bEnabled = false;
        RejectCurrent();
        Source->bEnabled = true;
        Body->SetActorLocation(FVector(1000, 0, 200));
        RejectCurrent();
        Body->SetActorLocation(FVector(0, 0, 200));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        RejectCurrent();
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
        RejectCurrent();
        ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
        PC->UnPossess();
        TestFalse(TEXT("Lost possession invalidates current-body permission"), Access->CanCommitFor(Equipment));
        PC->Possess(Body);
        Equipment->RefreshEquipmentState();
        TestTrue(TEXT("Current identity and space restored"), Access->CanCommitFor(Equipment));

        ASC->SetNumericAttributeBase(Health, 110.0f);
        TestTrue(TEXT("Paused unequip uses same transaction"), EquipAt(EETRelicSlot::First, NAME_None) == EResult::Success);
        TestEqual(TEXT("Unequip clamps current100"), ASC->GetNumericAttribute(Health), 100.0f);
        TestEqual(TEXT("Unequip clamps base100"), ASC->GetNumericAttributeBase(Health), 100.0f);
        TestTrue(TEXT("Reequip succeeds"), EquipAt(EETRelicSlot::First, LifeId) == EResult::Success);
        TestEqual(TEXT("Reequip grants no free heal"), ASC->GetNumericAttribute(Health), 100.0f);
        Access->Close();
        TestFalse(TEXT("Closed page context remains revoked"), Access->CanCommitFor(Equipment));
        TestTrue(TEXT("Closed context cannot unequip"), EquipAt(EETRelicSlot::First, NAME_None) == EResult::NotReady);
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        auto Fresh = FETRelicAltarAccess::TryOpen(PC, Source);
        if (!TestTrue(TEXT("New page needs a fresh context"), Fresh.IsValid())) return false;
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        TestTrue(TEXT("Fresh context usable before source destruction"), Fresh->CanCommitFor(Equipment));
        Source->Destroy();
        TestFalse(TEXT("Destroyed original source never authorizes"), Fresh->CanCommitFor(Equipment));
        Fresh->Close();
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);

        AETRelicAltar* DeathSource = World->SpawnActor<AETRelicAltar>(
            FVector(100, 0, 200), FRotator::ZeroRotator, Spawn);
        auto DeathAccess = FETRelicAltarAccess::TryOpen(PC, DeathSource);
        if (!TestTrue(TEXT("Live body opens death-case context"), DeathAccess.IsValid())) return false;
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const uint64 BeforeDeathRevision = Inventory->GetSnapshot().Revision;
        ASC->SetNumericAttributeBase(Health, 0.0f);
        TestFalse(TEXT("Dead body cannot retain permission"), DeathAccess->CanCommitFor(Equipment));
        TestTrue(TEXT("Dead body write rejected"), Equipment->TryEquipAtAltar(
            *DeathAccess, EETRelicSlot::First, NAME_None, BeforeDeathRevision).Result == EResult::NotReady);
        TestTrue(TEXT("Death rejection leaves inventory intact"),
            Inventory->GetSnapshot().Revision == BeforeDeathRevision);
        DeathAccess->Close();
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    }
```

上下文空间测试复用原注册Box，通过NoCollision撤除，不依赖Destroy后的物理同步。这里测试的是Core / 真实库存与GE，不宣称已创建UMG页面或验证键盘焦点。会话Pending与换身体的既有事务回归由下节原EquipmentTransactions承担；正式页面所有者的生命周期在C2单独覆盖。

## 7. 正常构建与索引

确认UE仍关闭，所有七文件已保存。PowerShell逐条执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

第一条脚本正常完成Editor Build与三模块索引，再做Game Build；完成后CLion Reload CMake。预期Core76 / UI29 / Editor3，新Access.cpp属于Core目标，头导航正常。禁止把.cpp include到另一个.cpp来绕过目标归属。本章不改反射字段、Build.cs、Target或插件；发生错误发第一条error及前后约10行。

## 8. 两项自动化与约2分钟短查

### 8.1 两个既有专项各一次

开UE，非PIE时Output Log / Cmd依次运行（第一项完成再运行下一项）：

```text
Automation RunTests EmberTrial.M3.Interaction.RelicAltar
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
```

都应Success。第一项保留原发现 / 资格 / 意图断言，新增暂停中真实装卸、旧入口拒绝、过期 / 重复 / 重入、空间 / 身份变化、Close与来源销毁；第二项检查原提交 / 新身体恢复协议。其他30项沿用，不跑全组、不新Cook。若失败保留第一条Expected与日志，由我排查，不修改Expected以迎合错误值。

### 8.2 真实短查，不搭测试蓝图

| 组 | 操作 | 预期 |
| --- | --- | --- |
| 开关 | 正式地图Play，在祭坛旁E开页，P或鼠标继续；普通位置P开关一次 | 沿用B单页 / 暂停输入；本批没有确认按钮是预期 |
| 基线 | 左键一下、2一次、原E-A/B与移动 / 转向短查 | 原伤害与资源HUD保持；不要求重复连段 / Retry矩阵 |
| 清理 | Stop → 再Play一次，随后Stop | 新身体资源默认、无旧来源 / 页面错误；无改资产则无需强制重存 |

不新增实机“暂停中装备”诊断命令；C1的暂停写入由真实World自动化验证，C2再通过正式按钮验证。没有新增Source测试类、资产配置或临时按键要清理。

## 9. 简短反馈

```text
M3-04C1 完成 / 卡在第__节第__步
两新 / 五旧Core Source；Editor Build / Core76-UI29-Editor3索引 / Game Build：
Interaction.RelicAltar / Inventory.EquipmentTransactions各一次：
E祭坛页与普通P开关 / 无新确认按钮符合预期：
原HUD / 左键-2-E与移动短查 / Stop再开清理：
Source已保存，原资产与配置保持；失败或不理解处（无填无）：
完成时分 / 手动代码 / 阅读 / 检查 / 构建等待：
```

长日志由助手读取。C1验收无待补后直接下发C2，普通C++仍由你手动；复杂排错 / UI直接操作按当次授权。

## 10. 本批 Commit / Push（固定放在最后）

C1七份源码已在本地`e659e8c`，审查起点工作区干净。本轮Stage C1验收报告 / 收尾、C2新指南及同步状态文档；C2正式Source / 资产尚未实施，不Stage忽略Artifacts内的候选。未变化源码 / 资产不重复Stage；未联网核验远端。

- 英文建议：`docs: verify altar access and prepare relic confirmation UI`
- 中文译文：`docs: 完成祭坛换装上下文验收并准备遗物确认界面`

Stage / Commit / Push由用户操作；Raw / 候选 / 日志 / 缓存 / 包留本地。
以下为已提交的七份实现源码与本章记录，保留供核对；本轮只Stage实际文档差异：

```text
EmberTrial/Source/EmberTrial/Public/Interaction/ETRelicAltarAccess.h
EmberTrial/Source/EmberTrial/Private/Interaction/ETRelicAltarAccess.cpp
EmberTrial/Source/EmberTrial/Public/Interaction/ETRelicAltarSubsystem.h
EmberTrial/Source/EmberTrial/Private/Interaction/ETRelicAltarSubsystem.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicAltarTests.cpp
Docs/M3_04C1_RelicAltarAccess.md
当次Reports / Progress / Backlog / 路线等实际差异
```

- 实现英文：`feat: authorize paused relic transactions with altar contexts`
- 中文译文：`feat: 通过祭坛上下文授权暂停页面内的遗物换装事务`

无实际变化资产不重复Stage，AssetManifest在有实际资产改动时才更新。CLion刷新Changes → Stage最新修改 → Review → Commit / Push由你操作；Raw / 候选 / 日志 / 缓存 / 包留本地。
