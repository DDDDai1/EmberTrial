#pragma once

#include "Internationalization/Text.h"

// 只共享数值显示策略，不接触Presenter、UMG状态或资源数值。
// Pattern由调用方的LOCTEXT提供，保留各面板的本地化命名空间和Key。
namespace ETResourceHUDText
{
inline FText FormatValues(const FText& Pattern, float Current, float Maximum)
{
    FNumberFormattingOptions NumberOptions;
    NumberOptions.SetMinimumFractionalDigits(0);
    NumberOptions.SetMaximumFractionalDigits(0);
    return FText::Format(Pattern, FText::AsNumber(Current, &NumberOptions),
                         FText::AsNumber(Maximum, &NumberOptions));
}
} // namespace ETResourceHUDText
