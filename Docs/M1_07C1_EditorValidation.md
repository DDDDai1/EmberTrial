# M1-07C1：Editor 模块与 BT / UMG 配置校验

更新：2026-10-14；固定 UE **5.5.4**。前置：[B性能报告及最终函数收尾](Reports/M1_07B_Verification.md)。本批代码只写在指南中，由你手动创建、填写和构建；当前工程尚无 `EmberTrialEditor` 模块，以下均为待实施内容。

## 1. 本批交付与时间

建立一个只在编辑器加载的模块，使用 UE 自带 **Data Validation** 检查追击 BT 的黑板绑定和 UMG 的必需控件。以后修改这两类配置，可在 Content Browser 右键检查，不必反复搭临时 Controller 函数。错误包含资产路径、规则 ID 和修复提示。

预计手动 **60–100分钟**：文件与代码35–60、模块 / 索引配置15–25、专项检查5–10、记录5；正常构建 / 索引等待约5–15分钟，实际另记。无需新增蓝图业务、PIE 临时按键或本轮 Cook。初次工具建设花费较多，后续用同一入口检查；尚未测得节省率。

| 本批检查 | 明确边界 |
| --- | --- |
| ET-AI-001 | 正式 BT 有 RootNode / BlackboardAsset |
| ET-AI-002 | `PursuitStopDistance` 存在且为 Float |
| ET-AI-003 | 树内恰有一个项目追击任务，Acceptable Radius 绑定该键 |
| ET-AI-004 | 两项 Includes Radius 为常量 False，Track Moving Goal 为常量 True |
| ET-UI-001 | Widget 编译 / 控件树可用，全部原生必需 BindWidget 能找到同名控件 |
| ET-UI-002 | 找到的控件类型匹配原生属性，如 Button / TextBlock / ProgressBar |
| ET-SETUP-001 | 校验器需要的原生类 / 反射字段可用；工具自身缺依赖也明确报错 |

静态检查不证明 AI 真正移动、按钮输入、事件解绑或蓝图白线正确。Controller 的90 / 100距离关系、能力 / GE / Montage引用、左键映射、启动与 Cook 地图留 C2；Attempt02和包内短流程留 C3。当前 B 最终函数的 **Game Build 尚未更新**，合并到本批正常 Game 构建；Editor 已成功，功能正常采用你的反馈。

## 2. 文件和模块分工

先 Save All，关闭 UE；CLion 可保留。在 CLion 项目树中对目录右键 → New → Directory / File 创建以下四个文件。文件扩展名必须正确，不要生成 `.h.txt`。这是 Editor 模块，**不要使用 UE 的 Actor / Component 新建向导**。

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\
├─ EmberTrialEditor.Build.cs
├─ Public\Validation\ETProjectAssetValidator.h
└─ Private\
   ├─ EmberTrialEditorModule.cpp
   └─ Validation\ETProjectAssetValidator.cpp
```

本批还修改 `EmberTrial.uproject`、`Source\EmberTrialEditor.Target.cs`，以及旧追击 cpp 的 include。**`EmberTrial.Target.cs` 与 Core / UI 的 Build.cs 保持现有内容。**

当前实际69份Source / 34份cpp / 清单46项资产。按本指南实施四新文件后预计73 / 36；保存合法Probe并验收后再登记第47项来源清单，不提前把计划记成实际结果。

依赖方向为 Editor → Core、AIModule、UMG / UMGEditor、DataValidation；Core / UI 不引用 Editor。Validator 只读取资产，不创建角色、Widget 实例或运行世界，不自动修改配置。BT 遍历和控件检查各自独立；规则 ID 便于定位问题。将来扩大规则时先新增小函数，不把运行时玩法写进这个工具。

## 3. 四个新文件：完整内容

### 3.1 EmberTrialEditor.Build.cs

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\EmberTrialEditor.Build.cs`。

```csharp
using UnrealBuildTool;

public class EmberTrialEditor : ModuleRules
{
    public EmberTrialEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "DataValidation", "AssetRegistry"
        });
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AIModule", "UMG", "UMGEditor", "UnrealEd", "EmberTrial"
        });
    }
}
```

