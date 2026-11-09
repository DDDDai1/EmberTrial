# M3-06C1 500条遗物数据投影压力验证

2026-11-08下发，UE 5.5.4。预计手动25–40分钟，构建等待另计。本节涉及两个既有头文件与一份既有测试。候选测试cpp已通过编译预检，正式构建与运行按第3–5节执行。[B4-03验收报告](Reports/M3_06B4_03_Verification.md)

## 1. 验证范围

在`EmberTrial.UI.Relics.ViewModels`原专项内创建500件临时定义和一份临时快照。用交错设计顺序、双槽资格与中文名，验证真实投影函数的排序、搜索、筛选和异常清空。重复30次交替投影全列表与单条搜索，只计`BuildRows`的耗时。正式目录继续使用六件。

本批的耗时口径为数据投影，C2再验证实际ListView创建与回收的Widget数量、旧绑定及滚动帧表现。六件版打包集中到后续集成。

## 2. 应用源码增量

Stop PIE，Save All并关闭UE。CLion可保留。按本节顺序替换。

### 2.1 ETRelicDefinition.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicDefinition.h`

**修改1　仅授权既有专项构造临时目录**

找到以下片段。

```cpp
    friend class FETRelicEquipmentTransactionTest;
```

替换为

```cpp
    friend class FETRelicEquipmentTransactionTest;
    friend class FETRelicViewModelsTest;
```

### 2.2 ETRelicInventorySubsystem.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicInventorySubsystem.h`

**修改1　仅授权既有专项构造临时目录**

找到以下片段。

```cpp
    friend class FETRelicEquipmentTransactionTest;
```

替换为

```cpp
    friend class FETRelicEquipmentTransactionTest;
    friend class FETRelicViewModelsTest;
```

### 2.3 ETRelicViewModelsTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Tests/ETRelicViewModelsTests.cpp`

**修改1　引用定义与CPU计时接口**

找到以下片段。

```cpp
#include "Inventory/ETRelicViewModels.h"
```

替换为

```cpp
#include "Inventory/ETRelicViewModels.h"
#include "Inventory/ETRelicDefinition.h"
#include "HAL/PlatformTime.h"
```

**修改2　在原专项末尾接入500条临时数据**

找到以下片段。

```cpp
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}
```

替换为

```cpp
    // ---- 500条合成数据只进入本测试GI的定义表；真实拥有列表与装备不参与改写 ----
    constexpr int32 StressCount = 500;
    const auto StressId = [](const int32 Index)
    {
        return FName(*FString::Printf(TEXT("stress_relic_r%03d"), Index));
    };
    FETRelicInventorySnapshot Stress = DomainBefore;
    Stress.OwnedIds.Reset(StressCount);
    Stress.Equipped.SlotA = StressId(0);
    Stress.Equipped.SlotB = StressId(499);
    const FSoftObjectPath IconPath(TEXT("/Game/Tests/T_UnloadedStressIcon.T_UnloadedStressIcon"));
    for (int32 Index = StressCount - 1; Index >= 0; --Index)
    {
        const FName Id = StressId(Index);
        if (Inventory->Definitions.Contains(Id))
        {
            AddError(FString::Printf(TEXT("Synthetic relic ID already exists: %s"), *Id.ToString()));
            return false;
        }
        UETRelicDefinition* Definition = NewObject<UETRelicDefinition>(Inventory);
        Definition->RelicId = Id;
        Definition->DisplayName = FText::FromString(FString::Printf(TEXT("压测遗物 %03d"), Index));
        Definition->Description = FText::FromString(TEXT("临时列表压力样本"));
        Definition->SortOrder = Index % 5;
        Definition->AllowedSlots = {Index % 2 == 0 ? EETRelicSlot::First : EETRelicSlot::Second};
        Definition->Modifiers = {{EETRelicModifierKind::MaxHealth, 1.0f}};
        Definition->Icon = TSoftObjectPtr<UTexture2D>(IconPath);
        Inventory->Definitions.Add(Id, Definition);
        Stress.OwnedIds.Add(Id);
    }

    Query = {};
    if (!TestTrue(TEXT("Project 500 synthetic rows"),
                  ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success) ||
        !TestEqual(TEXT("Unfiltered stress count"), Rows.Num(), StressCount))
    {
        return false;
    }
    TestTrue(TEXT("Sort order ties break by stable ID"),
             Rows[0].RelicId == StressId(0) && Rows[1].RelicId == StressId(5) &&
             Rows.Last().RelicId == StressId(499));
    TestTrue(TEXT("Stress icons keep the soft path and stay unloaded"),
             Rows[0].Icon.ToSoftObjectPath() == IconPath && Rows[0].Icon.IsPending());

    Query.Sort = EETRelicListSort::StableId;
    TestTrue(TEXT("Stable sort ignores design-order groups"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == StressCount && Rows[1].RelicId == StressId(1) &&
             Rows.Last().RelicId == StressId(499));
    Query.SearchText = TEXT("  STRESS_RELIC_R499  ");
    TestTrue(TEXT("Trimmed ID search on 500 rows"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 1 && Rows[0].RelicId == StressId(499));
    Query.SearchText = TEXT("压测遗物 042");
    TestTrue(TEXT("Chinese display-name search on 500 rows"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 1 && Rows[0].RelicId == StressId(42));

    Query.SearchText.Reset();
    Query.Filter = EETRelicListFilter::CompatibleWithSlotA;
    TestTrue(TEXT("Slot A returns exactly the even sample set"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 250 && Rows[0].RelicId == StressId(0) &&
             Rows.Last().RelicId == StressId(498));
    Query.Filter = EETRelicListFilter::CompatibleWithSlotB;
    TestTrue(TEXT("Slot B returns exactly the odd sample set"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 250 && Rows[0].RelicId == StressId(1) &&
             Rows.Last().RelicId == StressId(499));
    Query.Filter = EETRelicListFilter::Equipped;
    TestTrue(TEXT("Equipped filter preserves both slot badges"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 2 && Rows[0].RelicId == StressId(0) && Rows[0].bEquippedInSlotA &&
             Rows[1].RelicId == StressId(499) && Rows[1].bEquippedInSlotB);

    Query = {};
    Query.SearchText = TEXT("stress_relic_r499");
    FETRelicInventorySnapshot Broken = Stress;
    Broken.OwnedIds.Last() = FName(TEXT("missing_stress_definition"));
    TestTrue(TEXT("Search does not hide a missing definition in the 500-row sample"),
             ETRelicPresentation::BuildRows(Broken, Inventory, Query, Rows) ==
                 EResult::InvalidCatalog && Rows.IsEmpty());

    // 热路径取样只量BuildRows，不含构造500个定义、断言或Slate。
    constexpr int32 SampleCount = 30;
    double TotalMs = 0.0;
    double MaxMs = 0.0;
    for (int32 Index = 0; Index < SampleCount; ++Index)
    {
        const bool bSearch = Index % 2 != 0;
        Query.SearchText = bSearch ? TEXT("stress_relic_r499") : TEXT("");
        const double Start = FPlatformTime::Seconds();
        const EResult Result = ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows);
        const double ElapsedMs = (FPlatformTime::Seconds() - Start) * 1000.0;
        if (!TestTrue(TEXT("Repeated projection returns the expected count"),
                      Result == EResult::Success && Rows.Num() == (bSearch ? 1 : StressCount)))
        {
            return false;
        }
        TotalMs += ElapsedMs;
        MaxMs = FMath::Max(MaxMs, ElapsedMs);
    }
    AddInfo(FString::Printf(TEXT("[M3-06C1] Rows=%d Samples=%d MeanMs=%.3f MaxMs=%.3f"),
                           StressCount, SampleCount, TotalMs / SampleCount, MaxMs));
    const FETRelicInventorySnapshot DomainAfterStress = Inventory->GetSnapshot();
    TestTrue(TEXT("Stress leaves session inventory untouched"),
             DomainAfterStress.OwnedIds == DomainBefore.OwnedIds &&
             DomainAfterStress.Equipped == DomainBefore.Equipped &&
             DomainAfterStress.Revision == DomainBefore.Revision);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}
```

