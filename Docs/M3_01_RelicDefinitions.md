# M3-01：遗物静态定义、稳定ID与状态契约

更新2026-10-28，固定UE **5.5.4**。**本章已验收**，用户17:21全项完成、无失败 / 不理解处；代码配置40 / 检查2 / 等待5分钟，[实际报告](Reports/M3_01_Verification.md)已保存。当前进入[M3-02A两槽方案与纯预览](M3_02A_LoadoutRules.md)，下面代码 / 操作保留作参考，不要求重新执行。

原预计手动代码 / 配置45–75分钟、检查5–10分钟；实际手动合计42 / 等待5，已登记，不重复执行。代码可按完整文件复制；需要理解时优先看第2节分工及第4节注释，不要求逐字符敲入或重搭Controller验收图。

## 1. 本批目标、影响与保留基线

- 新增两件遗物定义：**生息余烬**（MaxHealth +20）与**砺锋残片**（LightAttackBonus +5）。两件都允许槽A / B；真正装备、最大生命120 / 每段基础30伤害是下一批效果，本章Play仍保持100 / 原25。
- 用UPrimaryDataAsset保存静态定义，ID与资产名 / 中文名分离；文本FText、图标软引用、排序值、槽位与类型化加法参数。
- 原生IsDataValid检查单资产；一个新自动化检查规则、跨资产重复ID、实际目录加载及AssetManager注册。四个新文件、Core Build.cs一个Editor条件依赖、DefaultGame.ini一个扫描条目。
- 玩家 / 敌人 / 训练目标、GAS / Character Ready、原HUD / 输入 / Flow均不改。本章不重跑旧23项，不新建GE蓝图、库存组件、装备组件、Presenter、Widget或地图。
- 当前已验收161 Source / 88cpp、Core62 / UI23 / Editor3、24测试、296 Content / 来源77。仅静态定义 / 注册已落地，正式Play仍100 / 原25，装备效果留02后续批次。

## 2. 先明确“定义 / 拥有 / 装备 / 选择 / 预览”

| 概念 | 谁负责 | 当前 / 后续边界 |
| --- | --- | --- |
| 静态定义 | 本章UETRelicDefinition | 所有人可共用；不能存ASC、当前生命、是否已装备或UI选择 |
| 拥有 | 后续GameInstance库存域，仅ID | 初始开发库存提供两件；掉落 / 拾取后续接，不由DataAsset标记“已拥有” |
| 已装备 | 后续库存域的已提交两槽ID + 身体装备组件效果 | 最多两件，同一RelicId不能占两槽；ASC句柄属于当前身体，不跨World |
| 选择 / 预览 | 后续页面Presenter | 移动选中 / 显示比较不提交装备，不向真实ASC临时应用 / 撤销GE |

本阶段规则先定：存活换装不免费补血，上限下降夹当前值；Retry / 本进程Continue保留已提交装备ID，重建身体后按最终上限回满生命 / 体力、治疗次数默认、冷却清空；NewRun确认后初始拥有 / 空槽，退出进程清空。M5再落实正式检查点快照，本章没有修改原会话行为。

只开放两种已有开发计划的效果类型，参数由DataAsset提供。M3-02统一用GAS应用 / 移除并扩攻击加成消费者；**这里不暴露任意GE Class供资源自由施加**，避免静态描述与真实规则不一致。后续六件原目标保持，新增机制再扩类型 / 消费者 / 校验，已有类型的新遗物只需数据。

`RelicId`规范为小写a–z / 数字 / 下划线、非None，例如`life_ember`。完整PrimaryAssetId为`ETRelic:life_ember`；`DA_ET_Relic_LifeEmber`只是资源文件名，中文名可改。发布后不随意改稳定ID；本批不做重命名试验。

## 3. 创建文件与目录（先保存并关闭UE）