DataValidation 在公开依赖中，因为公开头文件继承它的类；AssetRegistry 提供该父类头文件中的 FAssetData。UnrealEd / UMGEditor 只在这个 Editor 模块中，不向 Runtime Build.cs 添加。

### 3.2 EmberTrialEditorModule.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\EmberTrialEditorModule.cpp`。

```cpp
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETEditorModule, Log, All);

class FEmberTrialEditorModule final : public FDefaultModuleImpl
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogETEditorModule, Display,
            TEXT("[M1-07C1] Editor validation module ready"));
    }
};

IMPLEMENT_MODULE(FEmberTrialEditorModule, EmberTrialEditor)
```

UE 启动时会发现已加载模块里的原生 Validator；不用手动 AddValidator，也不用做一个 Validator 蓝图。新增反射类后正常关闭 UE 构建 / 重启，不用 Live Coding 验证注册。

### 3.3 ETProjectAssetValidator.h

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Public\Validation\ETProjectAssetValidator.h`。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "ETProjectAssetValidator.generated.h"

class UBehaviorTree;
class UWidgetBlueprint;

UCLASS()
class EMBERTRIALEDITOR_API UETProjectAssetValidator : public UEditorValidatorBase
{
    GENERATED_BODY()

public:
    virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData,
        UObject* InObject, FDataValidationContext& InContext) const override;

    virtual EDataValidationResult ValidateLoadedAsset_Implementation(
        const FAssetData& InAssetData, UObject* InAsset,
        FDataValidationContext& Context) override;

private:
    void AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Reason,
        int32& IssueCount);
    void ValidateTree(UBehaviorTree* Tree, int32& IssueCount);
    void ValidateWidget(UWidgetBlueprint* Blueprint, int32& IssueCount);
};
```

`generated.h` 必须是最后一个 include。这两个 override 使用本机5.5.4的新签名；不要再添加同名 UFUNCTION，不要换成旧教程的单 UObject / ValidationErrors 参数版本。

文件名必须为 **ETProjectAssetValidator.h**（结尾是 `Validator`），与 `ETProjectAssetValidator.generated.h` 的前缀一致。若误命名为 `ETProjectAssetValidation.h`，在 CLion 项目树选中该文件 → Shift+F6 → 改为正确文件名；保留上面的 include 和类名，保存后重跑第5节。UHT报要求 `ETProjectAssetValidation.generated.h` 时，先核对文件名，不能把这条 include 移到其他 include 前面。

### 3.4 ETProjectAssetValidator.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\Validation\ETProjectAssetValidator.cpp`。

