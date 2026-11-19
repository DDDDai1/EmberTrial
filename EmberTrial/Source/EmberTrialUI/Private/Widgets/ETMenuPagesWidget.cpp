#include "Widgets/ETMenuPagesWidget.h"
#include "Widgets/ETInputSettingsWidget.h"
#include "Widgets/ETVideoSettingsWidget.h"
#include "Widgets/ETGraphicsSettingsWidget.h"
#include "Widgets/ETAudioSettingsWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"

namespace
{
    enum EMenuPage { Home, Settings, Relics };
}

void UETMenuPagesWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (SettingsButton) SettingsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenSettings);
    if (SettingsBack) SettingsBack->OnClicked.AddUniqueDynamic(this, &ThisClass::BackFromSettings);
    if (InputTab) InputTab->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectInput);
    if (VideoTab) VideoTab->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectVideo);
    if (GraphicsTab) GraphicsTab->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectGraphics);
    if (AudioTab) AudioTab->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectAudio);
    if (BrowseRelics) BrowseRelics->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenRelics);
    if (RelicsBack) RelicsBack->OnClicked.AddUniqueDynamic(this, &ThisClass::BackFromRelics);
}

void UETMenuPagesWidget::SetNavigationBusy(bool bBusy)
{
    bNavigationBusy = bBusy;
    if (SettingsButton) SettingsButton->SetIsEnabled(!bBusy);
    if (BrowseRelics) BrowseRelics->SetIsEnabled(!bBusy);
}

void UETMenuPagesWidget::FocusButton(UButton* Button)
{
    if (Button && !bNavigationBusy && GetOwningPlayer())
        Button->SetUserFocus(GetOwningPlayer());
}

void UETMenuPagesWidget::SetPrimary(UButton* Button, bool bPrimary)
{
    if (!Button) return;
    FButtonStyle Style = Button->GetStyle();
    auto Tint = [](FSlateBrush& Brush, FColor Color)
    {
        Brush.TintColor = FSlateColor(FLinearColor::FromSRGBColor(Color));
    };
    Tint(Style.Normal, bPrimary ? FColor(173,116,69) : FColor(36,42,48));
    Tint(Style.Hovered, bPrimary ? FColor(185,134,81) : FColor(65,65,65));
    Tint(Style.Pressed, bPrimary ? FColor(167,111,67) : FColor(77,65,52));
    Button->SetStyle(Style);
    if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::FromSRGBColor(
            bPrimary ? FColor(16,13,10) : FColor(243,235,221))));
}

bool UETMenuPagesWidget::IsHome() const
{
    return !MenuPages || MenuPages->GetActiveWidgetIndex() == Home;
}

bool UETMenuPagesWidget::IsSettingsPage() const
{
    return MenuPages && MenuPages->GetActiveWidgetIndex() == Settings;
}

bool UETMenuPagesWidget::HasNavigationLayout(bool bNeedsRelics) const
{
    return MenuPages && SettingsPages && SettingsButton && SettingsBack &&
        InputTab && VideoTab && GraphicsTab && AudioTab &&
        (!bNeedsRelics || (BrowseRelics && RelicsBack));
}

void UETMenuPagesWidget::OpenSettings()
{
    if (bNavigationBusy || !MenuPages || !SettingsPages) return;
    MenuPages->SetActiveWidgetIndex(Settings);
    SelectCategory(0);
}

void UETMenuPagesWidget::DiscardActiveDraft()
{
    UWidget* Card = SettingsPages ? SettingsPages->GetActiveWidget() : nullptr;
    if (auto* Input = Cast<UETInputSettingsWidget>(Card)) Input->DiscardPageDraft();
    else if (auto* Video = Cast<UETVideoSettingsWidget>(Card)) Video->DiscardPageDraft();
    else if (auto* Graphics = Cast<UETGraphicsSettingsWidget>(Card)) Graphics->DiscardPageDraft();
    else if (auto* Audio = Cast<UETAudioSettingsWidget>(Card)) Audio->DiscardPageDraft();
}

void UETMenuPagesWidget::SelectCategory(int32 Index)
{
    if (bNavigationBusy || !SettingsPages) return;
    if (SettingsPages->GetActiveWidgetIndex() != Index)
    {
        DiscardActiveDraft();
        SettingsPages->SetActiveWidgetIndex(Index);
    }
    UButton* Tabs[] = {InputTab, VideoTab, GraphicsTab, AudioTab};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Tabs); ++I)
        SetPrimary(Tabs[I], I == Index);
    FocusButton(Tabs[Index]);
}

void UETMenuPagesWidget::FocusCurrentCategory()
{
    if (SettingsPages) SelectCategory(SettingsPages->GetActiveWidgetIndex());
}
void UETMenuPagesWidget::SelectInput() { SelectCategory(0); }
void UETMenuPagesWidget::SelectVideo() { SelectCategory(1); }
void UETMenuPagesWidget::SelectGraphics() { SelectCategory(2); }
void UETMenuPagesWidget::SelectAudio() { SelectCategory(3); }

void UETMenuPagesWidget::BackFromSettings()
{
    if (bNavigationBusy || !MenuPages) return;
    DiscardActiveDraft();
    MenuPages->SetActiveWidgetIndex(Home);
    FocusButton(SettingsButton);
}

void UETMenuPagesWidget::OpenRelics()
{
    if (bNavigationBusy || !MenuPages || !RelicsBack) return;
    MenuPages->SetActiveWidgetIndex(Relics);
    FocusRelicReturn();
}

void UETMenuPagesWidget::OpenAltarPage()
{
    bAltarPage = true;
    if (MenuPages) MenuPages->SetActiveWidgetIndex(Relics);
}

void UETMenuPagesWidget::FocusRelicReturn() { FocusButton(RelicsBack); }

void UETMenuPagesWidget::BackFromRelics()
{
    if (bNavigationBusy || !MenuPages) return;
    if (bAltarPage)
    {
        LeaveAltar();
        return;
    }
    MenuPages->SetActiveWidgetIndex(Home);
    FocusButton(BrowseRelics);
}

FReply UETMenuPagesWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape && MenuPages && !IsHome())
    {
        if (!Event.IsRepeat())
        {
            if (MenuPages->GetActiveWidgetIndex() == Settings) BackFromSettings();
            else BackFromRelics();
        }
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

int32 UETMenuPagesWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& DrawList, int32 LayerId,
    const FWidgetStyle& Style, bool bEnabled) const
{
    int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, DrawList, LayerId, Style, bEnabled);
    const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (!Focused || !HasFocusedDescendants()) return Layer;
    const FGeometry& Target = Focused->GetPaintSpaceGeometry();
    if (Target.GetLocalSize().GetMin() <= 0) return Layer;
    const FVector2D A = Geometry.AbsoluteToLocal(Target.LocalToAbsolute(FVector2D::ZeroVector));
    const FVector2D B = Geometry.AbsoluteToLocal(Target.LocalToAbsolute(Target.GetLocalSize()));
    if (A.X >= B.X || A.Y >= B.Y || (B - A).SizeSquared() > Geometry.GetLocalSize().SizeSquared() * 0.8)
        return Layer;
    const TArray<FVector2f> Points = { FVector2f(A), FVector2f(B.X, A.Y), FVector2f(B),
        FVector2f(A.X, B.Y), FVector2f(A) };
    FSlateDrawElement::MakeLines(DrawList, ++Layer, Geometry.ToPaintGeometry(), Points,
        ESlateDrawEffect::None, FLinearColor::FromSRGBColor(FColor(242,200,135)), true, 2.0f);
    return Layer;
}
