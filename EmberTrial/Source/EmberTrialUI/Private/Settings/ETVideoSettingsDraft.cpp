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