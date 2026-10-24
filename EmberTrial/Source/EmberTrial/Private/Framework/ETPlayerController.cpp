#include "Framework/ETPlayerController.h"
#include "TargetLock/ETTargetLockComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Logging/LogMacros.h"
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Characters/EmberTrialCharacter.h"
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Resources/ETHealingResourceComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPlayerController, Log, All);

#if WITH_EDITOR
namespace
{
// PIE注入的保护范围，不是正式属性上限，也不改变GA的治疗量 / 体力成本。
constexpr float MaxStaminaDebugDelta = 100.0f;
constexpr float MaxHealingDebugAmount = 1000.0f;
} // namespace
#endif

// ---- 原生输入意图；按键释放与暂停内输入的路由规则保持 ----

AETPlayerController::AETPlayerController()
{
    TargetLockComponent = CreateDefaultSubobject<UETTargetLockComponent>(TEXT("TargetLock"));
}

UETTargetLockComponent* AETPlayerController::GetTargetLockComponent() const
{
    return TargetLockComponent.Get();
}

void AETPlayerController::HandleTargetLockStarted()
{
    if (IsValid(TargetLockComponent.Get()))
    {
        TargetLockComponent->ToggleTargetLock();
    }
}

void AETPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent)
    {
        FInputKeyBinding& PauseBinding =
            InputComponent->BindKey(EKeys::P, IE_Released, this, &ThisClass::HandlePauseReleased);
        PauseBinding.bConsumeInput = true;
        PauseBinding.bExecuteWhenPaused = false;

        UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
        if (EnhancedInput && IsValid(TargetLockAction.Get()) &&
            TargetLockAction->ValueType == EInputActionValueType::Boolean)
        {
            EnhancedInput->BindAction(TargetLockAction.Get(), ETriggerEvent::Started,
                                      this, &ThisClass::HandleTargetLockStarted);
        }
        else
        {
            UE_LOG(LogETPlayerController, Display,
                   TEXT("[M2-06A] Target lock input unavailable Controller=%s"), *GetName());
        }
    }
}

void AETPlayerController::HandlePauseReleased()
{
    if (IsLocalController() && GetLocalPlayer())
    {
        OnPauseRequested.Broadcast();
    }
}

// ---- 控制生命周期与世界首次输入恢复 ----

void AETPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController() && GetLocalPlayer())
    {
        FInputModeGameOnly GameInput;
        // 首次鼠标点击可以直接进入攻击；先清掉菜单遗留的按键状态。
        GameInput.SetConsumeCaptureMouseDown(false);
        SetInputMode(GameInput);
        bShowMouseCursor = false;
        FlushPressedKeys();
        UE_LOG(LogETPlayerController, Display, TEXT("[M1-06A] Game input ready Controller=%s"),
               *GetNameSafe(this));
    }
    UE_LOG(LogETPlayerController, Display,
           TEXT("[M1-02B] Controller BeginPlay Controller=%s Local=%d Pawn=%s"), *GetNameSafe(this),
           IsLocalController() ? 1 : 0, *GetNameSafe(GetPawn()));
}

void AETPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    UE_LOG(LogETPlayerController, Display, TEXT("[M1-02B] Possess Controller=%s Pawn=%s"),
           *GetNameSafe(this), *GetNameSafe(GetPawn()));
}

void AETPlayerController::OnUnPossess()
{
    // Super会改变当前Pawn；先保存旧身份，日志仍记录真实前后关系。
    const FString PreviousPawnName = GetNameSafe(GetPawn());
    Super::OnUnPossess();
    UE_LOG(LogETPlayerController, Display,
           TEXT("[M1-02B] UnPossess Controller=%s PreviousPawn=%s CurrentPawn=%s"),
           *GetNameSafe(this), *PreviousPawnName, *GetNameSafe(GetPawn()));
}

void AETPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogETPlayerController, Display,
           TEXT("[M1-02B] Controller EndPlay Controller=%s Reason=%d"), *GetNameSafe(this),
           static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}

