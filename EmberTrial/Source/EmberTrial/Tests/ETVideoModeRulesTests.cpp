#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETVideoModeRules.h"
#include "Settings/ETGameUserSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETVideoModeRulesTest,
    "EmberTrial.M3.Settings.VideoModeRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETVideoModeRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    for (const EWindowMode::Type Native : {EWindowMode::Windowed,
        EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen})
    {
        EWindowMode::Type Back = EWindowMode::Windowed;
        TestTrue(TEXT("Known mode round trip"),
            FETVideoModeRules::ToNative(FETVideoModeRules::FromNative(Native), Back) && Back == Native);
    }
    EWindowMode::Type Rejected = EWindowMode::Fullscreen;
    TestFalse(TEXT("Invalid project mode rejected"),
        FETVideoModeRules::ToNative(EETVideoWindowMode::Invalid, Rejected));
    TestTrue(TEXT("Rejected native output is reset"), Rejected == EWindowMode::Windowed);
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
    // 真实引擎窗口调整顺序：先改请求尺寸，再虚调用ConfirmVideoMode。
    UETGameUserSettings* Guarded = NewObject<UETGameUserSettings>();
    if (!TestNotNull(TEXT("Guarded settings object"), Guarded))
    {
        return false;
    }
    Guarded->SetScreenResolution(FIntPoint(1280, 720));
    Guarded->SetFullscreenMode(EWindowMode::Windowed);
    Guarded->ConfirmVideoMode();
    TestTrue(TEXT("Preview guard acquired"), Guarded->BeginVideoPreviewGuard());
    TestFalse(TEXT("Second guard cannot replace first baseline"), Guarded->BeginVideoPreviewGuard());
    Guarded->SetScreenResolution(FIntPoint(1600, 900));
    Guarded->ConfirmVideoMode();
    TestTrue(TEXT("Engine automatic confirmation restores requested and confirmed baseline"),
        Guarded->GetScreenResolution() == FIntPoint(1280, 720) &&
        Guarded->GetLastConfirmedScreenResolution() == FIntPoint(1280, 720));
    Guarded->EndVideoPreviewGuard();
    Guarded->SetScreenResolution(FIntPoint(1600, 900));
    Guarded->ConfirmVideoMode();
    TestTrue(TEXT("Ordinary confirmation works after releasing guard"),
        Guarded->GetLastConfirmedScreenResolution() == FIntPoint(1600, 900));
    // 这里不调用SaveSettings / CommitVideoPreview，不写本机个人文件。
    return true;
}
#endif