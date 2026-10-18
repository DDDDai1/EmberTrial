#if WITH_DEV_AUTOMATION_TESTS

#include "Widgets/ETResourceHUDText.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"

namespace
{
// 仅测试临时切换locale；作用域结束恢复语言、locale及asset-group完整快照。
class FResourceHUDCultureRestore
{
public:
    explicit FResourceHUDCultureRestore(
        const FInternationalization::FCultureStateSnapshot& InSnapshot)
        : Snapshot(InSnapshot)
    {
    }

    ~FResourceHUDCultureRestore()
    {
        FInternationalization::Get().RestoreCultureState(Snapshot);
    }

    FResourceHUDCultureRestore(const FResourceHUDCultureRestore&) = delete;
    FResourceHUDCultureRestore& operator=(const FResourceHUDCultureRestore&) = delete;

private:
    FInternationalization::FCultureStateSnapshot Snapshot;
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETResourceHUDFormattingTest,
                                 "EmberTrial.UI.ResourceHUD.Formatting",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETResourceHUDFormattingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FInternationalization& I18N = FInternationalization::Get();
    FInternationalization::FCultureStateSnapshot Before;
    I18N.BackupCultureState(Before);
    {
        const FResourceHUDCultureRestore Restore(Before);
        const FText PairPattern = FText::AsCultureInvariant(TEXT("{0} / {1}"));
        for (const TCHAR* Locale : {TEXT("en-US"), TEXT("de-DE"), TEXT("zh-Hans")})
        {
            if (!TestTrue(FString::Printf(TEXT("Locale available: %s"), Locale),
                          I18N.SetCurrentLocale(Locale)))
            {
                continue;
            }
            const auto ExpectPair = [this, &PairPattern, Locale](const TCHAR* Case, float Current,
                                                                 float Maximum,
                                                                 const TCHAR* Expected)
            {
                TestEqual(FString::Printf(TEXT("%s: %s"), Locale, Case),
                          ETResourceHUDText::FormatValues(PairPattern, Current, Maximum).ToString(),
                          FString(Expected));
            };
            ExpectPair(TEXT("Full resource"), 100.0f, 100.0f, TEXT("100 / 100"));
            ExpectPair(TEXT("Zero resource remains visible"), 0.0f, 100.0f, TEXT("0 / 100"));
            ExpectPair(TEXT("Whole-number display rounding"), 12.4f, 100.6f, TEXT("12 / 101"));
            const bool bGerman = FString(Locale) == TEXT("de-DE");
            ExpectPair(TEXT("Locale-specific grouping"), 1234.0f, 10000.0f,
                       bGerman ? TEXT("1.234 / 10.000") : TEXT("1,234 / 10,000"));
            TestEqual(FString::Printf(TEXT("%s: caller owns localized pattern"), Locale),
                      ETResourceHUDText::FormatValues(
                          FText::AsCultureInvariant(TEXT("体力 {1}，当前 {0}")), 75.0f, 100.0f)
                          .ToString(),
                      FString(TEXT("体力 100，当前 75")));
        }

        // 保留FText的数字历史，语言环境改变后仍可重新生成，不提前转成FString缓存。
        if (I18N.SetCurrentLocale(TEXT("en-US")))
        {
            const FText Retained = ETResourceHUDText::FormatValues(PairPattern, 1234.0f, 10000.0f);
            TestEqual(TEXT("Retained text initially uses English grouping"), Retained.ToString(),
                      FString(TEXT("1,234 / 10,000")));
            if (TestTrue(TEXT("Retained text switches locale"),
                         I18N.SetCurrentLocale(TEXT("de-DE"))))
            {
                TestEqual(TEXT("Retained FText regenerates numeric history"), Retained.ToString(),
                          FString(TEXT("1.234 / 10.000")));
            }
        }
    }

    FInternationalization::FCultureStateSnapshot After;
    I18N.BackupCultureState(After);
    TestEqual(TEXT("Original language restored"), After.Language, Before.Language);
    TestEqual(TEXT("Original locale restored"), After.Locale, Before.Locale);
    bool bGroupsRestored = After.AssetGroups.Num() == Before.AssetGroups.Num();
    for (const TTuple<FName, FString>& Group : Before.AssetGroups)
    {
        const TTuple<FName, FString>* Restored =
            After.AssetGroups.FindByPredicate([&Group](const TTuple<FName, FString>& Candidate)
                                              { return Candidate.Get<0>() == Group.Get<0>(); });
        bGroupsRestored &= Restored && Restored->Get<1>() == Group.Get<1>();
    }
    TestTrue(TEXT("Original asset-group cultures restored"), bGroupsRestored);
    AddInfo(TEXT("HUD formatting: en-US/de-DE/zh-Hans, rounding, grouping, FText history and "
                 "culture restore"));
    return !HasAnyErrors();
}

#endif
