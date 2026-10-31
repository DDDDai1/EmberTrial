# M3-04A：祭坛发现与使用资格

2026-10-30，UE5.5.4。本章已验收，用户17:17全项反馈、专项Success / 双Build与清理证据见[报告](Reports/M3_04A_Verification.md)。人工70 / 等待5分钟，无待补；进入[M3-04B浏览流程](M3_04B_RelicAltarBrowseFlow.md)，下一批仅指南准备。以下代码与步骤保留作学习依据。

## 1. 本批交付与后续分批

正式地图新增一座灰盒遗物祭坛。原生WorldSubsystem登记当前世界的祭坛；未来交互请求按需查询最近可用的一座，校验当前本地存活、Ready / EquipmentReady、Idle、未暂停、距离 / 高差 / 遮挡。出生已在范围内直接查询有效；离开、禁用、销毁后查询即时失效。

本批先提供可复用的领域入口基础与可见占位物。**尚不接E、不打开新页面、不确认换装**；旧E-A/B与P只读浏览继续正常。原TryEquip暂停拒绝和PIE开发入口保持；完成本批不代表祭坛权限已经约束所有装备写入。

| 后续批次 | 主要工作 |
| --- | --- |
| A，本批 | World作用域登记 / 发现、空间与身体资格、灰盒祭坛 |
| B，下一批 | 复用既有E链转交祭坛意图，页面 / 暂停所有权与关闭恢复；避免第二条E处理链 |
| C | 页面确认 / 卸装、最终权限与版本重验、输入 / 身体 / 世界结束边界 |

“可用”在A只表示满足入口条件，不代表已经隔绝敌人或提供无敌。地图先选敌人感知范围之外的位置；B / C再处理打开页面时的安全与暂停事务。不能通过本章原生查询跳过最终确认。

预计代码70–100分钟、资产 / 地图15–25分钟、检查5–10分钟，Build等待另计。**五新Core Source、一旧Character.h仅补测试friend**；新增一个BP资产，修改正式地图。当前186 Source / 104cpp（72 / 29 / 3）、31测试 / 298 Content / 来源79；完成后预计191 Source / 107cpp（75 / 29 / 3）、32测试 / 299 Content / 来源80，以实际验收为准。

## 2. 架构与范围约定

| 类型 | 所有权与职责 |
| --- | --- |
| AETRelicAltar | 配置、标记、空间只读查询；BeginPlay注册 / EndPlay注销 |
| UETRelicAltarSubsystem | 当前Game / PIE World的弱索引；按请求选择最近可用祭坛；世界结束清空 |
| 既有Equipment / Inventory | 仍拥有属性、装备事务与GI库存；本批不迁移 |
| 后续UI Flow | 只接收意图；页面的暂停所有权和确认令牌在B / C设计 |

不依赖Begin / End Overlap去缓存一个“允许换装”布尔值；按请求读取当前身体位置与资格，因此不需要出生NextTick补Overlap。每次查询只遍历已注册祭坛，候选再做Visibility检测，无业务Tick / Timer / 每帧全世界扫描。IsInReach只回答空间问题，不能单独作为装备权限。

InteractionRadius是世界XY距离，MaxVerticalDistance是角色胶囊中心与祭坛原点的Z差；边界包含等号。整体Actor保持Scale1，缩放只改Marker。Visibility射线从身体胶囊中心到祭坛SightPointOffset，忽略身体与祭坛自身；阻挡物须真实Block Visibility。

## 3. 手动创建五份源码

Save All并关闭UE；在CLion原项目内手动建目录与文件。以下路径均相对于`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial`，**不放到EmberTrialUI或Editor模块**。不通过UE建自动父类模板，不改Build.cs / Target / uproject。现有Core的Engine / GAS依赖满足本批。

文件名含generated.h的只有两个公开头；generated.h是最后一条include。不要在cpp中include generated.h。

### 3.1 Public/Interaction/ETRelicAltarSubsystem.h

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Interaction\ETRelicAltarSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ETRelicAltarSubsystem.generated.h"

class AEmberTrialCharacter;
class AETRelicAltar;

// 当前World的祭坛索引：只弱持有注册对象，按请求查询，不扫描全世界或每帧轮询。
UCLASS()
class EMBERTRIAL_API UETRelicAltarSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Relic Altar")
    AETRelicAltar* FindAvailableAltar(AEmberTrialCharacter* PlayerBody) const;

    // 此处是打开入口前的运行态资格；不表示已获得页面中的换装许可。
    static bool IsEligiblePlayer(const AEmberTrialCharacter* PlayerBody);
    virtual void Deinitialize() override;