保存所有资产并关闭EmberTrial UE编辑器。CLion保持整个工程工作区，用Project视图创建以下目录 / 文件，不通过UE Class Wizard创建DataAsset Blueprint类，也不在子目录打开新CLion工程。

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Inventory\ETRelicTypes.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Inventory\ETRelicDefinition.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Inventory\ETRelicDefinition.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicDefinitionsTests.cpp
```

`Inventory`拼写与大小写保持一致；Tests仍在Core模块现有目录。四文件是新建，不覆盖旧Character / 测试。每段代码完整写入同名文件；两个反射头的`.generated.h`都必须是最后一个include，不自己创建generated文件。

## 4. 四份完整代码

### 4.1 ETRelicTypes.h：槽位与加法参数

路径：`Source/EmberTrial/Public/Inventory/ETRelicTypes.h`。两个槽位是稳定规则枚举；Modifiers只存参数，不是GE运行实例。


```cpp
#pragma once

#include "CoreMinimal.h"
#include "ETRelicTypes.generated.h"

// 槽位是规则标识，不使用UI中的数组下标或显示文字作为存储ID。
UENUM(BlueprintType)
enum class EETRelicSlot : uint8
{
    First UMETA(DisplayName = "Slot A"),
    Second UMETA(DisplayName = "Slot B")
};

// 先实现两种确有消费者计划的加法效果；新机制在对应批次扩展。
UENUM(BlueprintType)
enum class EETRelicModifierKind : uint8
{
    MaxHealth UMETA(DisplayName = "Max Health"),
    LightAttackBonus UMETA(DisplayName = "Light Attack Bonus")
};

USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETRelicModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
    EETRelicModifierKind Kind = EETRelicModifierKind::MaxHealth;

    // 加法参数，允许代价型负值；最终属性合法下限由装备事务 / 属性集保证。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
    float AdditiveValue = 0.0f;
};
```

### 4.2 ETRelicDefinition.h：静态定义与只读接口

路径：`Source/EmberTrial/Public/Inventory/ETRelicDefinition.h`。friend只给本批自动化设置临时夹具，不提供Runtime随意修改资产的入口。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/ETRelicTypes.h"
#include "UObject/PrimaryAssetId.h"
#include "ETRelicDefinition.generated.h"

class UTexture2D;

/** 静态只读定义；不存已拥有 / 已装备、ASC、GE句柄或Widget状态。 */
UCLASS(BlueprintType)
class EMBERTRIAL_API UETRelicDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    static const FPrimaryAssetType AssetType;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    FName GetRelicId() const { return RelicId; }
    const FText& GetDisplayName() const { return DisplayName; }
    const FText& GetDescription() const { return Description; }
    const TSoftObjectPtr<UTexture2D>& GetIcon() const { return Icon; }
    int32 GetSortOrder() const { return SortOrder; }
    const TArray<FETRelicModifier>& GetModifiers() const { return Modifiers; }

    bool SupportsSlot(EETRelicSlot Slot) const;

    // 纯检查，不加载图标、不应用效果；输出由本次调用重新生成。
    bool CollectValidationErrors(TArray<FText>& OutErrors) const;
    static bool CollectCatalogErrors(const TArray<const UETRelicDefinition*>& Definitions,
                                     TArray<FText>& OutErrors);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    friend class FETRelicDefinitionsTest;

    // ---- 稳定身份与本地化展示；更名资产不应更改已经发出的RelicId ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Identity",
              meta = (AllowPrivateAccess = "true"))
    FName RelicId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true"))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true", MultiLine = "true"))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Presentation",
              meta = (AllowPrivateAccess = "true"))
    int32 SortOrder = 0;

    // ---- 装备资格与效果参数；运行时应用由后续Equipment组件负责 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Equipment",
              meta = (AllowPrivateAccess = "true"))
    TArray<EETRelicSlot> AllowedSlots = {EETRelicSlot::First, EETRelicSlot::Second};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Equipment",
              meta = (AllowPrivateAccess = "true"))
    TArray<FETRelicModifier> Modifiers;
};
```

### 4.3 ETRelicDefinition.cpp：身份与无副作用校验

