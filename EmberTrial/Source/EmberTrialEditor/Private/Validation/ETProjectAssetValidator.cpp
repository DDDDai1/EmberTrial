#include "Validation/ETProjectAssetValidator.h"
#include "AI/ETBlackboardKeys.h"

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
const FString ValidationTestRoot(TEXT("/Game/EmberTrial/Tests/Validation/"));

// 沿设计时父蓝图查找继承控件；Visited防止异常继承链重复遍历。
UWidget* FindRequiredWidget(UWidgetBlueprint* Blueprint, FName WidgetName)
{
    TSet<const UWidgetBlueprint*> Visited;
    for (UWidgetBlueprint* Current = Blueprint; IsValid(Current) && !Visited.Contains(Current);)
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
        Current = IsValid(Parent) ? Cast<UWidgetBlueprint>(Parent->ClassGeneratedBy) : nullptr;
    }
    return nullptr;
}
} // namespace

// ---- 范围与结果：正式资源及专用Validation副本 ----
bool UETProjectAssetValidator::CanValidateAsset_Implementation(
    const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
    // 其他Tests（包括故意非法身体）不纳入；不能把范围扩成整个/Game。
    const FString Package = InAssetData.PackageName.ToString();
    const bool bValidationTest = Package.StartsWith(ValidationTestRoot);

    if (InObject && InObject->IsA<UBehaviorTree>())
    {
        return Package == TEXT("/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy") ||
               bValidationTest;
    }
    return InObject && InObject->IsA<UWidgetBlueprint>() &&
           Package.StartsWith(TEXT("/Game/EmberTrial/")) &&
           (!Package.StartsWith(TEXT("/Game/EmberTrial/Tests/")) || bValidationTest);
}

EDataValidationResult UETProjectAssetValidator::ValidateLoadedAsset_Implementation(
    const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    // 每次资产独立计数；不支持的类型返回NotValidated。
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
        UE_LOG(LogETAssetValidation, Display, TEXT("[M1-07C1] Invalid Asset=%s Issues=%d"),
               *GetPathNameSafe(InAsset), IssueCount);
        return EDataValidationResult::Invalid;
    }
    AssetPasses(InAsset);
    UE_LOG(LogETAssetValidation, Display, TEXT("[M1-07C1] Valid Asset=%s"),
           *GetPathNameSafe(InAsset));
    return EDataValidationResult::Valid;
}

// ---- 规则诊断：保留规则ID、资产路径与修复说明 ----
void UETProjectAssetValidator::AddIssue(UObject* Asset, const TCHAR* Rule, const FString& Reason,
                                        int32& IssueCount)
{
    ++IssueCount;
    AssetFails(Asset, FText::FromString(FString::Printf(TEXT("[%s] %s : %s"), Rule,
                                                        *GetPathNameSafe(Asset), *Reason)));
}

