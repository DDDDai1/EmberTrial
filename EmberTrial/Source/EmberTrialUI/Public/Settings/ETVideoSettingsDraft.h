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