// ---- PIE资源诊断：查询与注入分开；World / 身份 / 数值校验均保留 ----

void AETPlayerController::ET_Stamina(float Delta)
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(GetPawn());
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    if (!World || World->WorldType != EWorldType::PIE || !IsLocalController() || !IsValid(Body) ||
        !Body->HasAuthority() || !FMath::IsFinite(Delta) ||
        FMath::Abs(Delta) > MaxStaminaDebugDelta ||
        !Body->GetGameplayStaminaSnapshot(Before, MaxStamina))
    {
        UE_LOG(LogETPlayerController, Display,
               TEXT("[M2-01B1] Debug rejected Controller=%s Reason=ContextOrValue"),
               *GetNameSafe(this));
        return;
    }
    // 暂停 / 死亡允许0只读；非零注入仍被拒绝，不触发补执行。
    if (Delta != 0.0f && (World->IsPaused() || Body->IsCombatDead()))
    {
        UE_LOG(LogETPlayerController, Display,
               TEXT("[M2-01B1] Debug rejected Controller=%s Reason=PausedOrDead"),
               *GetNameSafe(this));
        return;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (Delta != 0.0f)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UETStaminaDeltaEffect::StaticClass(),
                                                               1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid())
        {
            return;
        }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Delta);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
    float After = 0.0f;
    if (!Body->GetGameplayStaminaSnapshot(After, MaxStamina))
    {
        return;
    }
    const UETStaminaRecoveryComponent* Recovery =
        Body->FindComponentByClass<UETStaminaRecoveryComponent>();
    UE_LOG(LogETPlayerController, Display,
           TEXT("[M2-01B1] Debug Owner=%s Delta=%.1f Before=%.1f After=%.1f Max=%.1f Time=%.3f "
                "Timer=%d Paused=%d"),
           *GetNameSafe(Body), Delta, Before, After, MaxStamina, World->GetTimeSeconds(),
           IsValid(Recovery) && Recovery->IsRecoveryScheduled() ? 1 : 0, World->IsPaused() ? 1 : 0);
#else
    (void)Delta;
#endif
}

void AETPlayerController::ET_Healing(float Amount)
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(GetPawn());
    UETHealingResourceComponent* Healing =
        IsValid(Body) ? Body->FindComponentByClass<UETHealingResourceComponent>() : nullptr;
    if (!World || World->WorldType != EWorldType::PIE || !IsLocalController() || !IsValid(Body) ||
        !IsValid(Healing) || !FMath::IsFinite(Amount) || Amount < 0.0f ||
        Amount > MaxHealingDebugAmount)
    {
        UE_LOG(LogETPlayerController, Display,
               TEXT("[M2-04A] Debug rejected Reason=ContextOrValue"));
        return;
    }
    float Before = 0.0f;
    float MaxHealth = 0.0f;
    Body->GetGameplayHealthSnapshot(Before, MaxHealth);
    // 正式Q由GA / Notify提交；这里只复用资源服务，不另写扣次数规则。
    const bool bApplied =
        Amount > 0.0f && Body->IsFormalActionIdle() && Healing->TryApplyHealing(Amount);
    int32 Charges = 0;
    int32 MaxCharges = 0;
    const bool bAvailable = Healing->GetHealingSnapshot(Charges, MaxCharges);
    float After = 0.0f;
    Body->GetGameplayHealthSnapshot(After, MaxHealth);
    UE_LOG(LogETPlayerController, Display,
           TEXT("[M2-04A] Debug Owner=%s Available=%d Amount=%.1f Applied=%d "
                "Health=%.1f->%.1f/%.1f Charges=%d/%d Paused=%d"),
           *GetNameSafe(Body), bAvailable ? 1 : 0, Amount, bApplied ? 1 : 0, Before, After,
           MaxHealth, Charges, MaxCharges, World->IsPaused() ? 1 : 0);
#else
    (void)Amount;
#endif
}