// ---- BT：黑板类型、追击任务数量与动态键绑定 ----
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
    const FBlackboard::FKey KeyId = Board->GetKeyID(ETBlackboardKeys::PursuitStopDistance);
    const UClass* KeyType = Board->GetKeyType(KeyId).Get();
    if (!IsValid(KeyType) || !KeyType->IsChildOf(UBlackboardKeyType_Float::StaticClass()))
    {
        AddIssue(Tree, TEXT("ET-AI-002"),
                 TEXT("PursuitStopDistance must exist as a Float blackboard key."), IssueCount);
    }

    // Resolve the loaded class by reflection; no exported project task header is required.
    const UClass* PursuitClass =
        FindObject<UClass>(nullptr, TEXT("/Script/EmberTrial.ETBTTask_PursuitMove"));
    const FBoolProperty* BoolDefault =
        FindFProperty<FBoolProperty>(FValueOrBBKey_Bool::StaticStruct(), TEXT("DefaultValue"));
    if (!IsValid(PursuitClass) || !BoolDefault)
    {
        AddIssue(Tree, TEXT("ET-SETUP-001"),
                 TEXT("Native pursuit class / bool default field unavailable; check Editor module "
                      "dependencies and UE version."),
                 IssueCount);
        return;
    }

    // 布尔常量必须未绑定黑板键，并具有要求的DefaultValue；同值的键绑定也不合格。
    const auto HasConstant = [BoolDefault](const FValueOrBBKey_Bool& Value, bool Expected)
    {
        return Value.GetKey().IsNone() &&
               BoolDefault->GetPropertyValue_InContainer(&Value) == Expected;
    };

    // 显式栈遍历复合节点；Visited保证异常共享 / 环形引用也能结束。
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
            // 数值恰好为90不能代替GetKey绑定，否则运行时距离更新无法生效。
            if (Move->AcceptableRadius.GetKey() != ETBlackboardKeys::PursuitStopDistance)
            {
                AddIssue(Tree, TEXT("ET-AI-003"),
                         TEXT("ET Pursuit Move To: bind Acceptable Radius to PursuitStopDistance; "
                              "a constant 90 is not a binding."),
                         IssueCount);
            }
            if (!HasConstant(Move->bReachTestIncludesAgentRadius, false) ||
                !HasConstant(Move->bReachTestIncludesGoalRadius, false) ||
                !HasConstant(Move->bTrackMovingGoal, true))
            {
                AddIssue(Tree, TEXT("ET-AI-004"),
                         TEXT("ET Pursuit Move To: Includes Agent / Goal Radius must be constant "
                              "False; Track Moving Goal constant True."),
                         IssueCount);
            }
        }
    }
    if (PursuitCount != 1)
    {
        AddIssue(Tree, TEXT("ET-AI-003"),
                 FString::Printf(TEXT("Expected one ET Pursuit Move To task; found %d. Restore the "
                                      "project pursuit node."),
                                 PursuitCount),
                 IssueCount);
    }
}

// ---- UMG：原生必需属性与设计树中的名称 / 类型 ----
void UETProjectAssetValidator::ValidateWidget(UWidgetBlueprint* Blueprint, int32& IssueCount)
{
    if (!IsValid(Blueprint->GeneratedClass) || !IsValid(Blueprint->WidgetTree) ||
        Blueprint->Status == BS_Error || Blueprint->Status == BS_Dirty)
    {
        AddIssue(Blueprint, TEXT("ET-UI-001"),
                 TEXT("Widget class / tree not ready; Compile and Save this Widget first."),
                 IssueCount);
        return;
    }
    // 包含原生父类属性，只检查必需BindWidget；Optional不要求存在。
    // 通过表示结构匹配，不证明焦点、输入或运行时事件解绑正确。
    for (TFieldIterator<FObjectPropertyBase> It(Blueprint->GeneratedClass,
                                                EFieldIteratorFlags::IncludeSuper);
         It; ++It)
    {
        const FObjectPropertyBase* Property = *It;
        if (!Property->HasMetaData(TEXT("BindWidget")) ||
            Property->HasMetaData(TEXT("BindWidgetOptional")) ||
            !Property->PropertyClass->IsChildOf(UWidget::StaticClass()))
        {
            continue;
        }
        UWidget* Widget = FindRequiredWidget(Blueprint, Property->GetFName());
        if (!IsValid(Widget))
        {
            AddIssue(Blueprint, TEXT("ET-UI-001"),
                     FString::Printf(TEXT("Required widget '%s' missing; rename the Designer "
                                          "widget to match the native BindWidget property."),
                                     *Property->GetName()),
                     IssueCount);
        }
        else if (!Widget->IsA(Property->PropertyClass))
        {
            AddIssue(Blueprint, TEXT("ET-UI-002"),
                     FString::Printf(TEXT("Widget '%s' must be %s; actual type is %s. Replace it "
                                          "with the expected Designer widget type."),
                                     *Property->GetName(), *Property->PropertyClass->GetName(),
                                     *Widget->GetClass()->GetName()),
                     IssueCount);
        }
    }
}
