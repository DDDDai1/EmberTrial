#include "Settings/ETInputSettingsDraft.h"

// ---- 打开 / 关闭拥有草稿；不会应用设置或保存文件 ----

bool FETInputSettingsDraft::Open(const FETInputPreferences& Applied)
{
    Close();
    if (!Applied.IsValid())
    {
        return false;
    }
    Baseline = Value = Applied;
    bOpen = true;
    return true;
}

void FETInputSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETInputPreferences{};
}

// ---- 本地编辑；拒绝非法值时保留原草稿 ----

bool FETInputSettingsDraft::SetSensitivity(const float NewValue)
{
    FETInputPreferences Next = Value;
    Next.MouseSensitivity = NewValue;
    if (!bOpen || !Next.IsValid())
    {
        return false;
    }
    Value = Next;
    return true;
}

bool FETInputSettingsDraft::SetInvertY(const bool bValue)
{
    if (!bOpen)
    {
        return false;
    }
    Value.bInvertMouseY = bValue;
    return true;
}

void FETInputSettingsDraft::UseDefaults()
{
    if (bOpen)
    {
        Value = FETInputPreferences{};
    }
}

// ---- 状态与最终候选；浮点微差不产生虚假的未应用提示 ----

bool FETInputSettingsDraft::Same(const FETInputPreferences& A, const FETInputPreferences& B)
{
    return A.IsValid() && B.IsValid() &&
        FMath::IsNearlyEqual(A.MouseSensitivity, B.MouseSensitivity, KINDA_SMALL_NUMBER) &&
        A.bInvertMouseY == B.bInvertMouseY;
}

bool FETInputSettingsDraft::IsDirty() const
{
    return bOpen && !Same(Value, Baseline);
}

bool FETInputSettingsDraft::IsDefault() const
{
    return bOpen && Same(Value, FETInputPreferences{});
}

bool FETInputSettingsDraft::TryMakeApply(
    const FETInputPreferences& Current, FETInputPreferences& Out) const
{
    Out = FETInputPreferences{};
    if (!bOpen || !Value.IsValid() || !Same(Current, Baseline))
    {
        return false;
    }
    Out = Value;
    return true;
}