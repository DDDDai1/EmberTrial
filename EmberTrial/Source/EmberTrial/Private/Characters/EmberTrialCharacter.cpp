// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/PlayerController.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETPoiseDeltaEffect.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayAbilitySpec.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Combat/ETCombatPoiseComponent.h"
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Inventory/ETRelicEquipmentComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

DEFINE_LOG_CATEGORY_STATIC(LogETAbilityFoundation, Log, All);
// ---- 原生子对象与模板默认值；不在这里硬引用内容资产 ----

AEmberTrialCharacter::AEmberTrialCharacter()
{
    // 子对象名称沿用已有蓝图身份；重复创建会破坏单ASC / 单资源契约。
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystemComponent->SetIsReplicated(false);

    HealthAttributes = CreateDefaultSubobject<UETHealthAttributeSet>(TEXT("HealthAttributes"));
    StaminaAttributes = CreateDefaultSubobject<UETStaminaAttributeSet>(TEXT("StaminaAttributes"));

    CombatTraceComponent = CreateDefaultSubobject<UETCombatTraceComponent>(TEXT("CombatTrace"));

    CombatLifeComponent = CreateDefaultSubobject<UETCombatLifeComponent>(TEXT("CombatLife"));

    CombatPoiseComponent = CreateDefaultSubobject<UETCombatPoiseComponent>(TEXT("CombatPoise"));

    StaminaRecoveryComponent =
        CreateDefaultSubobject<UETStaminaRecoveryComponent>(TEXT("StaminaRecovery"));

    PoiseAttributes = CreateDefaultSubobject<UETPoiseAttributeSet>(TEXT("PoiseAttributes"));
    CombatAttributes = CreateDefaultSubobject<UETCombatAttributeSet>(TEXT("CombatAttributes"));

    HealingResourceComponent =
        CreateDefaultSubobject<UETHealingResourceComponent>(TEXT("HealingResource"));

    RelicEquipmentComponent =
        CreateDefaultSubobject<UETRelicEquipmentComponent>(TEXT("RelicEquipment"));

    // 以下为既有模板配置，均可在组件 / Blueprint中覆盖；本次保留数值。
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement =
        true; // Character moves in the direction of input...
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength =
        400.0f; // The camera follows at this distance behind the character
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(
        CameraBoom,
        USpringArmComponent::
            SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

// ---- 启动：先建立ActorInfo和属性Ready，再依次尝试授予能力 ----
void AEmberTrialCharacter::BeginPlay()
{
    bAbilitySystemShuttingDown = false;
    Super::BeginPlay();
    PracticeActionComponent = FindComponentByClass<UETPracticeActionComponent>();
    RefreshEmberAbilityActorInfo(TEXT("BeginPlay"));
    EnsureGameplayInitialized();
    GrantLightAttackAbility();
    GrantDodgeAbility();
    GrantHealAbility();
    GrantBreakPulseAbility();
}

UAbilitySystemComponent* AEmberTrialCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent.Get();
}

// ---- 正式闪避：玩家专属；活动请求拒绝，不保存补执行输入 ----
void AEmberTrialCharacter::GrantDodgeAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        CombatTeam != EETCombatTeam::Player || !DodgeAbilityClass || DodgeAbilityHandle.IsValid())
    {
        return;
    }
    if (DodgeAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETAbilityFoundation, Warning, TEXT("[M2-02A] Abstract Dodge class Body=%s"),
               *GetNameSafe(this));
        return;
    }
    DodgeAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(DodgeAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-02A] Dodge granted Body=%s Valid=%d"),
           *GetNameSafe(this), DodgeAbilityHandle.IsValid() ? 1 : 0);
}

bool AEmberTrialCharacter::RequestDodge()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !DodgeAbilityHandle.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M2-02A] Dodge request Body=%s Accepted=0 Reason=NotReadyOrNotGranted"),
               *GetNameSafe(this));
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(DodgeAbilityHandle);
    if (!Spec || Spec->IsActive())
    {
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M2-02A] Dodge request Body=%s Accepted=0 Reason=MissingOrBusy"),
               *GetNameSafe(this));
        return false;
    }
    const bool bAccepted = AbilitySystemComponent->TryActivateAbility(DodgeAbilityHandle, false);
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-02A] Dodge request Body=%s Accepted=%d"),
           *GetNameSafe(this), bAccepted ? 1 : 0);
    return bAccepted;
}