路径：`Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp`。普通查询不加载图标；IsDataValid只在Editor编译，Runtime仍可用相同纯校验规则。

```cpp
#include "Inventory/ETRelicDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FPrimaryAssetType UETRelicDefinition::AssetType(TEXT("ETRelic"));

// 命名空间有明确文件前缀，避免Unity Build合并时与其他cpp的局部名称冲突。
namespace ETRelicDefinitionPrivate
{
bool IsStableId(const FName Id)
{
    if (Id.IsNone())
    {
        return false;
    }

    const FString Text = Id.ToString();
    for (const TCHAR Character : Text)
    {
        if (!((Character >= TEXT('a') && Character <= TEXT('z')) ||
              (Character >= TEXT('0') && Character <= TEXT('9')) || Character == TEXT('_')))
        {
            return false;
        }
    }
    return true;
}

bool IsKnownSlot(const EETRelicSlot Slot)
{
    return Slot == EETRelicSlot::First || Slot == EETRelicSlot::Second;
}

bool IsKnownKind(const EETRelicModifierKind Kind)
{
    return Kind == EETRelicModifierKind::MaxHealth ||
           Kind == EETRelicModifierKind::LightAttackBonus;
}
}

FPrimaryAssetId UETRelicDefinition::GetPrimaryAssetId() const
{
    return RelicId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(AssetType, RelicId);
}

bool UETRelicDefinition::SupportsSlot(const EETRelicSlot Slot) const
{
    return ETRelicDefinitionPrivate::IsKnownSlot(Slot) && AllowedSlots.Contains(Slot);
}

bool UETRelicDefinition::CollectValidationErrors(TArray<FText>& OutErrors) const
{
    OutErrors.Reset();
    auto AddError = [&](const TCHAR* Code, const TCHAR* Message)
    {
        OutErrors.Add(FText::FromString(FString::Printf(TEXT("[%s] %s: %s"), Code,
                                                       *GetPathName(), Message)));
    };

    // ---- ID / 展示契约；Icon可空，由未来View提供占位 ----
    if (!ETRelicDefinitionPrivate::IsStableId(RelicId))
    {
        AddError(TEXT("ET-RELIC-001"), TEXT("RelicId requires lowercase a-z, 0-9 or underscore."));
    }
    if (DisplayName.IsEmptyOrWhitespace() || Description.IsEmptyOrWhitespace())
    {
        AddError(TEXT("ET-RELIC-002"), TEXT("DisplayName and Description must contain text."));
    }

    // ---- 资格与数值契约；不把重复配置静默去重成另一份合法定义 ----
    TSet<EETRelicSlot> SeenSlots;
    bool bSlotsValid = !AllowedSlots.IsEmpty();
    for (const EETRelicSlot Slot : AllowedSlots)
    {
        bSlotsValid &= ETRelicDefinitionPrivate::IsKnownSlot(Slot) && !SeenSlots.Contains(Slot);
        SeenSlots.Add(Slot);
    }
    if (!bSlotsValid)
    {
        AddError(TEXT("ET-RELIC-003"), TEXT("AllowedSlots must be nonempty, known and unique."));
    }

    TSet<EETRelicModifierKind> SeenKinds;
    bool bModifiersValid = !Modifiers.IsEmpty();
    for (const FETRelicModifier& Modifier : Modifiers)
    {
        bModifiersValid &= ETRelicDefinitionPrivate::IsKnownKind(Modifier.Kind) &&
                           !SeenKinds.Contains(Modifier.Kind) &&
                           FMath::IsFinite(Modifier.AdditiveValue) && Modifier.AdditiveValue != 0.0f;
        SeenKinds.Add(Modifier.Kind);
    }
    if (!bModifiersValid)
    {
        AddError(TEXT("ET-RELIC-004"),
                 TEXT("Modifiers require unique known kinds and finite nonzero additive values."));
    }
    return OutErrors.IsEmpty();
}

bool UETRelicDefinition::CollectCatalogErrors(
    const TArray<const UETRelicDefinition*>& Definitions, TArray<FText>& OutErrors)
{
    OutErrors.Reset();
    TMap<FName, const UETRelicDefinition*> ById;
    for (const UETRelicDefinition* Definition : Definitions)
    {
        if (!IsValid(Definition))
        {
            OutErrors.Add(FText::FromString(TEXT("[ET-RELIC-005] Catalog contains an invalid object.")));
            continue;
        }

        TArray<FText> DefinitionErrors;
        Definition->CollectValidationErrors(DefinitionErrors);
        OutErrors.Append(MoveTemp(DefinitionErrors));
        if (Definition->RelicId.IsNone())
        {
            continue;
        }
        if (const UETRelicDefinition* const* Existing = ById.Find(Definition->RelicId))
        {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("[ET-RELIC-006] Duplicate RelicId '%s': %s / %s"),
                *Definition->RelicId.ToString(), *(*Existing)->GetPathName(), *Definition->GetPathName())));
        }
        else
        {
            ById.Add(Definition->RelicId, Definition);
        }
    }
    return OutErrors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UETRelicDefinition::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    const bool bDefinitionValid = CollectValidationErrors(Errors);
    for (const FText& Error : Errors)
    {
        Context.AddError(Error);
    }
    return bDefinitionValid && ParentResult != EDataValidationResult::Invalid
               ? EDataValidationResult::Valid
               : EDataValidationResult::Invalid;
}
#endif
```

