# M3-05B3-01：显示设置草稿与候选目录

2026-11-05，UE **5.5.4**。[B2-02已验收](Reports/M3_05B2_02_Verification.md)，无待补。本章为手动实施指南；完整代码只在文档与忽略候选中，正式Source尚未写入，未执行本章Build或自动化。

## 1. 本章效果与分工

显示页先在本地选择模式 / 尺寸，按“预览”才交给B2原生服务；改下拉框只改草稿。本批完成当前实际模式基线、目录去重 / 排序、模式往返保留各自选择、无边框固定桌面、无变化或外部环境变化时拒绝旧候选。页面关闭丢弃草稿，取消或事务结束后重读实际状态。

**本批不会出现新的显示设置卡片。** B3-02接共享Widget、模式 / 分辨率下拉框与确认 / 回退交互，再挂主菜单和原Pause页。Serial、阶段、计时和配置保护始终由B2拥有；本类不重复实现事务。后续Widget关闭须用自己的Owner / Serial显式Cancel并解绑，预览繁忙期间禁用编辑，事务结束重新建立草稿。PIE保持真实切屏拒绝，页面届时显示独立游戏操作提示。

实际209 Source / 118cpp（Core83 / UI32 / Editor3）、36测试、300 Content / 来源81；本批三新UI后预计212 Source / 120cpp（Core83 / UI34 / Editor3）、37测试，资产数量保持。预算代码45–65 / 阅读5–10 / 检查2–5分钟，构建等待另记。

## 2. 创建三份普通文件

Save All、停止PIE并关闭UE，CLion保留。在已有UI模块手动创建以下普通文件，**不使用UE类向导，不写UCLASS / GENERATED_BODY或generated.h**：

```text
EmberTrial/Source/EmberTrialUI/Public/Settings/ETVideoSettingsDraft.h
EmberTrial/Source/EmberTrialUI/Private/Settings/ETVideoSettingsDraft.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETVideoSettingsDraftTests.cpp
```

路径相对`E:UE_ProjectDemo`。Tests必须在EmberTrialUI模块下，不能放到Core后再增加Core→UI反向依赖。原UI Build.cs已公开依赖EmberTrial，本批不用修改依赖、共享Config或资产。

## 3. 完整源码

### 3.1 Public/Settings/ETVideoSettingsDraft.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Settings/ETVideoModeRules.h"

/** 平台适配提供的目录值快照；尺寸列表不表示OS已经应用成功。 */
struct EMBERTRIALUI_API FETVideoResolutionCatalogue
{
    FIntPoint Desktop = FIntPoint::ZeroValue;
    TArray<FIntPoint> Windowed;
    TArray<FIntPoint> Fullscreen;
};

/** 页面选择草稿；不持有UObject，不切屏、保存或管理事务计时。 */
class EMBERTRIALUI_API FETVideoSettingsDraft
{
public:
    bool Open(const FETVideoMode& Actual, const FETVideoResolutionCatalogue& Available);
    void Close();

    bool SelectMode(EETVideoWindowMode Mode);
    bool SelectResolution(FIntPoint Resolution);
    void GetModes(TArray<EETVideoWindowMode>& Out) const;
    void GetResolutions(EETVideoWindowMode Mode, TArray<FIntPoint>& Out) const;

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const { return bOpen && !(Value == Baseline); }
    const FETVideoMode& GetValue() const { return Value; }

    // 发预览前重读实际模式 / 平台目录；外部变化时拒绝旧草稿。
    bool TryMakePreview(const FETVideoMode& CurrentActual,
        const FETVideoResolutionCatalogue& CurrentCatalogue, FETVideoMode& Out) const;

private:
    FETVideoMode Baseline;
    FETVideoMode Value;
    FETVideoResolutionCatalogue Catalogue;
    FIntPoint WindowedChoice = FIntPoint::ZeroValue;
    FIntPoint FullscreenChoice = FIntPoint::ZeroValue;
    bool bOpen = false;
};
```

### 3.2 Private/Settings/ETVideoSettingsDraft.cpp

```cpp
#include "Settings/ETVideoSettingsDraft.h"