// ---- 方向只在请求时采样；位移期间的固定方向由AbilityTask拥有 ----
FVector AEmberTrialCharacter::MakeDodgeWorldDirection(const FVector2D& Axis, float ControlYaw,
                                                      const FVector& Forward)
{
    if (!FMath::IsFinite(Axis.X) || !FMath::IsFinite(Axis.Y) || !FMath::IsFinite(ControlYaw) ||
        Forward.ContainsNaN())
    {
        return FVector::ZeroVector;
    }
    if (Axis.SizeSquared() <= FMath::Square(0.1f))
    {
        return Forward.GetSafeNormal2D();
    }
    const FRotationMatrix Yaw(FRotator(0.0f, ControlYaw, 0.0f));
    return (Yaw.GetUnitAxis(EAxis::X) * Axis.Y + Yaw.GetUnitAxis(EAxis::Y) * Axis.X)
        .GetSafeNormal2D();
}

FVector AEmberTrialCharacter::GetDodgeWorldDirection() const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const UEnhancedPlayerInput* Input = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    FVector2D Axis = FVector2D::ZeroVector;
    if (IsValid(Input) && IsValid(MoveAction) &&
        MoveAction->ValueType == EInputActionValueType::Axis2D)
    {
        Axis = Input->GetActionValue(MoveAction).Get<FVector2D>();
    }
    const float Yaw = PC ? PC->GetControlRotation().Yaw : GetActorRotation().Yaw;
    return MakeDodgeWorldDirection(Axis, Yaw, GetActorForwardVector());
}

// ---- 轻攻击：玩家连段与敌人单段共用授予入口 ----
void AEmberTrialCharacter::GrantLightAttackAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        !LightAttackAbilityClass || LightAttackAbilityHandle.IsValid())
    {
        return;
    }
    if (LightAttackAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETAbilityFoundation, Error,
               TEXT("[M1-04A] Invalid abstract LightAttackAbilityClass Character=%s"),
               *GetNameSafe(this));
        return;
    }

    const FGameplayAbilitySpec Spec(LightAttackAbilityClass.Get(), 1, INDEX_NONE, this);
    LightAttackAbilityHandle = AbilitySystemComponent->GiveAbility(Spec);
    if (!LightAttackAbilityHandle.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Error, TEXT("[M1-04A] Grant failed Character=%s Class=%s"),
               *GetNameSafe(this), *GetNameSafe(LightAttackAbilityClass.Get()));
        return;
    }
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M1-04A] Granted Character=%s Class=%s Handle=%s"), *GetNameSafe(this),
           *GetNameSafe(LightAttackAbilityClass.Get()), *LightAttackAbilityHandle.ToString());
}

bool AEmberTrialCharacter::RequestLightAttack()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !LightAttackAbilityHandle.IsValid() ||
        AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) ||
         AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse))
    {
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M1-04A] Request rejected Character=%s Reason=NotReadyOrNotGranted"),
               *GetNameSafe(this));
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(LightAttackAbilityHandle);
    if (!Spec)
    {
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M1-04A] Request rejected Character=%s Reason=MissingSpecOrBusy"),
               *GetNameSafe(this));
        return false;
    }

    // 只有活动轻攻击允许把新按键交给连段缓冲；闪避 / 治疗不采用此规则。
    if (Spec->IsActive())
    {
        UETGA_LightAttack* Attack = Cast<UETGA_LightAttack>(Spec->GetPrimaryInstance());
        const bool bBuffered = IsValid(Attack) && Attack->TryBufferComboInput();
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M2-03B] Active attack input Character=%s Accepted=%d"), *GetNameSafe(this),
               bBuffered ? 1 : 0);
        return bBuffered;
    }

    const bool bAccepted =
        AbilitySystemComponent->TryActivateAbility(LightAttackAbilityHandle, false);
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M1-04A] Request Character=%s Accepted=%d"),
           *GetNameSafe(this), bAccepted ? 1 : 0);
    return bAccepted;
}

