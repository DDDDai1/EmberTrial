#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETVideoModeRules.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETVideoModeRulesTest,
    "EmberTrial.M3.Settings.VideoModeRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETVideoModeRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FIntPoint Desktop(2560, 1440);
    const TArray<FIntPoint> Sizes = {FIntPoint::ZeroValue, FIntPoint(1280, 720),
        FIntPoint(1920, 1080), FIntPoint(1920, 1080)};
    FETVideoMode Request;
    Request.Mode = EETVideoWindowMode::Windowed;
    Request.Resolution = FIntPoint(1280, 720);
    FETVideoMode Out;
    TestTrue(TEXT("Windowed catalogue entry accepted"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    TestTrue(TEXT("Resolution and mode preserved"), Out == Request);
    Request.Mode = EETVideoWindowMode::Fullscreen;
    TestTrue(TEXT("Fullscreen catalogue entry accepted"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    Request.Resolution = FIntPoint(1500, 900);
    TestFalse(TEXT("Unlisted resolution rejected"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    TestFalse(TEXT("Failure clears candidate"), Out.IsWellFormed());
    Request.Mode = static_cast<EETVideoWindowMode>(255);
    TestFalse(TEXT("Unknown mode rejected"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));

    Request.Mode = EETVideoWindowMode::Borderless;
    Request.Resolution = FIntPoint::ZeroValue;
    TestTrue(TEXT("Borderless uses desktop even without separate resolution choice"),
        FETVideoModeRules::TryResolve(Request, Desktop, {}, Out));
    TestTrue(TEXT("Borderless normalizes to actual supplied desktop"), Out.Resolution == Desktop);
    TestFalse(TEXT("Unknown desktop cannot be invented"),
        FETVideoModeRules::TryResolve(Request, FIntPoint::ZeroValue, Sizes, Out));
    Request.Mode = EETVideoWindowMode::Windowed;
    Request.Resolution = FIntPoint(-1, 720);
    TestFalse(TEXT("Negative dimensions rejected"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Out));
    Request.Resolution = FIntPoint(1280, 720);
    TestTrue(TEXT("Aliased input and output supported"),
        FETVideoModeRules::TryResolve(Request, Desktop, Sizes, Request));
    TestTrue(TEXT("Aliased result is intact"), Request.Resolution == FIntPoint(1280, 720));

    // 独立对象只改配置内存；ConfirmVideoMode只登记确认值，不调用Apply / Save切屏。
    UGameUserSettings* Local = NewObject<UGameUserSettings>();
    if (!TestNotNull(TEXT("Isolated settings object"), Local))
    {
        return false;
    }
    Local->SetScreenResolution(FIntPoint(1600, 900));
    Local->SetFullscreenMode(EWindowMode::Windowed);
    Local->ConfirmVideoMode();
    Local->SetScreenResolution(FIntPoint(1280, 720));
    Local->SetFullscreenMode(EWindowMode::Fullscreen);
    FETVideoModeSnapshot Snapshot;
    TestTrue(TEXT("Native snapshot captured"), FETVideoModeRules::Capture(Local, Snapshot));
    TestTrue(TEXT("Requested differs from last confirmed"),
        Snapshot.Requested.Resolution == FIntPoint(1280, 720) &&
        Snapshot.Requested.Mode == EETVideoWindowMode::Fullscreen &&
        Snapshot.LastConfirmed.Resolution == FIntPoint(1600, 900) &&
        Snapshot.LastConfirmed.Mode == EETVideoWindowMode::Windowed);
    TestTrue(TEXT("Capture does not confirm the pending native request"),
        Local->GetLastConfirmedScreenResolution() == FIntPoint(1600, 900));
    TestFalse(TEXT("Missing settings rejected"), FETVideoModeRules::Capture(nullptr, Snapshot));
    TestTrue(TEXT("Rejected snapshot is cleared"),
        !Snapshot.Requested.IsWellFormed() && Snapshot.DesktopResolution == FIntPoint::ZeroValue);
    return true;
}
#endif