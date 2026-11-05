#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETVideoPreviewTransaction.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETVideoPreviewTransactionTest,
    "EmberTrial.M3.Settings.VideoPreviewTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETVideoPreviewTransactionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETVideoMode Base;
    Base.Mode = EETVideoWindowMode::Windowed;
    Base.Resolution = FIntPoint(1280, 720);
    FETVideoMode Candidate = Base;
    Candidate.Resolution = FIntPoint(1600, 900);
    FETVideoMode Other = Base;
    Other.Resolution = FIntPoint(1920, 1080);
    auto Is = [&](const TCHAR* Label, const FETVideoPreviewCommand& Command,
        const EETVideoPreviewCommandType Type)
    {
        return TestTrue(Label, Command.Type == Type);
    };
    FETVideoPreviewTransaction Tx;
    Is(TEXT("Begin requests candidate once"), Tx.Begin(Base, Candidate, 10.0),
        EETVideoPreviewCommandType::ApplyCandidate);
    const uint64 First = Tx.GetSnapshot().Serial;
    Is(TEXT("Busy begin cannot replace baseline"), Tx.Begin(Other, Candidate, 10.25),
        EETVideoPreviewCommandType::None);
    Is(TEXT("Early confirm is not queued"), Tx.Confirm(First, 10.25, Candidate),
        EETVideoPreviewCommandType::None);
    Tx.Advance(10.5, Candidate);
    TestTrue(TEXT("Observed candidate awaits confirmation"),
        Tx.GetSnapshot().Phase == EETVideoPreviewPhase::AwaitingConfirmation);
    TestTrue(TEXT("Full 15 seconds starts after observation"),
        FMath::IsNearlyEqual(Tx.GetRemainingSeconds(10.5), 15.0));
    Is(TEXT("Explicit confirmation emits commit"), Tx.Confirm(First, 11.0, Candidate),
        EETVideoPreviewCommandType::CommitCandidate);
    Is(TEXT("Repeated confirmation cannot save twice"), Tx.Confirm(First, 11.25, Candidate),
        EETVideoPreviewCommandType::None);

    Tx.Begin(Base, Candidate, 20.0);
    const uint64 Second = Tx.GetSnapshot().Serial;
    TestTrue(TEXT("Each accepted preview has a new serial"), Second > First);
    Is(TEXT("Old page cannot cancel next preview"), Tx.Cancel(First, 20.25),
        EETVideoPreviewCommandType::None);
    Is(TEXT("Cancel while applying requests baseline"), Tx.Cancel(Second, 20.25),
        EETVideoPreviewCommandType::RestoreBaseline);
    const double RestoreDeadline = Tx.GetSnapshot().Deadline;
    Tx.Cancel(Second, 20.5);
    TestTrue(TEXT("Repeated cancel does not extend restoration"),
        Tx.GetSnapshot().Deadline == RestoreDeadline);
    Tx.Advance(20.75, Base);
    TestTrue(TEXT("Only actual restoration ends cancellation"),
        Tx.GetSnapshot().Phase == EETVideoPreviewPhase::Idle &&
        Tx.GetSnapshot().Result == EETVideoPreviewResult::Cancelled);

    // 使用能精确表示的半秒 / 四分之一秒，避免测试依赖浮点十进制边界误差。
    Tx.Begin(Base, Candidate, 100.0);
    Tx.Advance(100.5, Candidate);
    Is(TEXT("Old confirmation cannot approve a new waiting preview"),
        Tx.Confirm(First, 100.75, Candidate), EETVideoPreviewCommandType::None);
    Is(TEXT("Confirmation at deadline must restore, never commit"),
        Tx.Confirm(Tx.GetSnapshot().Serial, 115.5, Candidate),
        EETVideoPreviewCommandType::RestoreBaseline);
    Tx.Advance(115.75, Base);
    TestTrue(TEXT("Expired preview reports timeout after recovery"),
        Tx.GetSnapshot().Result == EETVideoPreviewResult::TimedOut);

    Tx.Begin(Base, Candidate, 200.0);
    Is(TEXT("OS not applying candidate eventually restores"), Tx.Advance(204.0, Base),
        EETVideoPreviewCommandType::RestoreBaseline);
    Tx.Advance(204.25, Base);
    TestTrue(TEXT("Apply failure distinguished"),
        Tx.GetSnapshot().Result == EETVideoPreviewResult::ApplyFailed);

    Tx.Begin(Base, Candidate, 300.0);
    Tx.Advance(300.5, Candidate);
    Is(TEXT("External mode change revokes confirmation"), Tx.Advance(301.0, Other),
        EETVideoPreviewCommandType::RestoreBaseline);
    Tx.Advance(301.25, Base);
    TestTrue(TEXT("External change recorded"),
        Tx.GetSnapshot().Result == EETVideoPreviewResult::ExternalChange);

    Tx.Begin(Base, Candidate, 400.0);
    const uint64 FailedSerial = Tx.GetSnapshot().Serial;
    Tx.Cancel(FailedSerial, 400.25);
    Tx.Advance(404.0, Candidate);
    TestTrue(TEXT("Failed restore is explicit, not silently idle"),
        Tx.GetSnapshot().Phase == EETVideoPreviewPhase::RestoreFailed);
    Is(TEXT("Failure cannot overwrite original baseline"), Tx.Begin(Other, Candidate, 405.0),
        EETVideoPreviewCommandType::None);
    Is(TEXT("Explicit cancel can retry same baseline"), Tx.Cancel(FailedSerial, 405.0),
        EETVideoPreviewCommandType::RestoreBaseline);
    Tx.Advance(405.25, Base);
    TestTrue(TEXT("Retry retains original cancel outcome"),
        Tx.GetSnapshot().Result == EETVideoPreviewResult::Cancelled);

    const double NaN = std::numeric_limits<double>::quiet_NaN();
    Is(TEXT("Invalid time cannot start preview"), Tx.Begin(Base, Candidate, NaN),
        EETVideoPreviewCommandType::None);
    Is(TEXT("Clock cannot move backwards"), Tx.Begin(Base, Candidate, 1.0),
        EETVideoPreviewCommandType::None);
    FETVideoPreviewTransaction Invalid(FETVideoPreviewTiming{0.0, 15.0, 3.0});
    Is(TEXT("Invalid timing policy is rejected"), Invalid.Begin(Base, Candidate, 1.0),
        EETVideoPreviewCommandType::None);
    FETVideoPreviewTransaction Clean;
    Is(TEXT("Unchanged mode does not apply"), Clean.Begin(Base, Base, 1.0),
        EETVideoPreviewCommandType::None);
    TestTrue(TEXT("No change stays idle"), Clean.GetSnapshot().Phase == EETVideoPreviewPhase::Idle);
    Is(TEXT("Invalid actual baseline rejected"), Clean.Begin(FETVideoMode{}, Candidate, 2.0),
        EETVideoPreviewCommandType::None);
    return true;
}
#endif