FGameplayAbilitySpecHandle AEmberTrialCharacter::GetLightAttackSpecHandle() const
{
    return !bAbilitySystemShuttingDown && IsGameplayReady() && !IsCombatDead() &&
                   IsValid(AbilitySystemComponent)
               ? LightAttackAbilityHandle
               : FGameplayAbilitySpecHandle();
}

void AEmberTrialCharacter::CancelLightAttack()
{
    if (IsValid(AbilitySystemComponent) && LightAttackAbilityHandle.IsValid())
    {
        AbilitySystemComponent->CancelAbilityHandle(LightAttackAbilityHandle);
    }
}

// ---- 阵营 / 受击 / 死亡与正式空闲查询 ----
EETCombatTeam AEmberTrialCharacter::GetCombatTeam() const
{
    return CombatTeam;
}

bool AEmberTrialCharacter::CanDealCombatDamageTo(const AEmberTrialCharacter* TargetBody) const
{
    if (!IsValid(TargetBody) || TargetBody == this || IsActorBeingDestroyed() ||
        TargetBody->IsActorBeingDestroyed())
    {
        return false;
    }

    switch (CombatTeam)
    {
    case EETCombatTeam::Player:
        return TargetBody->GetCombatTeam() == EETCombatTeam::Enemy ||
               TargetBody->GetCombatTeam() == EETCombatTeam::Neutral;
    case EETCombatTeam::Enemy:
        return TargetBody->GetCombatTeam() == EETCombatTeam::Player;
    default:
        return false;
    }
}

bool AEmberTrialCharacter::RequestHitReact()
{
    return !bAbilitySystemShuttingDown && IsGameplayReady() && !IsCombatDead() &&
           IsValid(CombatLifeComponent) && CombatLifeComponent->RequestHitReact();
}

bool AEmberTrialCharacter::IsCombatDead() const
{
    return IsValid(CombatLifeComponent) && CombatLifeComponent->IsDead();
}

// Ready表示四组属性初始化完成；零资源、失去控制或Stunned不单独撤销它。
bool AEmberTrialCharacter::IsGameplayReady() const
{
    return bGameplayReady && bStartupAttributesInitialized && !bAbilitySystemShuttingDown &&
           IsValid(AbilitySystemComponent) && IsValid(HealthAttributes) &&
           IsValid(StaminaAttributes) && IsValid(PoiseAttributes) && IsValid(CombatAttributes);
}

bool AEmberTrialCharacter::RequestStagger()
{
    return IsValid(CombatPoiseComponent) && CombatPoiseComponent->RequestStagger();
}

void AEmberTrialCharacter::ET_Poise(float Delta)
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    float Poise = 0.0f;
    float MaxPoise = 0.0f;
    if (!IsValid(World) || World->WorldType != EWorldType::PIE || !HasAuthority() ||
        !FMath::IsFinite(Delta) || Delta > 0.0f || Delta < -100000.0f ||
        !GetGameplayPoiseSnapshot(Poise, MaxPoise) || UGameplayStatics::IsGamePaused(this))
    {
        return;
    }
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    bool bApplied = false;
    if (Delta < 0.0f && IsValid(ASC) && !IsCombatDead() &&
        !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead) &&
        !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) &&
        !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable))
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
            UETPoiseDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (Spec.IsValid())
        {
            Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_PoiseDelta, Delta);
            bApplied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied();
        }
    }
    GetGameplayPoiseSnapshot(Poise, MaxPoise);
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-05B2] Poise dev Body=%s Delta=%.1f Applied=%d Poise=%.1f/%.1f"),
           *GetNameSafe(this), Delta, bApplied ? 1 : 0, Poise, MaxPoise);
#else
    (void)Delta;
#endif
}

bool AEmberTrialCharacter::IsFormalActionIdle() const
{
    return !bAbilitySystemShuttingDown && IsGameplayReady() && !IsCombatDead() &&
           IsValid(AbilitySystemComponent) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Dead) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_HitReact) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Healing) &&
               !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse);
}

// ---- 资源快照：不可用时先清空输出，不把启动配置当作当前属性 ----
bool AEmberTrialCharacter::GetGameplayHealthSnapshot(float& OutHealth, float& OutMaxHealth) const
{
    OutHealth = 0.0f;
    OutMaxHealth = 0.0f;
    if (!IsGameplayReady())
    {
        return false;
    }

    OutHealth = HealthAttributes->GetHealth();
    OutMaxHealth = HealthAttributes->GetMaxHealth();
    return true;
}

