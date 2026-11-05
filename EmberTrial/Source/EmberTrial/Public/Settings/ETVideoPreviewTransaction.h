#pragma once

#include "CoreMinimal.h"
#include "Settings/ETVideoModeRules.h"

enum class EETVideoPreviewPhase : uint8
{
    Idle,
    Applying,
    AwaitingConfirmation,
    Restoring,
    RestoreFailed
};

enum class EETVideoPreviewResult : uint8
{
    None,
    NoChange,
    Confirmed,
    Cancelled,
    TimedOut,
    ApplyFailed,
    ExternalChange,
    RestoreFailed
};

enum class EETVideoPreviewCommandType : uint8
{
    None,
    ApplyCandidate,
    RestoreBaseline,
    CommitCandidate
};

// 单次调用返回的副作用意图；内核不会执行切屏或保存。
struct EMBERTRIAL_API FETVideoPreviewCommand
{
    EETVideoPreviewCommandType Type = EETVideoPreviewCommandType::None;
    FETVideoMode Mode;
    uint64 Serial = 0;
};

struct EMBERTRIAL_API FETVideoPreviewTiming
{
    double ApplySeconds = 3.0;
    double ConfirmSeconds = 15.0;
    double RestoreSeconds = 3.0;

    bool IsValid() const;
};

struct EMBERTRIAL_API FETVideoPreviewSnapshot
{
    EETVideoPreviewPhase Phase = EETVideoPreviewPhase::Idle;
    EETVideoPreviewResult Result = EETVideoPreviewResult::None;
    FETVideoMode Baseline;
    FETVideoMode Candidate;
    uint64 Serial = 0;
    double Deadline = 0.0;
};

/** 视频预览事务内核；值与绝对单调时间输入，没有World / UObject / I/O依赖。 */
class EMBERTRIAL_API FETVideoPreviewTransaction
{
public:
    explicit FETVideoPreviewTransaction(FETVideoPreviewTiming InTiming = {});

    FETVideoPreviewCommand Begin(const FETVideoMode& ActualBaseline,
        const FETVideoMode& ResolvedCandidate, double Now);
    FETVideoPreviewCommand Advance(double Now, const FETVideoMode& Actual);
    FETVideoPreviewCommand Confirm(uint64 ExpectedSerial, double Now, const FETVideoMode& Actual);
    FETVideoPreviewCommand Cancel(uint64 ExpectedSerial, double Now);

    const FETVideoPreviewSnapshot& GetSnapshot() const { return Snapshot; }
    double GetRemainingSeconds(double Now) const;

private:
    bool CanUseTime(double Now) const;
    FETVideoPreviewCommand StartRestore(double Now, EETVideoPreviewResult Reason);
    FETVideoPreviewCommand MakeCommand(EETVideoPreviewCommandType Type, const FETVideoMode& Mode) const;

    FETVideoPreviewTiming Timing;
    FETVideoPreviewSnapshot Snapshot;
    EETVideoPreviewResult RestoreReason = EETVideoPreviewResult::None;
    double LastNow = 0.0;
};