### 4.4 ETRelicDefinitionsTests.cpp：规则与实物注册检查

路径：`Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp`。此测试只在Editor编译，强引用保留夹具 / 已加载定义到检查结束；不修改正式资源，不在测试中补注册扫描类型。

```cpp
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Inventory/ETRelicDefinition.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManager.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicDefinitionsTest, "EmberTrial.M3.Inventory.Definitions",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETRelicDefinitionsTest::RunTest(const FString& Parameters)
{
    // ---- 独立临时定义：不Spawn身体、不改CDO或磁盘资产 ----
    TStrongObjectPtr<UETRelicDefinition> Life(NewObject<UETRelicDefinition>());
    TStrongObjectPtr<UETRelicDefinition> Edge(NewObject<UETRelicDefinition>());
    auto Configure = [](UETRelicDefinition* Definition, const FName Id,
                        const EETRelicModifierKind Kind, const float Value)
    {
        Definition->RelicId = Id;
        Definition->DisplayName = FText::FromString(TEXT("Fixture"));
        Definition->Description = FText::FromString(TEXT("Definition contract fixture"));
        FETRelicModifier Modifier;
        Modifier.Kind = Kind;
        Modifier.AdditiveValue = Value;
        Definition->Modifiers = {Modifier};
    };
    Configure(Life.Get(), FName(TEXT("test_life")), EETRelicModifierKind::MaxHealth, 20.0f);
    Configure(Edge.Get(), FName(TEXT("test_edge")), EETRelicModifierKind::LightAttackBonus, 5.0f);

    TArray<FText> Errors;
    TestTrue(TEXT("Complete definition with placeholder icon is valid"),
             Life->CollectValidationErrors(Errors));
    TestTrue(TEXT("First slot supported"), Life->SupportsSlot(EETRelicSlot::First));
    TestTrue(TEXT("Second slot supported"), Life->SupportsSlot(EETRelicSlot::Second));
    TestFalse(TEXT("Unknown slot rejected"), Life->SupportsSlot(static_cast<EETRelicSlot>(255)));
    TestTrue(TEXT("Stable primary ID uses data ID"),
             Life->GetPrimaryAssetId() ==
                 FPrimaryAssetId(UETRelicDefinition::AssetType, FName(TEXT("test_life"))));

    Life->RelicId = NAME_None;
    TestFalse(TEXT("Empty ID rejected"), Life->CollectValidationErrors(Errors));
    TestFalse(TEXT("Empty ID has no valid primary ID"), Life->GetPrimaryAssetId().IsValid());
    Life->RelicId = FName(TEXT("bad.id"));
    TestFalse(TEXT("Punctuation ID rejected"), Life->CollectValidationErrors(Errors));
    Life->RelicId = FName(TEXT("test_life"));

    Life->AllowedSlots = {EETRelicSlot::First, EETRelicSlot::First};
    TestFalse(TEXT("Duplicate slot rejected"), Life->CollectValidationErrors(Errors));
    Life->AllowedSlots.Reset();
    TestFalse(TEXT("No compatible slot rejected"), Life->CollectValidationErrors(Errors));
    Life->AllowedSlots = {EETRelicSlot::First, EETRelicSlot::Second};
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN modifier rejected"), Life->CollectValidationErrors(Errors));
    Life->Modifiers[0].AdditiveValue = 0.0f;
    TestFalse(TEXT("Zero modifier rejected"), Life->CollectValidationErrors(Errors));
    Life->Modifiers[0].AdditiveValue = -10.0f;
    TestTrue(TEXT("Finite tradeoff value permitted"), Life->CollectValidationErrors(Errors));
    Life->Modifiers[0].AdditiveValue = 20.0f;
    // 先复制值，避免TArray扩容时传入指向自身元素的引用。
    const FETRelicModifier RepeatedModifier = Life->Modifiers[0];
    Life->Modifiers.Add(RepeatedModifier);
    TestFalse(TEXT("Repeated modifier kind rejected"), Life->CollectValidationErrors(Errors));
    Life->Modifiers.SetNum(1);

    TArray<const UETRelicDefinition*> Catalog = {Life.Get(), Edge.Get()};
    TestTrue(TEXT("Independent definitions form valid catalog"),
             UETRelicDefinition::CollectCatalogErrors(Catalog, Errors));
    Edge->RelicId = Life->RelicId;
    TestFalse(TEXT("Different objects with same ID rejected"),
              UETRelicDefinition::CollectCatalogErrors(Catalog, Errors));
    TestTrue(TEXT("Duplicate ID diagnostic is actionable"),
             Errors.ContainsByPredicate([](const FText& Error)
             {
                 return Error.ToString().Contains(TEXT("ET-RELIC-006"));
             }));
    Edge->RelicId = FName(TEXT("test_edge"));
    Catalog.Add(nullptr);
    TestFalse(TEXT("Null catalog entry rejected"),
              UETRelicDefinition::CollectCatalogErrors(Catalog, Errors));

    // ---- 实物目录 + 自动注册：从磁盘路径枚举，防PrimaryAssetId去重掩盖重复ID ----
    UAssetManager* Manager = UAssetManager::GetIfInitialized();
    if (!TestNotNull(TEXT("AssetManager initialized"), Manager))
    {
        return false;
    }
    FAssetRegistryModule& RegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& Registry = RegistryModule.Get();
    const FString Directory(TEXT("/Game/EmberTrial/Items/Relics"));
    Registry.ScanPathsSynchronous({Directory});
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*Directory));
    Filter.ClassPaths.Add(UETRelicDefinition::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> Assets;
    Registry.GetAssets(Filter, Assets);
    TestTrue(TEXT("At least two saved relic definitions discovered"), Assets.Num() >= 2);

    TArray<TStrongObjectPtr<UETRelicDefinition>> LoadedDefinitions;
    Catalog.Reset();
    TSet<FName> RegisteredIds;
    for (const FAssetData& Asset : Assets)
    {
        UETRelicDefinition* Definition = Cast<UETRelicDefinition>(Asset.GetAsset());
        if (!TestNotNull(FString::Printf(TEXT("Load %s"), *Asset.GetObjectPathString()), Definition))
        {
            continue;
        }
        LoadedDefinitions.Emplace(Definition);
        Catalog.Add(Definition);
        RegisteredIds.Add(Definition->GetRelicId());
        TestTrue(FString::Printf(TEXT("Configured AssetManager path matches %s"),
                                 *Definition->GetRelicId().ToString()),
                 Manager->GetPrimaryAssetPath(Definition->GetPrimaryAssetId()) ==
                     FSoftObjectPath(Definition));
    }
    const bool bCatalogValid = UETRelicDefinition::CollectCatalogErrors(Catalog, Errors);
    for (const FText& Error : Errors)
    {
        AddError(Error.ToString());
    }
    TestTrue(TEXT("Saved definitions satisfy catalog contract"), bCatalogValid);
    TestTrue(TEXT("Life Ember registered"), RegisteredIds.Contains(FName(TEXT("life_ember"))));
    TestTrue(TEXT("Keen Edge registered"), RegisteredIds.Contains(FName(TEXT("keen_edge"))));
    return !HasAnyErrors();
}

#endif
```