bool AEmberTrialCharacter::GetGameplayStaminaSnapshot(float& OutStamina, float& OutMaxStamina) const
{
    OutStamina = 0.0f;
    OutMaxStamina = 0.0f;
    if (!IsGameplayReady())
    {
        return false;
    }

    OutStamina = StaminaAttributes->GetStamina();
    OutMaxStamina = StaminaAttributes->GetMaxStamina();
    return true;
}

bool AEmberTrialCharacter::GetGameplayPoiseSnapshot(float& OutPoise, float& OutMaxPoise) const
{
    OutPoise = 0.0f;
    OutMaxPoise = 0.0f;
    if (!IsGameplayReady())
    {
        return false;
    }

    OutPoise = PoiseAttributes->GetPoise();
    OutMaxPoise = PoiseAttributes->GetMaxPoise();
    return true;
}

// ---- 一次初始化与Ready通知；先提交状态，再允许同步观察者查询 ----
bool AEmberTrialCharacter::GetGameplayLightAttackBonusSnapshot(float& OutBonus) const
{
    OutBonus = 0.0f;
    if (!IsGameplayReady() || !IsValid(CombatAttributes) ||
        AbilitySystemComponent->GetSet<UETCombatAttributeSet>() != CombatAttributes.Get())
    {
        return false;
    }
    const float Bonus = CombatAttributes->GetLightAttackBonus();
    if (!FMath::IsFinite(Bonus))
    {
        return false;
    }
    OutBonus = Bonus;
    return true;
}

void AEmberTrialCharacter::SetGameplayReady(bool bNewReady)
{
    if (bGameplayReady == bNewReady)
    {
        return;
    }

    bGameplayReady = bNewReady;
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M1-03A2-01] Ready changed Character=%s Ready=%d"), *GetNameSafe(this),
           bGameplayReady ? 1 : 0);
    OnGameplayReadyChanged.Broadcast(this, bGameplayReady);
}