namespace ETVideoDraftPrivate
{
void Normalize(TArray<FIntPoint>& Sizes)
{
    Sizes.RemoveAll([](const FIntPoint Size) { return Size.X <= 0 || Size.Y <= 0; });
    Sizes.Sort([](const FIntPoint& A, const FIntPoint& B)
    {
        return A.X == B.X ? A.Y < B.Y : A.X < B.X;
    });
    for (int32 Index = Sizes.Num() - 1; Index > 0; --Index)
    {
        if (Sizes[Index] == Sizes[Index - 1])
        {
            Sizes.RemoveAt(Index);
        }
    }
}
} // namespace ETVideoDraftPrivate

// ---- 页面会话：从实际状态创建副本，关闭丢弃所有选择 ----

bool FETVideoSettingsDraft::Open(
    const FETVideoMode& Actual, const FETVideoResolutionCatalogue& Available)
{
    const FETVideoMode ActualCopy = Actual; // 允许Open(GetValue(), ...)重新建立快照。
    const FETVideoResolutionCatalogue CatalogueCopy = Available;
    Close();
    if (!ActualCopy.IsWellFormed() || CatalogueCopy.Desktop.X <= 0 || CatalogueCopy.Desktop.Y <= 0 ||
        (ActualCopy.Mode == EETVideoWindowMode::Borderless && ActualCopy.Resolution != CatalogueCopy.Desktop))
    {
        return false;
    }
    Catalogue = CatalogueCopy;
    // 当前实际尺寸应可见，即使来自启动参数而不在平台推荐目录中。
    if (ActualCopy.Mode == EETVideoWindowMode::Windowed)
    {
        Catalogue.Windowed.AddUnique(ActualCopy.Resolution);
        WindowedChoice = ActualCopy.Resolution;
    }
    else if (ActualCopy.Mode == EETVideoWindowMode::Fullscreen)
    {
        Catalogue.Fullscreen.AddUnique(ActualCopy.Resolution);
        FullscreenChoice = ActualCopy.Resolution;
    }
    ETVideoDraftPrivate::Normalize(Catalogue.Windowed);
    ETVideoDraftPrivate::Normalize(Catalogue.Fullscreen);
    Baseline = Value = ActualCopy;
    bOpen = true;
    return true;
}

void FETVideoSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETVideoMode{};
    Catalogue = FETVideoResolutionCatalogue{};
    WindowedChoice = FullscreenChoice = FIntPoint::ZeroValue;
}

// ---- 目录与编辑：模式往返保留各自草稿；无边框只有桌面尺寸 ----

void FETVideoSettingsDraft::GetResolutions(
    const EETVideoWindowMode Mode, TArray<FIntPoint>& Out) const
{
    Out.Reset();
    if (!bOpen)
    {
        return;
    }
    switch (Mode)
    {
    case EETVideoWindowMode::Windowed: Out = Catalogue.Windowed; break;
    case EETVideoWindowMode::Fullscreen: Out = Catalogue.Fullscreen; break;
    case EETVideoWindowMode::Borderless: Out.Add(Catalogue.Desktop); break;
    default: break;
    }
}

void FETVideoSettingsDraft::GetModes(TArray<EETVideoWindowMode>& Out) const
{
    Out.Reset();
    if (!bOpen)
    {
        return;
    }
    if (!Catalogue.Windowed.IsEmpty()) Out.Add(EETVideoWindowMode::Windowed);
    Out.Add(EETVideoWindowMode::Borderless);
    if (!Catalogue.Fullscreen.IsEmpty()) Out.Add(EETVideoWindowMode::Fullscreen);
}

bool FETVideoSettingsDraft::SelectMode(const EETVideoWindowMode Mode)
{
    TArray<FIntPoint> Choices;
    GetResolutions(Mode, Choices);
    if (Choices.IsEmpty()) return false;
    if (Value.Mode == Mode) return true;

    const FIntPoint Remembered = Mode == EETVideoWindowMode::Windowed ? WindowedChoice : FullscreenChoice;
    Value.Mode = Mode;
    // 首次进入某模式只在草稿选目录第一项，不虚构通用默认分辨率。
    Value.Resolution = Choices.Contains(Remembered) ? Remembered : Choices[0];
    return SelectResolution(Value.Resolution);
}

