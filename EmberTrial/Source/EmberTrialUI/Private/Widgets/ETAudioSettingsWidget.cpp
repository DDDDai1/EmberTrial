#include "Widgets/ETAudioSettingsWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Settings/ETAudioSubsystem.h"
#include "Settings/ETGameUserSettings.h"
#include "Settings/ETVideoSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAudioSettingsView, Log, All);

// ---- 控件初始化；Construct观察当前服务，Close先封口再移除两个句柄 ----

bool UETAudioSettingsWidget::HasRequiredLayout() const
{
    return IsValid(MasterSlider.Get()) && IsValid(MusicSlider.Get()) &&
        IsValid(EffectsSlider.Get()) && IsValid(MasterValueText.Get()) &&
        IsValid(MusicValueText.Get()) && IsValid(EffectsValueText.Get()) &&
        IsValid(ApplyButton.Get()) && IsValid(RevertButton.Get()) &&
        IsValid(DefaultsButton.Get()) && IsValid(AudioCurrentText.Get()) &&
        IsValid(AudioStatusText.Get());
}

void UETAudioSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    MasterSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMasterChanged);
    MusicSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMusicChanged);
    EffectsSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleEffectsChanged);
    ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleApply);
    RevertButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRevert);
    DefaultsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDefaults);
    for (USlider* Slider : { MasterSlider.Get(), MusicSlider.Get(), EffectsSlider.Get() })
    {
        Slider->SetMinValue(0.0f);
        Slider->SetMaxValue(1.0f);
        Slider->SetStepSize(0.05f);
    }
}

void UETAudioSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !HasRequiredLayout())
    {
        return;
    }
    CloseView();
    bViewActive = true;
    Settings = UETGameUserSettings::GetETGameUserSettings();
    if (UGameInstance* Instance = GetGameInstance())
    {
        AudioService = Instance->GetSubsystem<UETAudioSubsystem>();
        PreviewService = Instance->GetSubsystem<UETVideoSettingsSubsystem>();
    }
    if (UETAudioSubsystem* Audio = AudioService.Get())
    {
        RoutingChangedHandle = Audio->OnRoutingChanged.AddUObject(
            this, &ThisClass::HandleRoutingChanged);
    }
    if (UETVideoSettingsSubsystem* Preview = PreviewService.Get())
    {
        PreviewChangedHandle = Preview->OnChanged.AddUObject(
            this, &ThisClass::HandlePreviewSnapshot);
    }
    ReadCurrent(FText::FromString(TEXT("修改后选择应用；离开页面丢弃未应用草稿。")));
}

void UETAudioSettingsWidget::NativeDestruct()
{
    CloseView();
    Super::NativeDestruct();
}

void UETAudioSettingsWidget::CloseView()
{
    bViewActive = false;
    if (UETAudioSubsystem* Audio = AudioService.Get())
    {
        Audio->OnRoutingChanged.Remove(RoutingChangedHandle);
    }
    if (UETVideoSettingsSubsystem* Preview = PreviewService.Get())
    {
        Preview->OnChanged.Remove(PreviewChangedHandle);
    }
    RoutingChangedHandle.Reset();
    PreviewChangedHandle.Reset();
    AudioService.Reset();
    PreviewService.Reset();
    Settings.Reset();
    Draft.Close();
    Feedback = FText::GetEmpty();
}

// ---- 就绪 / 视频事件只刷新可用性，不重开草稿，不增加Ticker / Timer ----

bool UETAudioSettingsWidget::HasLiveHost() const
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

bool UETAudioSettingsWidget::IsPreviewBusy() const
{
    return PreviewService.IsValid() &&
        PreviewService->GetSnapshot().Phase != EETVideoPreviewPhase::Idle;
}

bool UETAudioSettingsWidget::CanEdit() const
{
    const UWorld* World = GetWorld();
    return bViewActive && !bHostBusy && Draft.IsOpen() && Settings.IsValid() &&
        AudioService.IsValid() && AudioService->IsRoutingApplied(World) &&
        PreviewService.IsValid() && !IsPreviewBusy() && !GIsEditor &&
        World && !World->bIsTearingDown && World->WorldType == EWorldType::Game &&
        World->GetNetMode() == NM_Standalone;
}

void UETAudioSettingsWidget::SetHostBusy(const bool bBusy)
{
    bHostBusy = bBusy;
    Render();
}

void UETAudioSettingsWidget::ReadCurrent(const FText& Message)
{
    const bool bReady = Settings.IsValid() && Draft.Open(Settings->GetAudioPreferences());
    if (!bReady)
    {
        Draft.Close();
    }
    Feedback = bReady ? Message : FText::FromString(TEXT("音量设置暂不可用。"));
    Render();
}

void UETAudioSettingsWidget::HandleRoutingChanged()
{
    Render(); // 首屏PostLoadMap后启用；不覆盖用户已编辑的草稿。
}

void UETAudioSettingsWidget::HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot)
{
    (void)Snapshot;
    Render();
}