```cpp
#include "Validation/ETProjectAssetValidator.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/ValueOrBBKey.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Misc/DataValidation.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAssetValidation, Log, All);

namespace
{
    const FName ValidationStopDistanceKey(TEXT("PursuitStopDistance"));
    const FString ValidationTestRoot(TEXT("/Game/EmberTrial/Tests/Validation/"));

    UWidget* FindRequiredWidget(UWidgetBlueprint* Blueprint, FName WidgetName)
    {
        TSet<const UWidgetBlueprint*> Visited;
        for (UWidgetBlueprint* Current = Blueprint;
            IsValid(Current) && !Visited.Contains(Current);)
        {
            Visited.Add(Current);
            if (IsValid(Current->WidgetTree))
            {
                if (UWidget* Widget = Current->WidgetTree->FindWidget(WidgetName))
                {
                    return Widget;
                }
            }
            UClass* Parent = Current->ParentClass.Get();
            Current = IsValid(Parent)
                ? Cast<UWidgetBlueprint>(Parent->ClassGeneratedBy) : nullptr;
        }
        return nullptr;
    }
}

bool UETProjectAssetValidator::CanValidateAsset_Implementation(
    const FAssetData& InAssetData, UObject* InObject,
    FDataValidationContext& InContext) const
{
    const FString Package = InAssetData.PackageName.ToString();
    const bool bValidationTest = Package.StartsWith(ValidationTestRoot);

    if (InObject && InObject->IsA<UBehaviorTree>())
    {
        return Package == TEXT("/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy")
            || bValidationTest;
    }
    return InObject && InObject->IsA<UWidgetBlueprint>()
        && Package.StartsWith(TEXT("/Game/EmberTrial/"))
        && (!Package.StartsWith(TEXT("/Game/EmberTrial/Tests/")) || bValidationTest);
}

EDataValidationResult UETProjectAssetValidator::ValidateLoadedAsset_Implementation(
    const FAssetData& InAssetData, UObject* InAsset,
    FDataValidationContext& Context)
{
    int32 IssueCount = 0;
    if (UBehaviorTree* Tree = Cast<UBehaviorTree>(InAsset))
    {
        ValidateTree(Tree, IssueCount);
    }
    else if (UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(InAsset))
    {
        ValidateWidget(Blueprint, IssueCount);
    }
    else
    {
        return EDataValidationResult::NotValidated;
    }

    if (IssueCount > 0)
    {
        UE_LOG(LogETAssetValidation, Display,
            TEXT("[M1-07C1] Invalid Asset=%s Issues=%d"),
            *GetPathNameSafe(InAsset), IssueCount);
        return EDataValidationResult::Invalid;
    }
    AssetPasses(InAsset);
    UE_LOG(LogETAssetValidation, Display,
        TEXT("[M1-07C1] Valid Asset=%s"), *GetPathNameSafe(InAsset));
    return EDataValidationResult::Valid;
}

void UETProjectAssetValidator::AddIssue(UObject* Asset, const TCHAR* Rule,
    const FString& Reason, int32& IssueCount)
{
    ++IssueCount;
    AssetFails(Asset, FText::FromString(FString::Printf(
        TEXT("[%s] %s : %s"), Rule, *GetPathNameSafe(Asset), *Reason)));
}

void UETProjectAssetValidator::ValidateTree(UBehaviorTree* Tree, int32& IssueCount)
{
    if (!IsValid(Tree->RootNode) || !IsValid(Tree->BlackboardAsset))
    {
        AddIssue(Tree, TEXT("ET-AI-001"),
            TEXT("Root / Blackboard missing; restore the project tree and blackboard."),
            IssueCount);
        return;
    }

    const UBlackboardData* Board = Tree->BlackboardAsset;
    const FBlackboard::FKey KeyId = Board->GetKeyID(ValidationStopDistanceKey);
    const UClass* KeyType = Board->GetKeyType(KeyId).Get();
    if (!IsValid(KeyType) || !KeyType->IsChildOf(UBlackboardKeyType_Float::StaticClass()))
    {
        AddIssue(Tree, TEXT("ET-AI-002"),
            TEXT("PursuitStopDistance must exist as a Float blackboard key."), IssueCount);
    }

    // Resolve the loaded class by reflection; no exported project task header is required.
    const UClass* PursuitClass = FindObject<UClass>(nullptr,
        TEXT("/Script/EmberTrial.ETBTTask_PursuitMove"));
    const FBoolProperty* BoolDefault = FindFProperty<FBoolProperty>(
        FValueOrBBKey_Bool::StaticStruct(), TEXT("DefaultValue"));
    if (!IsValid(PursuitClass) || !BoolDefault)
    {
        AddIssue(Tree, TEXT("ET-SETUP-001"),
            TEXT("Native pursuit class / bool default field unavailable; check Editor module dependencies and UE version."),
            IssueCount);
        return;
    }

    const auto HasConstant = [BoolDefault](const FValueOrBBKey_Bool& Value, bool Expected)
    {
        return Value.GetKey().IsNone()
            && BoolDefault->GetPropertyValue_InContainer(&Value) == Expected;
    };

    TArray<const UBTCompositeNode*> Pending;
    TSet<const UBTCompositeNode*> Visited;
    Pending.Add(Tree->RootNode.Get());
    int32 PursuitCount = 0;
    while (!Pending.IsEmpty())
    {
        const UBTCompositeNode* Node = Pending.Pop();
        if (!IsValid(Node) || Visited.Contains(Node))
        {
            continue;
        }
        Visited.Add(Node);
        for (const FBTCompositeChild& Child : Node->Children)
        {
            if (IsValid(Child.ChildComposite))
            {
                Pending.Add(Child.ChildComposite.Get());
            }
            const UBTTask_MoveTo* Move = Cast<UBTTask_MoveTo>(Child.ChildTask.Get());
            if (!IsValid(Move) || !Move->IsA(PursuitClass))
            {
                continue;
            }
            ++PursuitCount;
            if (Move->AcceptableRadius.GetKey() != ValidationStopDistanceKey)
            {
                AddIssue(Tree, TEXT("ET-AI-003"),
                    TEXT("ET Pursuit Move To: bind Acceptable Radius to PursuitStopDistance; a constant 90 is not a binding."),
                    IssueCount);
            }
            if (!HasConstant(Move->bReachTestIncludesAgentRadius, false)
                || !HasConstant(Move->bReachTestIncludesGoalRadius, false)
                || !HasConstant(Move->bTrackMovingGoal, true))
            {
                AddIssue(Tree, TEXT("ET-AI-004"),
                    TEXT("ET Pursuit Move To: Includes Agent / Goal Radius must be constant False; Track Moving Goal constant True."),
                    IssueCount);
            }
        }
    }
    if (PursuitCount != 1)
    {
        AddIssue(Tree, TEXT("ET-AI-003"), FString::Printf(
            TEXT("Expected one ET Pursuit Move To task; found %d. Restore the project pursuit node."),
            PursuitCount), IssueCount);
    }
}

void UETProjectAssetValidator::ValidateWidget(UWidgetBlueprint* Blueprint,
    int32& IssueCount)
{
    if (!IsValid(Blueprint->GeneratedClass) || !IsValid(Blueprint->WidgetTree)
        || Blueprint->Status == BS_Error || Blueprint->Status == BS_Dirty)
    {
        AddIssue(Blueprint, TEXT("ET-UI-001"),
            TEXT("Widget class / tree not ready; Compile and Save this Widget first."),
            IssueCount);
        return;
    }
    for (TFieldIterator<FObjectPropertyBase> It(Blueprint->GeneratedClass,
        EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        const FObjectPropertyBase* Property = *It;
        if (!Property->HasMetaData(TEXT("BindWidget"))
            || Property->HasMetaData(TEXT("BindWidgetOptional"))
            || !Property->PropertyClass->IsChildOf(UWidget::StaticClass()))
        {
            continue;
        }
        UWidget* Widget = FindRequiredWidget(Blueprint, Property->GetFName());
        if (!IsValid(Widget))
        {
            AddIssue(Blueprint, TEXT("ET-UI-001"), FString::Printf(
                TEXT("Required widget '%s' missing; rename the Designer widget to match the native BindWidget property."),
                *Property->GetName()), IssueCount);
        }
        else if (!Widget->IsA(Property->PropertyClass))
        {
            AddIssue(Blueprint, TEXT("ET-UI-002"), FString::Printf(
                TEXT("Widget '%s' must be %s; actual type is %s. Replace it with the expected Designer widget type."),
                *Property->GetName(), *Property->PropertyClass->GetName(),
                *Widget->GetClass()->GetName()), IssueCount);
        }
    }
}
```