bool FETVideoSettingsDraft::SelectResolution(const FIntPoint Resolution)
{
    TArray<FIntPoint> Choices;
    GetResolutions(Value.Mode, Choices);
    if (!Choices.Contains(Resolution)) return false;
    Value.Resolution = Resolution;
    if (Value.Mode == EETVideoWindowMode::Windowed) WindowedChoice = Resolution;
    if (Value.Mode == EETVideoWindowMode::Fullscreen) FullscreenChoice = Resolution;
    return true;
}

// ---- 生成意图：Core服务仍须重验Owner / World / 真实窗口 ----

bool FETVideoSettingsDraft::TryMakePreview(const FETVideoMode& CurrentActual,
    const FETVideoResolutionCatalogue& CurrentCatalogue, FETVideoMode& Out) const
{
    const FETVideoMode ActualCopy = CurrentActual; // Out可与调用方CurrentActual使用同一变量。
    Out = FETVideoMode{};
    if (!IsDirty() || !(ActualCopy == Baseline)) return false;

    FETVideoSettingsDraft Fresh;
    if (!Fresh.Open(ActualCopy, CurrentCatalogue)) return false;
    TArray<FIntPoint> Allowed;
    Fresh.GetResolutions(Value.Mode, Allowed);
    FETVideoMode Resolved;
    if (!FETVideoModeRules::TryResolve(Value, CurrentCatalogue.Desktop, Allowed, Resolved) ||
        !(Resolved == Value))
    {
        return false; // 环境变化不能悄悄替换用户选择的尺寸。
    }
    Out = Resolved;
    return true;
}
```

### 3.3 Tests/ETVideoSettingsDraftTests.cpp

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETVideoSettingsDraft.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETVideoSettingsDraftTest,
    "EmberTrial.UI.Settings.VideoDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETVideoSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETVideoSettingsDraft Draft;
    const FETVideoMode Actual{FIntPoint(1365, 767), EETVideoWindowMode::Windowed};
    FETVideoResolutionCatalogue Catalogue;
    Catalogue.Desktop = FIntPoint(2560, 1440);
    Catalogue.Windowed = {FIntPoint(1600, 900), FIntPoint::ZeroValue,
        FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(-1, 720)};
    Catalogue.Fullscreen = {FIntPoint(1920, 1080)};
    FETVideoMode Candidate = Actual;
    TestFalse(TEXT("Closed draft cannot edit"), Draft.SelectMode(EETVideoWindowMode::Windowed));
    TestFalse(TEXT("Closed draft cannot preview"), Draft.TryMakePreview(Actual, Catalogue, Candidate));
    TestFalse(TEXT("Rejected output cleared"), Candidate.IsWellFormed());

    TestTrue(TEXT("Open actual snapshot"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Initial view is clean"), Draft.IsDirty());
    TestFalse(TEXT("Unchanged view cannot preview"), Draft.TryMakePreview(Actual, Catalogue, Candidate));
    TArray<FIntPoint> Choices;
    Draft.GetResolutions(EETVideoWindowMode::Windowed, Choices);
    TestEqual(TEXT("Deduplicated catalogue includes actual"), Choices.Num(), 3);
    TestTrue(TEXT("Current size visible outside recommendations"), Choices.Contains(Actual.Resolution));
    TestTrue(TEXT("Choices sorted deterministically"), !Choices.IsEmpty() && Choices[0] == FIntPoint(1280, 720));
    TestEqual(TEXT("Input catalogue untouched"), Catalogue.Windowed.Num(), 5);

    TestTrue(TEXT("Edit windowed size"), Draft.SelectResolution(FIntPoint(1600, 900)));
    TestTrue(TEXT("Candidate from current baseline"), Draft.TryMakePreview(Actual, Catalogue, Candidate));
    TestTrue(TEXT("Candidate preserves intended size"), Candidate.Resolution == FIntPoint(1600, 900));
    TestTrue(TEXT("Editing leaves actual snapshot unchanged"), Actual.Resolution == FIntPoint(1365, 767));
    TestFalse(TEXT("Unlisted edit rejected"), Draft.SelectResolution(FIntPoint(1111, 777)));
    TestTrue(TEXT("Rejected edit retains choice"), Draft.GetValue() == Candidate);
    TestTrue(TEXT("Borderless selectable"), Draft.SelectMode(EETVideoWindowMode::Borderless));
    TestTrue(TEXT("Borderless uses desktop"), Draft.GetValue().Resolution == Catalogue.Desktop);
    TestFalse(TEXT("Borderless cannot pick separate size"), Draft.SelectResolution(FIntPoint(1280, 720)));
    TestTrue(TEXT("Return to windowed"), Draft.SelectMode(EETVideoWindowMode::Windowed));
    TestTrue(TEXT("Windowed draft choice remembered"), Draft.GetValue().Resolution == FIntPoint(1600, 900));

    FETVideoResolutionCatalogue Missing = Catalogue;
    Missing.Windowed = {FIntPoint(1280, 720)};
    TestFalse(TEXT("Removed candidate rejects stale preview"), Draft.TryMakePreview(Actual, Missing, Candidate));
    TestFalse(TEXT("Stale preview clears output"), Candidate.IsWellFormed());
    FETVideoMode Changed = Actual;
    Changed.Resolution = FIntPoint(1280, 720);
    TestFalse(TEXT("External actual change rejects old draft"), Draft.TryMakePreview(Changed, Catalogue, Candidate));
    FETVideoMode Aliased = Actual;
    TestTrue(TEXT("Aliased actual and output supported"), Draft.TryMakePreview(Aliased, Catalogue, Aliased));
    TestTrue(TEXT("Aliased output intact"), Aliased.Resolution == FIntPoint(1600, 900));

    TestTrue(TEXT("Select borderless for desktop change"), Draft.SelectMode(EETVideoWindowMode::Borderless));
    FETVideoResolutionCatalogue NewDesktop = Catalogue;
    NewDesktop.Desktop = FIntPoint(1920, 1080);
    TestFalse(TEXT("Desktop change cannot silently replace choice"), Draft.TryMakePreview(Actual, NewDesktop, Candidate));
    TestTrue(TEXT("Fullscreen selectable"), Draft.SelectMode(EETVideoWindowMode::Fullscreen));
    TestTrue(TEXT("Fullscreen uses supplied catalogue"), Draft.GetValue().Resolution == FIntPoint(1920, 1080));
    TestFalse(TEXT("Invalid mode rejected"), Draft.SelectMode(EETVideoWindowMode::Invalid));

    TestTrue(TEXT("Reopen cancels edits"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Reopened draft clean"), Draft.IsDirty());
    Catalogue.Fullscreen.Reset();
    TestTrue(TEXT("Open without fullscreen catalogue"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Unavailable fullscreen rejected"), Draft.SelectMode(EETVideoWindowMode::Fullscreen));
    TArray<EETVideoWindowMode> Modes;
    Draft.GetModes(Modes);
    TestEqual(TEXT("Only available modes offered"), Modes.Num(), 2);

    Draft.Close();
    Draft.GetResolutions(EETVideoWindowMode::Windowed, Choices);
    TestTrue(TEXT("Close clears choices"), Choices.IsEmpty());
    TestFalse(TEXT("Close rejects edits"), Draft.SelectResolution(FIntPoint(1600, 900)));
    TestFalse(TEXT("Invalid actual unavailable"), Draft.Open(FETVideoMode{}, Catalogue));
    TestFalse(TEXT("Invalid open leaves no old session"), Draft.IsOpen());
    TestTrue(TEXT("Next page opens cleanly"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Next page has no pending edits"), Draft.IsDirty());
    return true;
}
#endif
```


