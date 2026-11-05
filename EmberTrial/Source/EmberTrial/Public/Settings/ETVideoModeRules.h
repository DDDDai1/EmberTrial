#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

class UGameUserSettings;

// 原生值类型，不是UENUM / USTRUCT；UI后续用枚举映射显示文字。
enum class EETVideoWindowMode : uint8
{
    Windowed,
    Borderless,
    Fullscreen,
    Invalid
};

struct EMBERTRIAL_API FETVideoMode
{
    FIntPoint Resolution = FIntPoint::ZeroValue;
    EETVideoWindowMode Mode = EETVideoWindowMode::Invalid;

    bool IsWellFormed() const;
    bool operator==(const FETVideoMode& Other) const
    {
        return Resolution == Other.Resolution && Mode == Other.Mode;
    }
};

// Requested / LastConfirmed都是引擎配置状态，不能冒充当前Viewport实际像素。
struct EMBERTRIAL_API FETVideoModeSnapshot
{
    FETVideoMode Requested;
    FETVideoMode LastConfirmed;
    FIntPoint DesktopResolution = FIntPoint::ZeroValue;
};

class EMBERTRIAL_API FETVideoModeRules
{
public:
    // 只读已有引擎实例；未初始化尺寸保留0，由IsWellFormed表达不可用。
    static bool Capture(const UGameUserSettings* Settings, FETVideoModeSnapshot& Out);

    // AllowedSizes由后续平台适配提供；本函数不查询显卡、不切屏或保存。
    static bool TryResolve(const FETVideoMode& Request, FIntPoint Desktop,
        TConstArrayView<FIntPoint> AllowedSizes, FETVideoMode& Out);
};