protected:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    friend class AETRelicAltar;
    void RegisterAltar(AETRelicAltar* Altar);
    void UnregisterAltar(AETRelicAltar* Altar);

    TSet<TWeakObjectPtr<AETRelicAltar>> Altars;
    bool bClosing = false;
};
```

### 3.2 Private/Interaction/ETRelicAltarSubsystem.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Interaction\ETRelicAltarSubsystem.cpp`

```cpp
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Interaction/ETRelicAltar.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Kismet/GameplayStatics.h"

bool UETRelicAltarSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UETRelicAltarSubsystem::IsEligiblePlayer(const AEmberTrialCharacter* PlayerBody)
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
    return World && World->GetNetMode() == NM_Standalone &&
        !UGameplayStatics::IsGamePaused(PlayerBody) && IsValid(PC) &&
        PC->IsLocalController() && PC->GetPawn() == PlayerBody &&
        Equipment && Equipment->IsEquipmentReady();
}

void UETRelicAltarSubsystem::RegisterAltar(AETRelicAltar* Altar)
{
    if (!bClosing && IsValid(Altar) && Altar->GetWorld() == GetWorld() && Altar->HasActorBegunPlay())
    {
        Altars.Add(TWeakObjectPtr<AETRelicAltar>(Altar));
    }
}

void UETRelicAltarSubsystem::UnregisterAltar(AETRelicAltar* Altar)
{
    Altars.Remove(TWeakObjectPtr<AETRelicAltar>(Altar));
}

AETRelicAltar* UETRelicAltarSubsystem::FindAvailableAltar(AEmberTrialCharacter* PlayerBody) const
{
    if (bClosing || !IsEligiblePlayer(PlayerBody) || PlayerBody->GetWorld() != GetWorld())
    {
        return nullptr;
    }
    AETRelicAltar* Best = nullptr;
    double BestDistanceSquared = TNumericLimits<double>::Max();
    for (const TWeakObjectPtr<AETRelicAltar>& Candidate : Altars)
    {
        AETRelicAltar* Altar = Candidate.Get();
        if (!IsValid(Altar) || !Altar->IsInReach(PlayerBody))
        {
            continue;
        }
        const double DistanceSquared = FVector::DistSquared2D(
            PlayerBody->GetActorLocation(), Altar->GetActorLocation());
        // 完全同距时按对象路径破同分，不依赖TSet遍历顺序。
        if (!Best || DistanceSquared < BestDistanceSquared ||
            (DistanceSquared == BestDistanceSquared && Altar->GetPathName() < Best->GetPathName()))
        {
            Best = Altar;
            BestDistanceSquared = DistanceSquared;
        }
    }
    return Best;
}

void UETRelicAltarSubsystem::Deinitialize()
{
    bClosing = true;
    Altars.Reset();
    Super::Deinitialize();
}
```

### 3.3 Public/Interaction/ETRelicAltar.h

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Interaction\ETRelicAltar.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETRelicAltar.generated.h"

class AEmberTrialCharacter;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// 祭坛提供空间条件；库存、页面和暂停各由原有服务负责。
UCLASS()
class EMBERTRIAL_API AETRelicAltar : public AActor
{
    GENERATED_BODY()

public:
    AETRelicAltar();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Relic Altar")
    bool CanUseAltar(AEmberTrialCharacter* PlayerBody) const;

    // 空间只读查询不检查暂停；后续页面必须另核验身份 / 权限，不能凭它直接写装备。
    bool IsInReach(const AEmberTrialCharacter* PlayerBody) const;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETRelicAltarTest;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> AltarRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> RangePreview;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Marker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar",
              meta = (ClampMin = "1.0", Units = "cm"))
    float InteractionRadius = 180.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar",
              meta = (ClampMin = "0.0", Units = "cm"))
    float MaxVerticalDistance = 180.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar")
    FVector SightPointOffset = FVector(0.0f, 0.0f, 80.0f);

    bool bEndingPlay = false;
};
```

### 3.4 Private/Interaction/ETRelicAltar.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Interaction\ETRelicAltar.cpp`