void UETAudioSettingsWidget::Render()
{
    if (!bViewActive || !HasRequiredLayout())
    {
        return;
    }
    const TGuardValue<bool> SyncGuard(bSyncingControls, true);
    const FETAudioPreferences& Value = Draft.GetValue();
    MasterSlider->SetValue(Value.MasterVolume);
    MusicSlider->SetValue(Value.MusicVolume);
    EffectsSlider->SetValue(Value.EffectsVolume);
    FNumberFormattingOptions Number;
    Number.SetMaximumFractionalDigits(1);
    MasterValueText->SetText(FText::Format(NSLOCTEXT("ETAudioUI", "Master", "总音量：{0}"),
        FText::AsPercent(Value.MasterVolume, &Number)));
    MusicValueText->SetText(FText::Format(NSLOCTEXT("ETAudioUI", "Music", "音乐：{0}"),
        FText::AsPercent(Value.MusicVolume, &Number)));
    EffectsValueText->SetText(FText::Format(NSLOCTEXT("ETAudioUI", "Effects", "音效：{0}"),
        FText::AsPercent(Value.EffectsVolume, &Number)));
    if (Settings.IsValid())
    {
        const FETAudioPreferences Applied = Settings->GetAudioPreferences();
        AudioCurrentText->SetText(FText::Format(
            NSLOCTEXT("ETAudioUI", "Current", "当前：总 {0} / 音乐 {1} / 音效 {2}"),
            FText::AsPercent(Applied.MasterVolume, &Number),
            FText::AsPercent(Applied.MusicVolume, &Number),
            FText::AsPercent(Applied.EffectsVolume, &Number)));
    }
    FText Message = Feedback;
    if (GIsEditor)
    {
        Message = FText::FromString(TEXT("请在独立游戏窗口中调整音量。"));
    }
    else if (IsPreviewBusy())
    {
        Message = FText::FromString(TEXT("请先确认或取消显示预览；音量草稿保留。"));
    }
    else if (bHostBusy)
    {
        Message = FText::FromString(TEXT("正在切换页面，请稍候。"));
    }
    else if (!AudioService.IsValid() || !AudioService->IsRoutingApplied(GetWorld()))
    {
        Message = FText::FromString(TEXT("声音路由尚未就绪，请稍候。"));
    }
    AudioStatusText->SetText(Message);
    const bool bEdit = CanEdit();
    MasterSlider->SetIsEnabled(bEdit);
    MusicSlider->SetIsEnabled(bEdit);
    EffectsSlider->SetIsEnabled(bEdit);
    ApplyButton->SetIsEnabled(bEdit && Draft.IsDirty());
    RevertButton->SetIsEnabled(bEdit && Draft.IsDirty());
    DefaultsButton->SetIsEnabled(bEdit && !Draft.IsDefault());
}

// ---- 编辑只更新纯值；实际应用沿Core唯一提交入口 ----

void UETAudioSettingsWidget::UpdateDraft(const FETAudioPreferences& Request)
{
    if (!bSyncingControls && CanEdit() && Draft.SetValues(Request))
    {
        Feedback = FText::FromString(Draft.IsDirty() ?
            TEXT("有未应用的修改。") : TEXT("与当前设置一致。"));
        Render();
    }
}

void UETAudioSettingsWidget::HandleMasterChanged(const float Value)
{
    FETAudioPreferences Next = Draft.GetValue();
    Next.MasterVolume = Value;
    UpdateDraft(Next);
}

void UETAudioSettingsWidget::HandleMusicChanged(const float Value)
{
    FETAudioPreferences Next = Draft.GetValue();
    Next.MusicVolume = Value;
    UpdateDraft(Next);
}

void UETAudioSettingsWidget::HandleEffectsChanged(const float Value)
{
    FETAudioPreferences Next = Draft.GetValue();
    Next.EffectsVolume = Value;
    UpdateDraft(Next);
}

void UETAudioSettingsWidget::FocusMaster()
{
    if (CanEdit() && HasLiveHost() && IsValid(GetOwningPlayer()))
    {
        MasterSlider->SetUserFocus(GetOwningPlayer());
    }
}

void UETAudioSettingsWidget::HandleApply()
{
    if (!CanEdit() || !HasLiveHost() || !Draft.IsDirty())
    {
        return;
    }
    UETGameUserSettings* Current = Settings.Get();
    FETAudioPreferences Request;
    if (!Draft.TryMakeApply(Current->GetAudioPreferences(), Request))
    {
        ReadCurrent(FText::FromString(TEXT("音量已由其他入口更改，已刷新，请重新编辑。")));
        FocusMaster();
        return;
    }
    const TStrongObjectPtr<UETAudioSettingsWidget> KeepAlive(this);
    if (!Current->ApplyAudioPreferences(Request, GetWorld()))
    {
        Feedback = FText::FromString(TEXT("暂不能应用，请确认声音与显示预览状态。"));
        Render();
        return;
    }
    UE_LOG(LogETAudioSettingsView, Display,
        TEXT("[M3-05C3] Applied Widget=%s Master=%.3f Music=%.3f Effects=%.3f"),
        *GetName(), Request.MasterVolume, Request.MusicVolume, Request.EffectsVolume);
    if (bViewActive && HasLiveHost() && Settings.Get() == Current)
    {
        ReadCurrent(FText::FromString(TEXT("已应用并请求保存。")));
        FocusMaster();
    }
}

void UETAudioSettingsWidget::HandleRevert()
{
    if (CanEdit())
    {
        ReadCurrent(FText::FromString(TEXT("已取消修改，保留当前音量。")));
        FocusMaster();
    }
}

void UETAudioSettingsWidget::HandleDefaults()
{
    if (CanEdit())
    {
        Draft.UseDefaults();
        Feedback = FText::FromString(TEXT("草稿已恢复三项100%，选择应用后生效。"));
        Render();
        FocusMaster();
    }
}

FReply UETAudioSettingsWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        HandleRevert();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}