说明：Bool 的默认值在5.5.4中是 protected，工具通过反射读取，不把 `FValueOrBBKey_Bool` 强转 bool，也不实例化 BehaviorTreeComponent 去运行世界。Acceptable Radius **默认值150与绑定键并存是合法的**；实际值由黑板提供，工具不要求把默认常量改成90。原生追击类未导出跨模块 API，所以只按 `/Script/EmberTrial.ETBTTask_PursuitMove` 获取已加载类，不修改它的 .h。需要 Editor→Core 的显式依赖保证模块顺序。

Widget 检查读取设计时控件树，并向父 Widget Blueprint 查找继承控件；不查看尚未构建的 CDO BindWidget 指针。可选控件不当成必需控件。默认 Widget 编译器本身也检查 BindWidget，本工具统一输出项目规则与资产路径，便于以后批量入口复用。

## 4. 注册模块与插件

### 4.1 EmberTrial.uproject

文件：`E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`。完整替换为以下内容（只新增 Editor 模块和 Editor 专用插件项，原两模块 / 插件保留）：

```json
{
    "FileVersion": 3,
    "EngineAssociation": "5.5",
    "Category": "",
    "Description": "",
    "Modules": [
        {
            "Name": "EmberTrial",
            "Type": "Runtime",
            "LoadingPhase": "Default",
            "AdditionalDependencies": ["Engine"]
        },
        {
            "Name": "EmberTrialUI",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        },
        {
            "Name": "EmberTrialEditor",
            "Type": "Editor",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        {
            "Name": "ModelingToolsEditorMode",
            "Enabled": true,
            "TargetAllowList": ["Editor"]
        },
        {
            "Name": "GameplayAbilities",
            "Enabled": true
        },
        {
            "Name": "DataValidation",
            "Enabled": true,
            "TargetAllowList": ["Editor"]
        }
    ]
}
```