## 4. 数据来源与接口含义

| 接口 / 数据 | 本批含义 | 后续页面接法 |
| --- | --- | --- |
| Actual | 当前窗口真实尺寸与模式 | 从B2 GetActualMode读取；不能用Requested / LastConfirmed代替 |
| Catalogue.Desktop | 本机桌面尺寸值快照 | 从本机显示快照读取，与对应目录一起刷新 |
| Windowed / Fullscreen | 平台候选列表副本 | 从B2 GetAvailableResolutions分别取得；不是产品固定列表 |
| Open / Close | 建立 / 丢弃页面草稿 | 打开取实际快照，关闭丢弃；非法Open不会保留旧会话 |
| GetModes / GetResolutions | 列出当前可显示的模式与尺寸 | 下拉框按返回值生成；空目录不造假数据 |
| SelectMode / SelectResolution | 只编辑页面草稿 | 不切屏；模式往返保留各自选择 |
| TryMakePreview | 产出重验后的候选值 | 按预览前重新获取实际模式与目录，再交B2检查Owner / World并Begin |

无边框只有桌面尺寸，不提供单独分辨率选择。首次进入某模式时草稿选其目录第一项，用户仍需明确预览；没有跨设备通用的“默认分辨率”。目录推荐项与当前实际窗口可能不同，因此当前尺寸会加入本页快照，避免打开后看不见当前值；这不扩大B2的真实应用权限。