## 5. Core Build.cs与AssetManager共享配置

### 5.1 只给Editor编译的测试增加AssetRegistry依赖

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrial.Build.cs`。在构造函数内找到原来的：

```csharp
        PrivateIncludePaths.Add(ModuleDirectory);
```

**在这行上方插入一次**，不要另建构造函数、不要改Public依赖，也不要把整个Build.cs换成下面片段：

```csharp
        // 遗物目录集成测试只在Editor编译；Game不引入AssetRegistry依赖。
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("AssetRegistry");
        }

```

Core不是依赖本项目Editor模块。AssetRegistry是引擎模块，这里仅供`WITH_EDITOR`测试读取实际目录；DataAsset Runtime字段不依赖Editor / UnrealEd / UI。原UI / Editor Build.cs不改。

### 5.2 在DefaultGame.ini追加扫描条目

打开`E:\UE_Project\Demo\EmberTrial\Config\DefaultGame.ini`，在文件末尾追加下面完整区块，原Packaging / Cue / 地图配置全保留。当前基线没有该节；如果你已手动添加同名节，合入该节且同一ETRelic条目只保留一次，不添加`!PrimaryAssetTypesToScan=ClearArray`。

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="ETRelic",AssetBaseClass="/Script/EmberTrial.ETRelicDefinition",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/EmberTrial/Items/Relics")),SpecificAssets=(),Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
```

