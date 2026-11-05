#include "Widgets/ETGraphicsSettingsWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Settings/ETGameUserSettings.h"
#include "Settings/ETVideoSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGraphicsSettingsView, Log, All);

namespace ETGraphicsViewPrivate
{
FString QualityLabel(const int32 Level)
{
    switch (Level)
    {
    case -1: return TEXT("自定义（保留当前组）");
    case 0: return TEXT("低");
    case 1: return TEXT("中");
    case 2: return TEXT("高");
    case 3: return TEXT("极高");
    case 4: return TEXT("影视级");
    default: return TEXT("不可用");
    }
}

FString FrameRateLabel(const float Limit)
{
    if (Limit == 0.0f)
    {
        return TEXT("不限制（0）");
    }
    FNumberFormattingOptions Number;
    Number.SetMinimumFractionalDigits(0);
    Number.SetMaximumFractionalDigits(2);
    return FString::Printf(TEXT("%s FPS"), *FText::AsNumber(Limit, &Number).ToString());
}
} // namespace ETGraphicsViewPrivate

// ---- 初始化只绑一次；每次Construct观察新服务，宿主关闭先撤销观察 ----

bool UETGraphicsSettingsWidget::HasRequiredLayout() const
{
    return IsValid(QualityCombo.Get()) && IsValid(FrameRateCombo.Get()) &&
        IsValid(VSyncCheckBox.Get()) && IsValid(ApplyButton.Get()) &&
        IsValid(RevertButton.Get()) && IsValid(DefaultsButton.Get()) &&
        IsValid(GraphicsCurrentText.Get()) && IsValid(GraphicsStatusText.Get());
}

void UETGraphicsSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    QualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleQualityChanged);
    FrameRateCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleFrameRateChanged);
    VSyncCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleVSyncChanged);
    ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleApply);
    RevertButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRevert);
    DefaultsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDefaults);
}

void UETGraphicsSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !HasRequiredLayout())
    {
        return;
    }
    bViewActive = true;
    Settings = UETGameUserSettings::GetETGameUserSettings();
    if (UGameInstance* Instance = GetGameInstance())
    {
        PreviewService = Instance->GetSubsystem<UETVideoSettingsSubsystem>();
    }
    if (UETVideoSettingsSubsystem* Service = PreviewService.Get())
    {
        Service->OnChanged.Remove(PreviewChangedHandle);
        PreviewChangedHandle = Service->OnChanged.AddUObject(
            this, &ThisClass::HandlePreviewSnapshot);
    }
    ReadCurrent(FText::FromString(TEXT("修改后选择应用；离开页面丢弃未应用草稿。")));
}

void UETGraphicsSettingsWidget::NativeDestruct()
{
    CloseView();
    Super::NativeDestruct();
}

void UETGraphicsSettingsWidget::CloseView()
{
    bViewActive = false;
    if (UETVideoSettingsSubsystem* Service = PreviewService.Get())
    {
        Service->OnChanged.Remove(PreviewChangedHandle);
    }
    PreviewChangedHandle.Reset();
    PreviewService.Reset();
    Settings.Reset();
    Draft.Close();
    QualityLevels.Reset();
    FrameRateLimits.Reset();
}

// ---- 资格与呈现；视频预览事件只刷新可用性，不清理本卡未应用草稿 ----

bool UETGraphicsSettingsWidget::HasLiveHost() const
{
    for (const UUserWidget* View = this; View; View = View->GetTypedOuter<UUserWidget>())
    {
        if (View->IsInViewport())
        {
            return true;
        }
    }
    return false;
}

bool UETGraphicsSettingsWidget::IsPreviewBusy() const
{
    return PreviewService.IsValid() &&
        PreviewService->GetSnapshot().Phase != EETVideoPreviewPhase::Idle;
}

bool UETGraphicsSettingsWidget::CanEdit() const
{
    const UWorld* World = GetWorld();
    return bViewActive && !bHostBusy && Draft.IsOpen() && Settings.IsValid() &&
        PreviewService.IsValid() && !IsPreviewBusy() && !GIsEditor && World &&
        !World->bIsTearingDown && World->WorldType == EWorldType::Game &&
        World->GetNetMode() == NM_Standalone;
}

void UETGraphicsSettingsWidget::SetHostBusy(const bool bBusy)
{
    bHostBusy = bBusy;
    Render();
}

void UETGraphicsSettingsWidget::ReadCurrent(const FText& Message)
{
    FETGraphicsPreferences Current;
    const bool bReady = Settings.IsValid() &&
        Settings->ReadGraphicsPreferences(Current) && Draft.Open(Current);
    if (!bReady)
    {
        Draft.Close();
    }
    Feedback = bReady ? Message : FText::FromString(TEXT("图形设置暂不可用。"));
    Render(true);
}

void UETGraphicsSettingsWidget::HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot)
{
    (void)Snapshot;
    if (bViewActive)
    {
        Render(); // 不重建下拉目录、不重读覆盖草稿、不抢视频确认按钮的焦点。
    }
}