```cpp
#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Characters/EmberTrialCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicAltar, Log, All);

AETRelicAltar::AETRelicAltar()
{
    PrimaryActorTick.bCanEverTick = false;
    AltarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AltarRoot"));
    SetRootComponent(AltarRoot);

    RangePreview = CreateDefaultSubobject<USphereComponent>(TEXT("RangePreview"));
    RangePreview->SetupAttachment(AltarRoot);
    RangePreview->InitSphereRadius(InteractionRadius);
    RangePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RangePreview->SetGenerateOverlapEvents(false);
    RangePreview->SetCanEverAffectNavigation(false);
    RangePreview->SetHiddenInGame(true);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Marker->SetupAttachment(AltarRoot);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetGenerateOverlapEvents(false);
    Marker->SetCanEverAffectNavigation(false);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(AltarRoot);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    Label->SetText(FText::FromString(TEXT("RELIC ALTAR")));
    Label->SetTextRenderColor(FColor::Cyan);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(20.0f);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCanEverAffectNavigation(false);
}

void AETRelicAltar::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    // 预览圆只辅助搭图；非法配置仍由真实查询拒绝，不靠编辑器Clamp保证正确性。
    RangePreview->SetSphereRadius(FMath::IsFinite(InteractionRadius)
        ? FMath::Max(0.0f, InteractionRadius) : 0.0f, false);
}

void AETRelicAltar::BeginPlay()
{
    Super::BeginPlay();
    if (UETRelicAltarSubsystem* Registry = GetWorld()->GetSubsystem<UETRelicAltarSubsystem>())
    {
        Registry->RegisterAltar(this);
    }
    UE_LOG(LogETRelicAltar, Display, TEXT("[M3-04A] Altar ready Actor=%s"), *GetName());
}

bool AETRelicAltar::CanUseAltar(AEmberTrialCharacter* PlayerBody) const
{
    return UETRelicAltarSubsystem::IsEligiblePlayer(PlayerBody) && IsInReach(PlayerBody);
}

bool AETRelicAltar::IsInReach(const AEmberTrialCharacter* PlayerBody) const
{
    if (bEndingPlay || !bEnabled || !HasActorBegunPlay() || IsActorBeingDestroyed() ||
        !IsValid(PlayerBody) || PlayerBody->IsActorBeingDestroyed() ||
        PlayerBody->GetWorld() != GetWorld() || !GetWorld() ||
        !FMath::IsFinite(InteractionRadius) || InteractionRadius <= 0.0f ||
        !FMath::IsFinite(MaxVerticalDistance) || MaxVerticalDistance < 0.0f ||
        SightPointOffset.ContainsNaN())
    {
        return false;
    }
    const FVector From = PlayerBody->GetActorLocation();
    const FVector Origin = GetActorLocation();
    const FVector To = GetActorTransform().TransformPosition(SightPointOffset);
    if (From.ContainsNaN() || Origin.ContainsNaN() || To.ContainsNaN() ||
        FVector::DistSquared2D(From, Origin) > FMath::Square(static_cast<double>(InteractionRadius)) ||
        FMath::Abs(From.Z - Origin.Z) > MaxVerticalDistance)
    {
        return false;
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETRelicAltarReach), false, PlayerBody);
    Query.AddIgnoredActor(this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query);
}

void AETRelicAltar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (UWorld* World = GetWorld())
    {
        if (UETRelicAltarSubsystem* Registry = World->GetSubsystem<UETRelicAltarSubsystem>())
        {
            Registry->UnregisterAltar(this);
        }
    }
    UE_LOG(LogETRelicAltar, Display, TEXT("[M3-04A] Altar end Actor=%s"), *GetName());
    Super::EndPlay(EndPlayReason);
}
```

