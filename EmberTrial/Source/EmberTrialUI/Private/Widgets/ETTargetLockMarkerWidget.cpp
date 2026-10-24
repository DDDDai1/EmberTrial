#include "Widgets/ETTargetLockMarkerWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

void UETTargetLockMarkerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    ApplyMarkerFrame(false, FVector2D::ZeroVector);
}

bool UETTargetLockMarkerWidget::HasValidMarkerLayout() const
{
    return LockMarker && Cast<UCanvasPanelSlot>(LockMarker->Slot);
}

void UETTargetLockMarkerWidget::ApplyMarkerFrame(bool bShow, const FVector2D& Position)
{
    if (!LockMarker)
    {
        return;
    }
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(LockMarker->Slot);
    if (!bShow || !CanvasSlot || Position.ContainsNaN())
    {
        LockMarker->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    if (CanvasSlot->GetPosition() != Position)
    {
        CanvasSlot->SetPosition(Position);
    }
    LockMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}