这是**一条完整配置行**；编辑器视觉自动折行可以，不能中间按Enter拆成多条。不要把C++前缀U写到`/Script/EmberTrial.ETRelicDefinition`里。

`bHasBlueprintClasses=False`因为创建的是原生类实例；`bIsEditorOnly=False`因为正式游戏也需要定义。只对这个受控小目录AlwaysCook，为没有地图硬引用的遗物保留Cook规则，不把整个Content / Tests改AlwaysCook。本批不实际Cook，因此不提前宣称已验证包内加载。软图标暂为None，后续加载 / Bundle与释放在列表批次处理。

## 6. 正常Build与索引（当前不要先跑自动化）

1. 确认四新文件、Build.cs与DefaultGame.ini已保存，UE已关闭。不要用Live Coding替代UHT / 正常Build。
2. PowerShell执行：

   ```powershell
   Set-Location 'E:\UE_Project\Demo'
   .\Tools\Refresh-CLion.ps1
   ```

3. 该脚本完成正常Editor构建与三模块索引输出。成功后CLion按现有方式Reload CMake / Refresh Changes；新Inventory公开头与两个cpp应属于Core目标。预计Core62 / UI23 / Editor3，计数只是辅助，不因文件展示顺序不同重建缓存。
4. 仍保持UE关闭，执行正常Game Development构建，检查Editor条件依赖不会泄入Game：

   ```powershell
   & 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
   ```

5. 成功后打开原`EmberTrial.uproject`，先做第7节两个实物资产。新自动化包含实际目录 / ID注册检查，**没有两份资产就运行会按设计失败**。

Build错误发第一条真正的C++ / UHT错误及其文件行号，不只发最后exit code6；不要自行删除所有缓存。`EDataValidationResult / FDataValidationContext`相关错误先确认头 / cpp完整、generated最后、WITH_EDITOR成对。