### 3.5 Tests/ETRelicAltarTests.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicAltarTests.cpp`

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicAltarTest, "EmberTrial.M3.Interaction.RelicAltar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicAltarTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>();
    AETRelicAltar* Near = World->SpawnActor<AETRelicAltar>(
        FVector(100, 0, 200), FRotator::ZeroRotator, Spawn);
    AETRelicAltar* Far = World->SpawnActor<AETRelicAltar>(
        FVector(150, 0, 200), FRotator::ZeroRotator, Spawn);
    UETRelicAltarSubsystem* Registry = World->GetSubsystem<UETRelicAltarSubsystem>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("PC"), PC) ||
        !TestNotNull(TEXT("Near altar"), Near) || !TestNotNull(TEXT("Far altar"), Far) ||
        !TestNotNull(TEXT("World registry"), Registry))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Player;
    Body->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
    PC->Possess(Body);
    TestNull(TEXT("Before BeginPlay no usable altar"), Registry->FindAvailableAltar(Body));
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    // 本测试没有地面也不推进世界时间；启动后冻结移动，避免重力改变几何夹具。
    Body->GetCharacterMovement()->DisableMovement();
    TestTrue(TEXT("Initial in range works without overlap event"), Registry->FindAvailableAltar(Body) == Near);
    TestTrue(TEXT("Direct eligibility agrees"), Near->CanUseAltar(Body));
    Body->SetActorLocation(FVector(1000, 0, 200));
    TestNull(TEXT("Leaving range clears query"), Registry->FindAvailableAltar(Body));
    Body->SetActorLocation(FVector(0, 0, 200));
    Near->bEnabled = false;
    TestTrue(TEXT("Disabled nearest falls back"), Registry->FindAvailableAltar(Body) == Far);
    Near->bEnabled = true;
    Near->MaxVerticalDistance = 10.0f;
    Near->SetActorLocation(FVector(100, 0, 220));
    TestFalse(TEXT("Different floor rejected"), Near->IsInReach(Body));
    Near->MaxVerticalDistance = 180.0f;
    Near->SetActorLocation(FVector(100, 0, 200));
    Near->InteractionRadius = 0.0f;
    TestFalse(TEXT("Invalid radius rejected"), Near->CanUseAltar(Body));
    Near->InteractionRadius = 180.0f;

    // 真实Visibility阻挡，使用NoCollision撤除，避免Destroy后物理场景尚未同步。
    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Wall"), Wall)) return false;
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(10, 100, 100));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(50, 0, 240));
    TestNull(TEXT("Wall blocks both altars"), Registry->FindAvailableAltar(Body));
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestTrue(TEXT("Wall removal permits nearest"), Registry->FindAvailableAltar(Body) == Near);

    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("ASC"), ASC)) return false;
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestNull(TEXT("Busy rejected"), Registry->FindAvailableAltar(Body));
    ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pauser"), Pauser)) return false;
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestNull(TEXT("Paused opening rejected"), Registry->FindAvailableAltar(Body));
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    PC->UnPossess();
    TestNull(TEXT("Unpossessed body rejected"), Registry->FindAvailableAltar(Body));
    PC->Possess(Body);
    TestTrue(TEXT("Restored current body"), Registry->FindAvailableAltar(Body) == Near);
    Body->CombatTeam = EETCombatTeam::Enemy;
    TestNull(TEXT("Enemy cannot open"), Registry->FindAvailableAltar(Body));
    Body->CombatTeam = EETCombatTeam::Player;
    Near->Destroy();
    TestTrue(TEXT("Destroyed nearest falls back"), Registry->FindAvailableAltar(Body) == Far);
    Far->Destroy();
    TestNull(TEXT("No old altar retained"), Registry->FindAvailableAltar(Body));
    Fixture.EndPlayInTestWorld();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

### 3.6 既有Character.h补一个测试friend

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h`。

在原有`friend class FETRelicPresenterTest;`后新增一行，原friend全部保留：

```cpp
    friend class FETRelicAltarTest;
```

新测试沿用既有身体夹具，需要在BeginPlay前设置私有LightAttackAbilityClass，让真实装备恢复按原条件完成。不新增生产用测试开关或公开Setter，不改Character.cpp。

## 4. 正常构建与索引

五个新文件与Character.h都保存后，保持UE关闭，在PowerShell依次操作：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后在CLion Reload CMake，确认Core CodeModel能找到三个新cpp与两个头，generated.h不手工创建。再运行Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功后打开工程。若UBT失败，发第一条具体error及上下约10行，不只发exit code 6。普通C++由你手动；复杂排错按当次授权处理。

## 5. 创建一座灰盒祭坛

### 5.1 目录与父类

1. 在Content Browser的`/Game/EmberTrial/Blueprints`新建Interaction文件夹；在其中右键 → Blueprint Class → All Classes，搜索**ETRelicAltar**，选原生父类创建，命名**BP_ETRelicAltar**。不从PracticeActor / Checkpoint复制。
2. 打开BP，Components应有AltarRoot、RangePreview、Marker、Label。Event Graph保持空，不增加Overlap或E按键逻辑。
3. 点Class Defaults，搜索`Altar`：Enabled=True，Interaction Radius=180，Max Vertical Distance=180，Sight Point Offset=(0,0,80)。实际属性在EmberTrial → Altar分类中。无库存 / WidgetClass配置。

### 5.2 标记模型与碰撞

1. Components选**Marker**，Details的Static Mesh选引擎自带**Cylinder**（路径`/Engine/BasicShapes/Cylinder`）。找不到时Content Browser Settings勾Show Engine Content，再搜索Cylinder；不下载资源。
2. Marker相对Location=(0,0,30)，Rotation=(0,0,0)，Scale=(0.6,0.6,0.6)。只改组件，Actor整体保持Scale1。
3. Marker → Collision Presets=NoCollision、Generate Overlap Events=False、Can Ever Affect Navigation=False。RangePreview同样NoCollision / Overlap False / Navigation False；其圆仅编辑器辅助，没有实际阻挡。
4. Label沿用原生RELIC ALTAR英文灰盒文字，World Size20、Z100；它不表示本批已经有“按E打开”提示，不换中文字体。后续重要模型再接Blender，保留这份占位可替换。
5. **先Compile至绿色，再Save**。若Message Log保留旧红行，以最新时间的Valid / Invalid为准，不能用历史行判断本次编译失败。

### 5.3 地图

1. 打开`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，只放一份BP_ETRelicAltar，Outliner名**RelicAltar_A**。不移走原Entry / Checkpoint / 训练目标 / 敌人。
2. 在出生附近地面选空位：祭坛原点贴地，距Entry约200–400cm，距正式敌人至少1500cm，避开已有E-A/B交互范围。可沿地面拖动，不把原点设到标记顶面；记录最终XYZ坐标即可。
3. 原地图若无法满足该间距，先选敌人未获得目标的空位，不改敌人感知 / 默认血量来凑条件。A没有战斗隔离功能。
4. Actor Rotation可绕Z调整文字可读方向，Scale保持(1,1,1)。Compile / Save BP，再Save地图。标记无碰撞，不要求额外Build Paths。