JSON 最后一项后不要加逗号，插件名是 `DataValidation`，模块名是 `EmberTrialEditor`。

### 4.2 Editor Target

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor.Target.cs`。

仅将现有 `ExtraModuleNames.AddRange(...)` 这一行改为：

```csharp
ExtraModuleNames.AddRange(new string[] { "EmberTrial", "EmberTrialUI", "EmberTrialEditor" });
```

类名、Type=Editor、V5、Unreal5_5 和其余代码保留。**不要向 `EmberTrial.Target.cs` 增加 Editor 模块**；Game Target 原两模块保持不变。`Tools\Refresh-CLion.ps1` 已遍历 .uproject 的模块，无需再改脚本。

### 4.3 合并 B 的 include 清理

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp`。

只删下面两个已无用途的 include：

```cpp
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
```

在其他 include 后加入：

```cpp
#include "GameFramework/Pawn.h"
```

最终 `OnTaskFinished` 和构造函数的先常量 / 后 SetKey **保持你刚完成的版本**。这只是显式头文件依赖清理，不改追击逻辑。

## 5. 正常构建与索引

四个新文件和三个修改文件 Save All；确认 UE 已关闭。PowerShell 按顺序执行，前一步失败先停止，发完整首个错误，不继续 Game Build：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

预期：UBT / UHT 成功，脚本打印三个模块的 Index metadata，其中新增 `EmberTrialEditor`。CLion 的 CMake 自动重载后应出现 `EmberTrialEditorCodeModel`，两 cpp 可见；必要时点 Reload CMake Project，**不要改 UE 生成的 CMake 或清空缓存**。对 Validator 的 EditorValidatorBase.h / WidgetBlueprint.h 可导航。

然后执行 Game 构建，补齐 B 最终函数并验证 Editor 依赖隔离：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

Game 构建应成功，Game 不编译 `ETProjectAssetValidator.cpp` / Editor 模块，不报“Unable to instantiate UnrealEd module for non-editor targets”。没有编译输出时可能是 Up to date，不能单靠输出少判断隔离失败；我会复核 Target / Build 记录。启动 UE 后 Output Log 搜索 `[M1-07C1] Editor validation module ready` 应有一条。Editor 构建通过不等于校验注册和规则已验收，第6–7节仍需要执行。

## 6. 正常资产检查（约2–3分钟）

### 6.1 正式 BT

