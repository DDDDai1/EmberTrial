#include "Settings/ETVideoPreviewTransaction.h"

bool FETVideoPreviewTiming::IsValid() const
{
    return FMath::IsFinite(ApplySeconds) && ApplySeconds > 0.0 &&
        FMath::IsFinite(ConfirmSeconds) && ConfirmSeconds > 0.0 &&
        FMath::IsFinite(RestoreSeconds) && RestoreSeconds > 0.0;
}

FETVideoPreviewTransaction::FETVideoPreviewTransaction(const FETVideoPreviewTiming InTiming)
    : Timing(InTiming)
{
}

bool FETVideoPreviewTransaction::CanUseTime(const double Now) const
{
    return Timing.IsValid() && FMath::IsFinite(Now) && Now >= LastNow && Now >= 0.0 &&
        FMath::IsFinite(Now + Timing.ApplySeconds + Timing.ConfirmSeconds + Timing.RestoreSeconds);
}

FETVideoPreviewCommand FETVideoPreviewTransaction::MakeCommand(
    const EETVideoPreviewCommandType Type, const FETVideoMode& Mode) const
{
    return {Type, Mode, Snapshot.Serial};
}

// ---- 先接收实际基线，再申请候选；繁忙 / 非法请求没有任何副作用意图 ----

FETVideoPreviewCommand FETVideoPreviewTransaction::Begin(
    const FETVideoMode& ActualBaseline, const FETVideoMode& ResolvedCandidate, const double Now)
{
    if (Snapshot.Phase != EETVideoPreviewPhase::Idle || !CanUseTime(Now) ||
        !ActualBaseline.IsWellFormed() || !ResolvedCandidate.IsWellFormed() ||
        Snapshot.Serial == MAX_uint64)
    {
        return {};
    }
    const FETVideoMode BaselineCopy = ActualBaseline;
    const FETVideoMode CandidateCopy = ResolvedCandidate;
    LastNow = Now;
    Snapshot.Baseline = BaselineCopy;
    Snapshot.Candidate = CandidateCopy;
    Snapshot.Deadline = 0.0;
    Snapshot.Result = EETVideoPreviewResult::NoChange;
    if (BaselineCopy == CandidateCopy)
    {
        return {};
    }

    ++Snapshot.Serial;
    Snapshot.Phase = EETVideoPreviewPhase::Applying;
    Snapshot.Result = EETVideoPreviewResult::None;
    Snapshot.Deadline = Now + Timing.ApplySeconds;
    RestoreReason = EETVideoPreviewResult::None;
    return MakeCommand(EETVideoPreviewCommandType::ApplyCandidate, Snapshot.Candidate);
}

FETVideoPreviewCommand FETVideoPreviewTransaction::StartRestore(
    const double Now, const EETVideoPreviewResult Reason)
{
    Snapshot.Phase = EETVideoPreviewPhase::Restoring;
    Snapshot.Result = EETVideoPreviewResult::None;
    Snapshot.Deadline = Now + Timing.RestoreSeconds;
    RestoreReason = Reason;
    return MakeCommand(EETVideoPreviewCommandType::RestoreBaseline, Snapshot.Baseline);
}

// ---- 观察实际结果；请求已发出不等于OS已切换，恢复也必须再次观察 ----

FETVideoPreviewCommand FETVideoPreviewTransaction::Advance(
    const double Now, const FETVideoMode& Actual)
{
    if (Snapshot.Phase == EETVideoPreviewPhase::Idle ||
        Snapshot.Phase == EETVideoPreviewPhase::RestoreFailed || !CanUseTime(Now))
    {
        return {};
    }
    LastNow = Now;
    switch (Snapshot.Phase)
    {
    case EETVideoPreviewPhase::Applying:
        if (Actual == Snapshot.Candidate)
        {
            // 候选真正被观察到后才给足确认时间；不消耗应用等待的秒数。
            Snapshot.Phase = EETVideoPreviewPhase::AwaitingConfirmation;
            Snapshot.Deadline = Now + Timing.ConfirmSeconds;
        }
        else if (Now >= Snapshot.Deadline)
        {
            return StartRestore(Now, EETVideoPreviewResult::ApplyFailed);
        }
        break;
    case EETVideoPreviewPhase::AwaitingConfirmation:
        if (Now >= Snapshot.Deadline)
        {
            return StartRestore(Now, EETVideoPreviewResult::TimedOut);
        }
        if (!(Actual == Snapshot.Candidate))
        {
            return StartRestore(Now, EETVideoPreviewResult::ExternalChange);
        }
        break;
    case EETVideoPreviewPhase::Restoring:
        if (Actual == Snapshot.Baseline)
        {
            Snapshot.Phase = EETVideoPreviewPhase::Idle;
            Snapshot.Result = RestoreReason;
            Snapshot.Deadline = 0.0;
        }
        else if (Now >= Snapshot.Deadline)
        {
            // 保留基线和Serial，可显式重试；不能循环请求或假报已恢复。
            Snapshot.Phase = EETVideoPreviewPhase::RestoreFailed;
            Snapshot.Result = EETVideoPreviewResult::RestoreFailed;
            Snapshot.Deadline = 0.0;
        }
        break;
    default:
        break;
    }
    return {};
}

// ---- 确认严格重验Serial / 阶段 / 时间 / 实际模式；过期确认返回恢复意图 ----

FETVideoPreviewCommand FETVideoPreviewTransaction::Confirm(
    const uint64 ExpectedSerial, const double Now, const FETVideoMode& Actual)
{
    if (ExpectedSerial != Snapshot.Serial ||
        Snapshot.Phase != EETVideoPreviewPhase::AwaitingConfirmation || !CanUseTime(Now))
    {
        return {};
    }
    const FETVideoPreviewCommand Command = Advance(Now, Actual);
    if (Command.Type != EETVideoPreviewCommandType::None ||
        Snapshot.Phase != EETVideoPreviewPhase::AwaitingConfirmation)
    {
        return Command;
    }
    Snapshot.Phase = EETVideoPreviewPhase::Idle;
    Snapshot.Result = EETVideoPreviewResult::Confirmed;
    Snapshot.Deadline = 0.0;
    // Confirmed表示允许提交；真实适配层仍须执行此命令并记录保存请求。
    return MakeCommand(EETVideoPreviewCommandType::CommitCandidate, Snapshot.Candidate);
}

FETVideoPreviewCommand FETVideoPreviewTransaction::Cancel(
    const uint64 ExpectedSerial, const double Now)
{
    if (ExpectedSerial != Snapshot.Serial || !CanUseTime(Now) ||
        Snapshot.Phase == EETVideoPreviewPhase::Idle ||
        Snapshot.Phase == EETVideoPreviewPhase::Restoring)
    {
        return {};
    }
    LastNow = Now;
    return StartRestore(Now, Snapshot.Phase == EETVideoPreviewPhase::RestoreFailed
        ? RestoreReason : EETVideoPreviewResult::Cancelled);
}

double FETVideoPreviewTransaction::GetRemainingSeconds(const double Now) const
{
    return Snapshot.Phase == EETVideoPreviewPhase::AwaitingConfirmation && CanUseTime(Now)
        ? FMath::Max(0.0, Snapshot.Deadline - Now) : 0.0;
}