外部窗口变化或候选从最新目录消失时，TryMakePreview返回false并清空Out；页面应重读快照、提示重新选择。桌面变化也不能悄悄替换用户选中的无边框尺寸。浏览 / 切模式不会改实际快照、ASC、GameUserSettings或磁盘。

未启动真实预览时，“取消修改”可以重新Open当前实际快照；已启动预览时先由宿主Cancel对应Serial，不能只Close草稿。本批没有宿主 / Tick / Timer / 委托，无需在Blueprint增加临时按键或函数。

## 5. 正常Build与索引

保存三份文件、UE保持关闭。PowerShell：

```powershell
Set-Location 'E:UE_ProjectDemo'
.ToolsRefresh-CLion.ps1
```

成功后Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload，预计Core83 / UI34 / Editor3。三文件应属于UI CodeModel，ETVideoSettingsDraft.h与Core视频规则头可导航；不手改生成CMake或清除缓存。

## 6. 一次专项与最短实机检查

### 6.1 草稿专项

打开UE，停止PIE，在Output Log执行一次：

```text
Automation RunTests EmberTrial.UI.Settings.VideoDraft
```

应Success。测试覆盖关闭拒绝、初始无变化、目录去重和当前尺寸可见、浏览无副作用、模式往返、无边框固定、目录 / 外部实际 / 桌面变化后拒绝旧候选、清空失败输出、关闭重开。测试具体尺寸仅是夹具数据，不能复制到产品目录作为固定选项。它不切屏、Apply / Save或创建World，不冒充新确认页已经实现。

### 6.2 原页面短查（约2分钟）

常用地图Play → P打开现有暂停页，原输入卡仍1.00 / 不反转 → P关闭后移动与转向正常 → Stop。没有新显示卡符合本批预期，不需要做新Designer配置。

不搭临时Blueprint，不重复B2真实切屏 / 暂停 / 无边框矩阵，不跑全37项或新Cook；遇真实失败再扩大排查。

## 7. 完成后反馈

```text
M3-05B3-01 完成
三新UI；关闭UE后Editor Build / Core83-UI34-Editor3索引 / Game Build：
UI.Settings.VideoDraft一次：
原P输入卡1 / 不反转、恢复移动转向与Stop短查：
源码已保存，资产 / Config未改；失败或不理解处（无填无）：
完成时分 / 手动代码 / 阅读 / 检查 / 构建等待：
```

长日志由助手读取。编译失败发送首条真实error与附近10–20行；UI状态不符附操作顺序与截图，不只发送exit code 6。

## 8. Commit / Push（固定放在最后）

### 8.1 本轮文档收尾

B2-02源码已在本地`c2df3c1`，只Stage本轮有变化的以下文档；B3-01源码仍为Artifacts候选，不提前Stage：

```text
Docs/Reports/M3_05B2_02_Verification.md
Docs/M3_05B2_02_NativeVideoPreview.md
Docs/M3_05B3_01_VideoSettingsDraft.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/M2_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

英文建议：`docs: verify native video preview and plan display settings draft`。

专业中文：`docs: 完成原生视频预览验收并规划显示设置草稿`。

### 8.2 本章实施后的源码与文档

源码为第2节三新UI文件。英文建议：`feat: add display settings draft with validated preview candidates`；专业中文：`feat: 新增显示设置草稿与预览候选校验`。

文档为本指南与实际更新的验收 / Progress / Roadmap等，未生成报告不建空文件。英文建议：`docs: record display settings draft implementation and verification`；专业中文：`docs: 记录显示设置草稿实现与验证结果`。若源码和文档一起提交，可用上一条feat建议。

Artifacts候选、Runtime / 专用ini、Saved / Intermediate / Binaries、.idea与生成索引留本地。资产 / Config无变化无需Stage。Stage后又编辑须重新Stage → Review → Commit / Push由用户操作。