void UETGraphicsSettingsWidget::Render(const bool bRebuildChoices)
{
    if (!bViewActive || !HasRequiredLayout())
    {
        return;
    }
    const TGuardValue<bool> SyncGuard(bSyncingControls, true);
    if (bRebuildChoices)
    {
        Draft.GetQualityLevels(QualityLevels);
        Draft.GetFrameRateLimits(FrameRateLimits);
        QualityCombo->ClearOptions();
        FrameRateCombo->ClearOptions();
        for (const int32 Level : QualityLevels)
        {
            QualityCombo->AddOption(ETGraphicsViewPrivate::QualityLabel(Level));
        }
        for (const float Limit : FrameRateLimits)
        {
            FrameRateCombo->AddOption(ETGraphicsViewPrivate::FrameRateLabel(Limit));
        }
    }
    const FETGraphicsPreferences& Value = Draft.GetValue();
    QualityCombo->SetSelectedIndex(QualityLevels.IndexOfByKey(Value.QualityLevel));
    FrameRateCombo->SetSelectedIndex(FrameRateLimits.IndexOfByPredicate([&Value](float Limit)
        { return FMath::IsNearlyEqual(Limit, Value.FrameRateLimit); }));
    VSyncCheckBox->SetIsChecked(Value.bUseVSync);

    FETGraphicsPreferences Applied;
    const bool bReady = Settings.IsValid() && Settings->ReadGraphicsPreferences(Applied);
    GraphicsCurrentText->SetText(bReady ? FText::FromString(FString::Printf(
        TEXT("当前配置：%s / %s / 垂直同步%s"),
        *ETGraphicsViewPrivate::QualityLabel(Applied.QualityLevel),
        *ETGraphicsViewPrivate::FrameRateLabel(Applied.FrameRateLimit),
        Applied.bUseVSync ? TEXT("开") : TEXT("关"))) :
        FText::FromString(TEXT("当前配置不可用")));

    FText Message = Feedback;
    if (GIsEditor)
    {
        Message = FText::FromString(TEXT("请在独立游戏窗口中调整图形设置。"));
    }
    else if (IsPreviewBusy())
    {
        Message = FText::FromString(TEXT("请先确认或取消显示预览；本卡草稿保留。"));
    }
    else if (bHostBusy)
    {
        Message = FText::FromString(TEXT("正在切换页面，请稍候。"));
    }
    GraphicsStatusText->SetText(Message);

    const bool bEdit = CanEdit();
    QualityCombo->SetIsEnabled(bEdit);
    FrameRateCombo->SetIsEnabled(bEdit);
    VSyncCheckBox->SetIsEnabled(bEdit);
    ApplyButton->SetIsEnabled(bEdit && Draft.IsDirty());
    RevertButton->SetIsEnabled(bEdit && Draft.IsDirty());
    DefaultsButton->SetIsEnabled(bEdit && !Draft.IsDefault());
}

void UETGraphicsSettingsWidget::FocusQuality()
{
    if (CanEdit() && HasLiveHost() && IsValid(GetOwningPlayer()))
    {
        QualityCombo->SetUserFocus(GetOwningPlayer());
    }
}

// ---- 编辑只改值副本；只在独立应用按钮中交给Core提交 ----

void UETGraphicsSettingsWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;
    const int32 Index = QualityCombo->GetSelectedIndex();
    if (!bSyncingControls && CanEdit() && QualityLevels.IsValidIndex(Index) &&
        Draft.SetQuality(QualityLevels[Index]))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETGraphicsSettingsWidget::HandleFrameRateChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;
    const int32 Index = FrameRateCombo->GetSelectedIndex();
    if (!bSyncingControls && CanEdit() && FrameRateLimits.IsValidIndex(Index) &&
        Draft.SetFrameRateLimit(FrameRateLimits[Index]))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETGraphicsSettingsWidget::HandleVSyncChanged(const bool bChecked)
{
    if (!bSyncingControls && CanEdit() && Draft.SetVSync(bChecked))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETGraphicsSettingsWidget::HandleApply()
{
    if (!CanEdit() || !HasLiveHost() || !Draft.IsDirty())
    {
        return;
    }
    UETGameUserSettings* Current = Settings.Get();
    FETGraphicsPreferences Applied;
    FETGraphicsPreferences Request;
    if (!Current->ReadGraphicsPreferences(Applied) || !Draft.TryMakeApply(Applied, Request))
    {
        ReadCurrent(FText::FromString(TEXT("配置已由其他入口更改，已刷新，请重新选择。")));
        FocusQuality();
        return;
    }

    const TStrongObjectPtr<UETGraphicsSettingsWidget> KeepAlive(this);
    if (!Current->ApplyGraphicsPreferences(Request, GetWorld()))
    {
        Feedback = FText::FromString(TEXT("暂不能应用，请确认显示预览已结束。"));
        Render();
        return;
    }
    UE_LOG(LogETGraphicsSettingsView, Display,
        TEXT("[M3-05B4-02] Applied Widget=%s Quality=%d Cap=%.2f VSync=%d"),
        *GetName(), Request.QualityLevel, Request.FrameRateLimit, Request.bUseVSync ? 1 : 0);
    if (bViewActive && HasLiveHost() && Settings.Get() == Current)
    {
        ReadCurrent(FText::FromString(TEXT("已应用并请求保存；实际帧率仍受性能和同步限制。")));
        FocusQuality();
    }
}

void UETGraphicsSettingsWidget::HandleRevert()
{
    if (CanEdit())
    {
        ReadCurrent(FText::FromString(TEXT("已取消修改，保留当前配置。")));
        FocusQuality();
    }
}

void UETGraphicsSettingsWidget::HandleDefaults()
{
    if (CanEdit())
    {
        Draft.UseDefaults();
        Feedback = FText::FromString(TEXT("草稿已填入中画质 / 60 FPS / 同步关，应用后生效。"));
        Render();
        FocusQuality();
    }
}

FReply UETGraphicsSettingsWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (HasRequiredLayout() && InKeyEvent.GetKey() == EKeys::Escape &&
        !QualityCombo->IsOpen() && !FrameRateCombo->IsOpen())
    {
        HandleRevert();
        return FReply::Handled();
    }
    // 展开列表先自行处理Esc；P继续冒泡给Pause，Enter / Space由焦点控件处理。
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}