bool AEmberTrialCharacter::EnsureGameplayInitialized()
{
    if (!HasActorBegunPlay() || bAbilitySystemShuttingDown || IsActorBeingDestroyed())
    {
        return false;
    }

    if (!IsValid(AbilitySystemComponent) || !AbilitySystemComponent->AbilityActorInfo.IsValid() ||
        AbilitySystemComponent->AbilityActorInfo->OwnerActor.Get() != this ||
        AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get() != this ||
        !IsValid(HealthAttributes) || !IsValid(StaminaAttributes) || !IsValid(PoiseAttributes) ||
        !IsValid(CombatAttributes) ||
        AbilitySystemComponent->GetSet<UETHealthAttributeSet>() != HealthAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETStaminaAttributeSet>() != StaminaAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETPoiseAttributeSet>() != PoiseAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETCombatAttributeSet>() != CombatAttributes.Get())
    {
        SetGameplayReady(false);
        UE_LOG(LogETAbilityFoundation, Error,
               TEXT("[M2-01A] Missing gameplay dependencies Character=%s"), *GetNameSafe(this));
        return false;
    }

    // 已成功初始化就只读；重复Ensure或换控都不补血 / 体力 / 韧性。
    if (bStartupAttributesInitialized)
    {
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M2-01A] Already initialized Character=%s Health=%.1f/%.1f Stamina=%.1f/%.1f "
                    "Poise=%.1f/%.1f"),
               *GetNameSafe(this), HealthAttributes->GetHealth(), HealthAttributes->GetMaxHealth(),
               StaminaAttributes->GetStamina(), StaminaAttributes->GetMaxStamina(),
               PoiseAttributes->GetPoise(), PoiseAttributes->GetMaxPoise());
        return IsGameplayReady();
    }

    // 三组可配初值全通过后才写入；Combat加成基础固定0，不开放重复启动配置。
    if (!FMath::IsFinite(InitialMaxHealth) || !FMath::IsFinite(InitialHealth) ||
        InitialMaxHealth < 1.0f || InitialHealth < 0.0f || InitialHealth > InitialMaxHealth)
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M1-03A2-01] Invalid startup health Character=%s Health=%.1f MaxHealth=%.1f"),
               *GetNameSafe(this), InitialHealth, InitialMaxHealth);
        return false;
    }

    if (!FMath::IsFinite(InitialMaxStamina) || !FMath::IsFinite(InitialStamina) ||
        InitialMaxStamina < 1.0f || InitialStamina < 0.0f || InitialStamina > InitialMaxStamina)
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M2-01A] Invalid startup stamina Character=%s Stamina=%.1f MaxStamina=%.1f"),
               *GetNameSafe(this), InitialStamina, InitialMaxStamina);
        return false;
    }

    if (!FMath::IsFinite(InitialMaxPoise) || !FMath::IsFinite(InitialPoise) ||
        InitialMaxPoise < 1.0f || InitialPoise < 0.0f || InitialPoise > InitialMaxPoise)
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M2-05A] Invalid startup poise Character=%s Poise=%.1f MaxPoise=%.1f"),
               *GetNameSafe(this), InitialPoise, InitialMaxPoise);
        return false;
    }

    // 每组先上限后当前值；基础资源与Combat写入后才发布Ready。
    AbilitySystemComponent->SetNumericAttributeBase(UETHealthAttributeSet::GetMaxHealthAttribute(),
                                                    InitialMaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(),
                                                    InitialHealth);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETStaminaAttributeSet::GetMaxStaminaAttribute(), InitialMaxStamina);
    AbilitySystemComponent->SetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute(),
                                                    InitialStamina);
    AbilitySystemComponent->SetNumericAttributeBase(UETPoiseAttributeSet::GetMaxPoiseAttribute(),
                                                    InitialMaxPoise);
    AbilitySystemComponent->SetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute(),
                                                    InitialPoise);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetLightAttackBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;

    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M1-03A2-01] Health initialized Character=%s Health=%.1f MaxHealth=%.1f"),
           *GetNameSafe(this), HealthAttributes->GetHealth(), HealthAttributes->GetMaxHealth());
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-01A] Stamina initialized Character=%s Stamina=%.1f MaxStamina=%.1f"),
           *GetNameSafe(this), StaminaAttributes->GetStamina(), StaminaAttributes->GetMaxStamina());
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-05A] Poise initialized Character=%s Poise=%.1f MaxPoise=%.1f"),
           *GetNameSafe(this), PoiseAttributes->GetPoise(), PoiseAttributes->GetMaxPoise());
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M3-02B] Combat initialized Character=%s LightAttackBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus());
    SetGameplayReady(true);
    return IsGameplayReady();
}

// ---- ASC始终归身体；控制变化只刷新关联，不重建ASC或重置资源 ----
void AEmberTrialCharacter::RefreshEmberAbilityActorInfo(const TCHAR* Reason)
{
    if (bAbilitySystemShuttingDown || IsActorBeingDestroyed())
    {
        return;
    }

    if (!IsValid(AbilitySystemComponent) || !AbilitySystemComponent->AbilityActorInfo.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Error,
               TEXT("[M1-03A1] Missing ASC or ActorInfo Character=%s Reason=%s"),
               *GetNameSafe(this), Reason);
        return;
    }

    const bool bNeedsInit = AbilitySystemComponent->AbilityActorInfo->OwnerActor.Get() != this ||
                            AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get() != this;

    if (bNeedsInit)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
    else
    {
        AbilitySystemComponent->RefreshAbilityActorInfo();
    }

    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M1-03A1] ActorInfo ready Character=%s Reason=%s Mode=%s ASC=%s Owner=%s "
                "Avatar=%s Controller=%s"),
           *GetNameSafe(this), Reason, bNeedsInit ? TEXT("Init") : TEXT("Refresh"),
           *GetNameSafe(AbilitySystemComponent.Get()),
           *GetNameSafe(AbilitySystemComponent->GetOwnerActor()),
           *GetNameSafe(AbilitySystemComponent->GetAvatarActor()),
           *GetNameSafe(AbilitySystemComponent->AbilityActorInfo->PlayerController.Get()));
}

void AEmberTrialCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (HasActorBegunPlay())
    {
        RefreshEmberAbilityActorInfo(TEXT("PossessedBy"));
    }
}

void AEmberTrialCharacter::UnPossessed()
{
    Super::UnPossessed();
    if (HasActorBegunPlay())
    {
        RefreshEmberAbilityActorInfo(TEXT("UnPossessed"));
    }
}

// ---- 结束：先封锁重入 / 撤销Ready，再取消能力与清关联，最后交还Super ----
void AEmberTrialCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bAbilitySystemShuttingDown = true;
    SetGameplayReady(false);

    if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->AbilityActorInfo.IsValid())
    {
        // 身体正在退出，取消本ASC的能力；随后只清除Character自己保存的授予句柄。
        AbilitySystemComponent->CancelAllAbilities();
        if (LightAttackAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(LightAttackAbilityHandle);
        }
        if (DodgeAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(DodgeAbilityHandle);
        }
        if (HealAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(HealAbilityHandle);
        }
        if (BreakPulseAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(BreakPulseAbilityHandle);
        }
        AbilitySystemComponent->RemoveAllGameplayCues();
        AbilitySystemComponent->ClearActorInfo();

        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M1-03A1] ActorInfo cleared Character=%s Reason=%d"), *GetNameSafe(this),
               static_cast<int32>(EndPlayReason));
    }
    LightAttackAbilityHandle = FGameplayAbilitySpecHandle();
    DodgeAbilityHandle = FGameplayAbilitySpecHandle();
    HealAbilityHandle = FGameplayAbilitySpecHandle();
    BreakPulseAbilityHandle = FGameplayAbilitySpecHandle();
    Super::EndPlay(EndPlayReason);
}

// ---- 输入绑定与兼容门控；镜头不受正式动作Idle限制 ----

void AEmberTrialCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                    PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent =
            Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {

        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this,
                                           &AEmberTrialCharacter::StartPracticeAwareJump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
                                           &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,
                                           &AEmberTrialCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,
                                           &AEmberTrialCharacter::Look);

        if (IsValid(BreakPulseAction))
        {
            EnhancedInputComponent->BindAction(BreakPulseAction, ETriggerEvent::Started, this,
                &AEmberTrialCharacter::HandleBreakPulseStarted);
        }
        if (IsValid(HealAction))
        {
            EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started, this,
                                               &AEmberTrialCharacter::HandleHealStarted);
        }
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error,
               TEXT("'%s' Failed to find an Enhanced Input component! This template is built to "
                    "use the Enhanced Input system. If you intend to use the legacy system, then "
                    "you will need to update this C++ file."),
               *GetNameSafe(this));
    }
}

// 移动与跳跃共用相同短路检查；正式状态通过后才查询旧Practice状态。
bool AEmberTrialCharacter::CanAcceptLocomotionInput() const
{
    return IsFormalActionIdle() &&
           (!IsValid(PracticeActionComponent) || PracticeActionComponent->IsPracticeActionIdle());
}

void AEmberTrialCharacter::Move(const FInputActionValue& Value)
{
    if (!CanAcceptLocomotionInput())
    {
        return;
    }

    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // get right vector
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AEmberTrialCharacter::StartPracticeAwareJump()
{
    if (!CanAcceptLocomotionInput())
    {
        return;
    }

    Jump();
}

void AEmberTrialCharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// ---- 治疗：玩家专属，效果 / 次数事务由GA与资源组件负责 ----
void AEmberTrialCharacter::GrantHealAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        CombatTeam != EETCombatTeam::Player || !HealAbilityClass || HealAbilityHandle.IsValid())
    {
        return;
    }
    if (HealAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETAbilityFoundation, Warning, TEXT("[M2-04B2] Abstract Heal class Body=%s"),
               *GetNameSafe(this));
        return;
    }
    HealAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(HealAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-04B2] Heal granted Body=%s Valid=%d"),
           *GetNameSafe(this), HealAbilityHandle.IsValid() ? 1 : 0);
}

bool AEmberTrialCharacter::RequestHeal()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !IsValid(AbilitySystemComponent) || !HealAbilityHandle.IsValid())
    {
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(HealAbilityHandle);
    if (!Spec || Spec->IsActive())
    {
        return false;
    }
    const bool bAccepted = AbilitySystemComponent->TryActivateAbility(HealAbilityHandle, false);
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-04B2] Heal request Body=%s Accepted=%d"),
           *GetNameSafe(this), bAccepted ? 1 : 0);
    return bAccepted;
}