1. 打开 Content Drawer / Content Browser，定位 `/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy`，选中**资产缩略图**。这里 `/Game` 对应 Content，不是文件系统路径。
2. 右键缩略图 → **Asset Actions（资产操作）→ Validate Assets（验证资产）**。选择这一项，不选带 Dependencies 的版本，也不点蓝图 Compile 代替数据校验。
3. 打开 Output Log，搜索 `[M1-07C1]`，应看到 `Valid Asset=/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy...`。这是本项目 Validator 真正执行的标记；仅看到“验证完成”而没有项目标记，不能当注册通过。
4. 如果正式 BT 报 ET-AI-003，检查所选追击节点的 Acceptable Radius 绑定键确为 PursuitStopDistance，保存后重查；不要为了过校验直接改工具的规则。ET-AI-004 则对应三项常量开关。

### 6.2 正式 Health HUD

1. 在 Content Browser 定位 `/Game/EmberTrial/ui/HUD/WBP_ETHealthHUD`。打开它，Compile → Save，保留当前四个控件和原生父类。
2. 回资产缩略图，右键 → Asset Actions → Validate Assets。Output Log 应有该 HUD 路径的 `[M1-07C1] Valid`。
3. 不需要改控件名字制造错误，也不需要重跑 HUD 旧测试矩阵。若失败，先按规则给出的控件名称 / 类型局部核查。不要把 Native Tick、UI焦点等运行问题当成工具已覆盖的项目。

消息有错误时 UE 通常打开 Message Log 的 **Data Validation** 页；也可用 Window → Developer Tools → Message Log 查结果。若菜单翻译不同，以 Asset Actions 子菜单内含“Validate Assets”的项和 Output Log 标记为准。本机菜单注册位置及入口已核对；插件项缺失先看模块启动 / Edit → Plugins → Data Validation，别重复创建 Validator 蓝图。

## 7. 一份隔离 BT：失败与恢复（约3–5分钟）

目标是证明规则真的能捕获**这次发生过的键绑定回归**。不更改正式 BT，不给 Controller 换树，不运行 Play。

1. Content Browser 创建 `/Game/EmberTrial/Tests/Validation` 文件夹。对正式 `BT_ETMeleeEnemy` 右键 → Duplicate（复制），在名称框输入 **`BT_ETValidation_Probe`** 并按 Enter。将**副本缩略图**拖到左侧的 Validation 文件夹，弹出选项时选 **Move Here**。最终资源路径必须为 `/Game/EmberTrial/Tests/Validation/BT_ETValidation_Probe`。移动可能在原 AI/BehaviorTrees 留下同名 Redirector；对原文件夹右键 → **Fix Up Redirectors in Folder** 清理副本的旧路径，随后 Save All。正式 BT 名称 / 路径保持原值。
2. 打开副本，选中图内 **ET Pursuit Move To** 任务节点；右侧 Details 找 **Acceptable Radius**。使用你在 B 中已操作过的黑板绑定选择器，将键从 **PursuitStopDistance** 改为 **None / 不绑定**。常量保留150；其他节点 / Blackboard 引用和开关不改。
3. Save 副本。若当前编辑器开启保存时自动校验，这一步已可能报错，这是预期负例。回 Content Browser 对副本执行 Validate Assets，错误应包含：

   ```text
   [ET-AI-003] /Game/EmberTrial/Tests/Validation/BT_ETValidation_Probe...
   ET Pursuit Move To: bind Acceptable Radius to PursuitStopDistance...
   ```

   同时 Output Log 有该副本的 `[M1-07C1] Invalid ... Issues=1`。已有日志可能因保存 / 手动校验各出现一次，不按总行数判断去重；看最近一次校验。如果变成 ET-SETUP-001 或没有项目标记，先发日志，不能当预期失败通过。
4. 回副本同一节点，把 Acceptable Radius 的绑定键恢复 **PursuitStopDistance**，Save。再对副本执行 Validate Assets，应变为 `[M1-07C1] Valid`，本次没有新的 ET-AI 错误。旧 Message Log 页中的历史错误仍可能保留，按新一轮 / 时间区分。
5. 这份**已恢复为合法配置**的 Probe 保留，供以后复用。它不放地图、不设为 Controller 的树、不加 Cook 地图列表；不要留下故意错误的最终资产，也不要修改它引用的共享黑板。