## 7. 创建两件遗物DataAsset（无蓝图连线）

### 7.1 目录与正确类型

1. Content Browser打开`Content/EmberTrial`，依次新建`Items`→`Relics`，最终虚拟路径`/Game/EmberTrial/Items/Relics`。
2. 在Relics空白处右键 → **Miscellaneous / 杂项 → Data Asset / 数据资产**；也可Add按钮进入同一菜单。不是Blueprint Class。
3. 弹出Pick Data Asset Class，搜索`ETRelicDefinition`，选刚编译的原生**ETRelicDefinition** → Select。资源命名`DA_ET_Relic_LifeEmber`。
4. 再按同样步骤创建第二份`DA_ET_Relic_KeenEdge`；不要从第一份复制后带着旧ID保存，避免向AssetManager短暂注册两个相同主资产ID。
5. 双击第一份，打开的是DataAsset Details。**DataAsset没有Blueprint的Compile按钮，设置后Save即可。**如果看到EventGraph / Class Defaults，说明建成了Blueprint Class，先不要继续填，反馈资源类型截图。

### 7.2 每一字段填写

Details清空搜索，展开`Relic`各子组。英文名可能自动加空格，使用下表。表中的“Kind / Additive Value”在Modifiers元素0里，不在顶部找。

| 字段 | DA_ET_Relic_LifeEmber | DA_ET_Relic_KeenEdge |
| --- | --- | --- |
| Relic Id | `life_ember` | `keen_edge` |
| Display Name | `生息余烬` | `砺锋残片` |
| Description | `最大生命 +20。存活换装不恢复当前生命。` | `每段轻攻击的基础伤害 +5，再按本段倍率结算。` |
| Icon | None，暂留空 | None，暂留空 |
| Sort Order | 10 | 20 |
| Allowed Slots元素数 | 2 | 2 |
| Allowed Slots [0] / [1] | Slot A / Slot B | Slot A / Slot B |
| Modifiers元素数 | **1** | **1** |
| Modifiers [0] → Kind | **Max Health** | **Light Attack Bonus** |
| Modifiers [0] → Additive Value | **20.0** | **5.0** |

操作数组的最小步骤：

1. Allowed Slots默认已有两项，展开确认[0]=Slot A、[1]=Slot B，不要再按+增加到四项；相同槽位重复会被拒绝。
2. Modifiers默认0项，在该行点**+一次**，展开Index[0]；在它内部设置Kind与Additive Value。不要把Sort Order误当加成数值。
3. 两资产分别Ctrl+S，确认星号消失；不要给它们设置“已拥有 / 已装备”字段，本章没有这些字段。

### 7.3 单资产校验与一次正常重启

1. 回Content Browser，选中两份DataAsset，右键 **Asset Actions → Validate Assets**（本地化可能叫验证资产）。用当前原生`IsDataValid`，不需要另建Validator蓝图。
2. 预期两份均Valid / 无Error；不要强求旧M2 Validator的`M2-05…Valid`日志前缀，新类走自己的原生校验。
3. 有错误按ET-RELIC代码对应修：001为ID格式，002为名称 / 说明空白，003为槽位空 / 非法 / 重复，004为参数空 / 未知 / 重复 / 非有限或0。005 / 006由目录总校验检查无效对象 / 重复ID，自动化用临时对象验证，不要求你制造磁盘负例。
4. **Save All → 正常关闭UE → 再打开同一工程一次。**这次重启是让真实启动扫描读取已保存ID，检验共享Config可复现；不清缓存、不重新Build，不用测试里手动注册来掩盖配置缺失。

## 8. 最小检查：一项新自动化 + 一次短基线

### 8.1 自动化（不要点Play）

重开UE后，保持非PIE，在Output Log的命令输入栏执行：

```text
Automation RunTests EmberTrial.M3.Inventory.Definitions
```