void AEmberTrialCharacter::HandleHealStarted()
{
    RequestHeal();
}

// ---- 破势请求与冷却快照日志；不在输入层播放动画或修改Poise ----
void AEmberTrialCharacter::GrantBreakPulseAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        CombatTeam != EETCombatTeam::Player || !BreakPulseAbilityClass ||
        BreakPulseAbilityHandle.IsValid())
    {
        return;
    }
    if (BreakPulseAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        return;
    }
    BreakPulseAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(BreakPulseAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-05C2-01] Break granted Body=%s Valid=%d"),
           *GetNameSafe(this), BreakPulseAbilityHandle.IsValid() ? 1 : 0);
}

bool AEmberTrialCharacter::RequestBreakPulse()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !IsValid(AbilitySystemComponent) || !BreakPulseAbilityHandle.IsValid())
    {
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(BreakPulseAbilityHandle);
    if (!Spec || !Spec->Ability || Spec->Ability->GetClass() != BreakPulseAbilityClass.Get())
    {
        return false;
    }
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = AbilitySystemComponent;
    const FString BodyName = GetName();
    const FGameplayAbilitySpecHandle Handle = BreakPulseAbilityHandle;
    const bool bActivated = AbilitySystemComponent->TryActivateAbility(Handle, false);
    float Remaining = 0.0f;
    float Duration = 0.0f;
    UAbilitySystemComponent* ASC = WeakASC.Get();
    Spec = IsValid(ASC) ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
    const bool bAccepted = bActivated && Spec && Spec->IsActive();
    const UETGA_BreakPulse* Ability = Spec ? Cast<UETGA_BreakPulse>(Spec->Ability) : nullptr;
    if (Ability && ASC->AbilityActorInfo.IsValid())
    {
        Ability->GetCooldownTimeRemainingAndDuration(Handle, ASC->AbilityActorInfo.Get(), Remaining,
                                                     Duration);
    }
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-05C2-01] Break request Body=%s Accepted=%d Remaining=%.2f Duration=%.2f"),
           *BodyName, bAccepted ? 1 : 0, Remaining, Duration);
    return bAccepted;
}

void AEmberTrialCharacter::HandleBreakPulseStarted()
{
    RequestBreakPulse();
}

bool AEmberTrialCharacter::GetBreakPulseCooldownSnapshot(bool& bOutCooling, float& OutRemaining,
                                                         float& OutDuration) const
{
    bOutCooling = false;
    OutRemaining = 0.0f;
    OutDuration = 0.0f;
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (bAbilitySystemShuttingDown || !IsGameplayReady() || CombatTeam != EETCombatTeam::Player ||
        !IsValid(ASC) || !BreakPulseAbilityHandle.IsValid() || !ASC->AbilityActorInfo.IsValid() ||
        ASC->AbilityActorInfo->OwnerActor.Get() != this ||
        ASC->AbilityActorInfo->AvatarActor.Get() != this)
    {
        return false;
    }
    const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(BreakPulseAbilityHandle);
    const UETGA_BreakPulse* Ability = Spec ? Cast<UETGA_BreakPulse>(Spec->Ability) : nullptr;
    if (!IsValid(Ability) || Ability->GetClass() != BreakPulseAbilityClass.Get())
    {
        return false;
    }
    if (!ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse))
    {
        return true;
    }
    float Remaining = 0.0f;
    float Duration = 0.0f;
    Ability->GetCooldownTimeRemainingAndDuration(BreakPulseAbilityHandle,
                                                 ASC->AbilityActorInfo.Get(), Remaining, Duration);
    if (!FMath::IsFinite(Remaining) || !FMath::IsFinite(Duration) || Duration <= 0.0f)
    {
        return false;
    }
    // 到期Tag尚未撤销的极短窗口仍属于Cooling，不能仅按Remaining==0显示就绪。
    bOutCooling = true;
    OutDuration = Duration;
    OutRemaining = FMath::Clamp(Remaining, 0.0f, Duration);
    return true;
}