本批只实测 ET-AI-003 的负例，其余规则检查实现和正常资产；不声称每条规则都已有独立错误样例。未来修改某条规则或出现相关失败时，再增加对应最小负例。既有 Tests 下的非法健康配置示例不在本工具范围内，不阻止正常批次开发。

## 8. 完成边界与日常用法

只需第5节双构建、第6节两个正常资产、第7节副本失败 / 恢复。**不要求本批重跑 PIE、旧战斗 / UI矩阵或 Insights**；模块只增加编辑器静态检查，B的行为正常已有反馈。正常检查目标5–10分钟，遇到失败只针对该项排查。

以后改正式 BT / 项目 Widget 后，Compile（如适用）→ Save → Validate Assets。多选资源可一次验证；是否运行全项目 / 命令行检查留 C2 设计，不默认每次扫描整个模板 / 第三方资源。静态通过之后，真正改变运行逻辑时仍执行对应窄实机检查。

本批已核对本机 `EditorValidatorBase.h`、`ValueOrBBKey.h`、`BTTask_MoveTo.h`、`BaseWidgetBlueprint.h` 与菜单注册源码；代码尚未由本轮助手写入 Source 或实际编译。UE 官方入口和原生注册说明见 [Data Validation，UE5.5](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine?application_version=5.5)；方法签名以本机头文件为准。

## 9. 最简反馈

```text
M1-07C1 完成 / 卡在第__节第__步
四新文件 / uproject / Editor Target / B include清理，已保存：
Editor Build / 三模块索引 / Game Build：
Editor模块启动一条；正式BT与HealthHUD均有本项目Valid标记：
隔离Probe None绑定→ET-AI-003，恢复绑定→Valid：
Probe已恢复 / 未用于正式Controller；源码与资产Save All：
失败或特别说明（无则填无）：
完成时分 / 实际手动用时 / 构建等待：
```

编译问题优先发送**从首个实际 error 起的文字日志**、文件 / 行号和刚改的内容；不要只发脚本最后的 ExitCode6。Details / 资产配置看不清则发截到资产名、选中节点和右侧字段的截图。已有这份 Probe，后续无需重新设计验收图。

## 10. 本批需要 Commit / Push（固定放在文档最后）

- 新增四份 Source：`Source/EmberTrialEditor/EmberTrialEditor.Build.cs`、`Private/EmberTrialEditorModule.cpp`、`Public/Validation/ETProjectAssetValidator.h`、`Private/Validation/ETProjectAssetValidator.cpp`，均位于 `EmberTrial` 工程下。
- 修改 `EmberTrial/EmberTrial.uproject`、`EmberTrial/Source/EmberTrialEditor.Target.cs`；重新 Stage 当前 `EmberTrial/Source/EmberTrial/Private/AI/ETBTTask_PursuitMove.cpp`，包含 B 最终短函数 / 正确 SetKey 顺序和本批 include 清理。B代码如已经提交，则本次只提交真实新增差异。
- 新增**恢复为合法配置**的 `EmberTrial/Content/EmberTrial/Tests/Validation/BT_ETValidation_Probe.uasset`（LFS）。正式 BT / HealthHUD 若只读取没有内容差异，不人为 Stage；既有测试资产不改。
- 新增本指南；M1-07路线、B报告、Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow_Options / Introduction 的实际更新，以及验收后更新的 `Docs/AssetManifest.csv` 和本批验收报告。后续报告尚未生成，不提前声明已存在或通过。
- `Tools/Refresh-CLion.ps1` 本批无需修改；CMake、Intermediate / Binaries / Saved、`.idea`、Raw、Artifacts里的Trace / 日志 / JSON / 包继续留本地，不 Force Add。Game Build不等于新版包，Attempt02留 C3。

CLion：Save All → 刷新 Changes → 对**最新** Unstaged 差异重新 Stage → 检查 Staged Diff → Commit → Push。建议实现提交 `feat: add editor validation for pursuit and required widgets`；B收尾未提交时也纳入最新清单。由你操作 Git，助手不代 Stage / Commit / Push。