等它明确输出该项**Success**再继续；只执行这一次，不默认重跑旧23项。此项同时检查临时非法定义、两件正式资源、目录内重复ID与每个ID注册到正确资产路径；不Spawn身体、不Apply GE、不修改磁盘DataAsset。

如果提示`No automation tests matched`，确认文件在Core/Tests、四文件写全并已正常Editor Build，而不是只构建Game。如果`At least two…`失败，核对真实目录 / 类型 / Save；如果`Configured AssetManager path…`失败，核对第5.2节完整条目、保存ID和第7.3节重启。不要在测试里加`ScanPathsForPrimaryAssets`动态注册让错误配置假通过。

### 8.2 一次Play短基线（约1–2分钟）

1. 打开原正式`L_ET_VerticalSlice`，Play一次；生命 / 体力100、治疗3、F就绪，原各HUD与锁定组件无重复。
2. 对训练目标左键单点仍真实25；按2一次仍消费25体力 / 位移恢复。**本章没有装备入口，不能期待生命120或伤害30。**
3. E一次提示 / 交互、P一种继续方式、移动 / 镜头 / 跳跃正常后Stop。无需再重做十次重建、死亡 / Retry / Continue或全组。
4. Save All，保留原100 / 默认配置，不新增临时Controller变量 / 按键 / Graph。发反馈后我读日志、核对Source / Config / 资产实物，登记来源清单；无问题直接进入M3-02首批。

## 9. 为后续实现保留的解释

本章测试的目录与两件ID是集成夹具，不是Runtime里写死每件遗物分支。`ETRelic`是全项目类型契约，扫描目录属于Config，数值只来自DataAsset；Getter / 纯校验不应用效果，不增加常驻Tick / Timer / 订阅。

单资产合法不代表目录ID唯一，AssetManager按ID建表会去重，不能只遍历它的结果就声称没有重复。因此测试先从AssetRegistry按真实路径读取每一份，再做目录校验，并单独核对自动注册路径。

允许有限负值是为取舍型遗物预留数据能力，不意味着已允许最终MaxHealth≤0；应用时的整体合法性、无效请求保持旧装备、GE句柄和资源恢复顺序仍留M3-02。Icon为空是当前约定，填写软引用也不等于完成异步加载、Bundle卸载或Cook实证。

## 10. 简短反馈

```text
M3-01 完成 / 卡在第__节第__步
四新Source / Core Build.cs / DefaultGame.ini已保存；Editor Build / 三索引 / Game Build：
两DataAsset目录与类型 / ID / 槽位 / 参数；两份Valid并正常保存重开：
Inventory.Definitions一次：
正式地图原100 / 25伤害 / 2-E-P与基础输入短基线：
Source / Config / 两资产Save All；失败或不理解处（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

不用贴成功长日志；报错给第一条文字 / 路径 / 行号，字段或类型定位给含资源标题与Details的截图。本章没有复杂动态图或需要精确按键时点的检查。

## 11. 本批 Commit / Push（固定放在最后）

[M3-01报告](Reports/M3_01_Verification.md)已保存，当前[M3-02A两槽方案 / 纯预览](M3_02A_LoadoutRules.md)待用户手动实现。本地81a32c7已含01实现 / 两资产；本轮只新增报告 / A指南并同步清单 / 入口，精确13文件及未来三新 / 一旧Source范围见[A末尾](M3_02A_LoadoutRules.md#11-本批-commit--push固定放在最后)。文档建议英文`docs: verify relic definitions and prepare loadout rules`，中文译文`docs: 记录遗物定义验收并准备装备方案规则`。

实现后建议英文`feat: add relic loadout planning and stat previews`，中文译文`feat: 新增遗物装备方案与属性预览`。运行工程 / Config / Tools与原包本轮保持；无差异旧实现 / 资产不重复Stage，Raw / 日志 / 包留本地。用户在CLion Stage最新完整内容 / Review Diff / Commit / Push，助手未改索引或核验远端。