## 3. 正常构建

保存三份源码。关闭UE后在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本完成Editor Build与三模块索引。CLion有提示时Reload CMake。再执行Game Development Build。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 4. 专项与耗时

打开UE，在非Play状态下于Output Log底部运行原专项。

```text
Automation RunTests EmberTrial.UI.Relics.ViewModels
```

等到`Result={Success}`，再搜索`[M3-06C1]`，复制含`Rows=500 Samples=30 MeanMs=… MaxMs=…`的一行。均值和最大值覆盖全列表及单条搜索各15次。记录本次实测值，耗时用于后续比较。

通过条件为500条可投影、稳定排序与两类搜索正确，槽A／B各250条、已装备筛选2条、损坏定义导致失败并清空行，以及正式会话拥有ID、槽位与Revision保持原值。

## 5. 实机短查

1. Play正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。按P，查看只读列表仍是六件，切换一次排序并选中裂势印。
2. 关闭页面，确认移动正常，再Stop。反馈双Build／三索引、专项Success、`[M3-06C1]`计时行、正式六件与P关闭、实际手动工时与构建等待。

## C1验收结果

2026-11-08通过。双Build、三索引、ViewModels Success、正式六件与P关闭已确认。500条／30次投影平均0.241 ms、最大0.327 ms。手动30分钟，构建等待5分钟。本地实现为`2d98975`。[验收报告](Reports/M3_06C1_Verification.md) · [C2操作指南](M3_06C2_RelicListViewStress.md)

## 6. Commit / Push

B4-03实现已在本地提交`bce7dac`。在CLion Review并Stage本轮12份文档及来源清单，再Commit和Push。

```text
Docs/Reports/M3_06B4_03_Verification.md
Docs/M3_06B4_03_ShatterSealAndComparison.md
Docs/M3_06C1_RelicProjectionStress.md
Docs/AssetManifest.csv
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文 `docs: verify six relics and plan 500-row projection stress`
- 中文 `docs: 完成六件遗物验收并规划500条数据投影压测`

C1操作通过后，功能提交包含以下三份既有源码。

```text
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicDefinition.h
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicInventorySubsystem.h
EmberTrial/Source/EmberTrialUI/Tests/ETRelicViewModelsTests.cpp
```

- 英文 `test: verify 500-row relic data projection`
- 中文 `test: 验证500条遗物数据投影`
