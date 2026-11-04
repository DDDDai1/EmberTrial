#pragma once

#include "CoreMinimal.h"
#include "Settings/ETGameUserSettings.h"

/** 页面草稿；只保存值副本，不持有引擎设置、Pawn或Widget，也不负责写盘。 */
class EMBERTRIALUI_API FETInputSettingsDraft
{
public:
    bool Open(const FETInputPreferences& Applied);
    void Close();

    bool SetSensitivity(float NewValue);
    bool SetInvertY(bool bValue);
    void UseDefaults();

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const;
    bool IsDefault() const;
    const FETInputPreferences& GetValue() const { return Value; }

    // 确认时重验当前已应用值；其他入口改过设置就拒绝旧草稿，调用者重新读取。
    bool TryMakeApply(const FETInputPreferences& Current, FETInputPreferences& Out) const;

private:
    static bool Same(const FETInputPreferences& A, const FETInputPreferences& B);

    FETInputPreferences Baseline;
    FETInputPreferences Value;
    bool bOpen = false;
};