## 6. 分组检查

### 6.1 一个新专项

非PIE时，UE Output Log命令栏执行一次：

```text
Automation RunTests EmberTrial.M3.Interaction.RelicAltar
```

应Success。它覆盖真实注册、初始范围、离开、禁用回退、高差 / 非法参数、Visibility挡板、忙碌 / 暂停 / 无控制 / 敌方拒绝，以及销毁清理；不需要手搭测试蓝图。此批未改库存 / UI，不重复全31项或上章两项。

### 6.2 实机只检查装配与兼容（约5分钟）

1. 正式地图Play，看到一座灰盒柱与文字，走近 / 穿过模型不受阻挡。**本章走近没有新HUD提示、按E不会打开祭坛页面，属于预期。**不要重复给E绑定。
2. 跑到原A/B确认原E仍工作；P打开原遗物页，搜索一次、Esc离开再P关闭，最后移动 / 左键 / 2短查即可。没有新的临时测试函数。
3. Stop再Play一次，新的Altar ready；Stop有Altar end，原HUD无旧页面 / 旧World错误。长日志由我读取，不要求手抄名字或每条时间。
4. 保持正式默认值、单祭坛、Scale1；Save All。回报最终位置及是否挡住原出生 / 交互路径。无需新Cook或Retry / Continue完整矩阵。

如专项失败，发失败断言原文 / 文件行号；如模型或Components不符，发对应Details / Hierarchy截图。不要为说明一个问题重做整套场景。

## 7. 简短反馈与理解重点

```text
M3-04A 完成 / 卡在第__节第__步
五新Core / Character.h测试friend；关闭UE后Editor Build / 三索引 / Game Build：
原生Altar父类 / 四组件 / 180-180-80 / NoCollision-Navigation False / Compile-Save：
正式地图单RelicAltar_A，最终位置XYZ / Actor Scale1：
Interaction.RelicAltar一次：
灰盒无阻挡、没有E新页面符合预期 / 原E-P搜索与能力短查：
两轮Play注册 / Stop清理、默认值 / Source资产Save All：
失败或最明显灰盒问题（无填无）：
完成时分 / 手动代码 / 资产地图 / 检查 / 构建等待：
```

应能说明：World与GI寿命为何不同，弱索引为何不拥有祭坛，为什么每次查询都读当前资格而非永久缓存允许状态，以及空间可达与最终装备权限为何必须区分。后续打开页面不允许通过删除TryEquip的暂停保护来绕过输入 / 所有权设计。

## 8. 本批 Commit / Push（固定放在最后）

[M3-04A验收报告](Reports/M3_04A_Verification.md)已保存，无待补；本地`522edb9`包含源码 / 新BP / 地图，起点工作区干净。当前Stage A报告 / 收尾、[B浏览流程指南](M3_04B_RelicAltarBrowseFlow.md)、AssetManifest与同步状态文档；B尚未实施，不提交本地候选。未联网核验远端。

英文建议`docs: verify altar discovery and prepare altar browsing flow`；中文`docs: 完成祭坛发现验收并准备祭坛浏览流程`。Stage / Commit / Push由用户操作；Raw / 日志 / 缓存 / 包留本地。

A实现范围为五新Core、Character.h测试friend、一新BP与正式地图，已在本地提交；英文实现标题`feat: add world scoped relic altar discovery and eligibility checks`，中文`feat: 新增世界级遗物祭坛发现与使用资格校验`。无需重复Stage无变化实物。
