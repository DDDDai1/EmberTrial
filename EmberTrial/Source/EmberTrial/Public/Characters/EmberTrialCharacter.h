// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameplayAbilitySpecHandle.h"
#include "EmberTrialCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UETPracticeActionComponent;
class UAbilitySystemComponent;
class UETHealthAttributeSet;
class AEmberTrialCharacter;
class UETGA_LightAttack;
class UETGA_Dodge;
class UETGA_Heal;
class UETGA_BreakPulse;
class UETCombatTraceComponent;
class UETCombatLifeComponent;
class UETCombatPoiseComponent;
class UETStaminaAttributeSet;
class UETPoiseAttributeSet;
class UETStaminaRecoveryComponent;
class UETHealingResourceComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FETGameplayReadyChangedSignature,
                                             AEmberTrialCharacter*, Character, bool, bReady);

UENUM(BlueprintType)
enum class EETCombatTeam : uint8
{
    Neutral UMETA(DisplayName = "Neutral"),
    Player UMETA(DisplayName = "Player"),
    Enemy UMETA(DisplayName = "Enemy")
};

// 玩家 / 敌人 / 训练身体共享ASC与资源；输入、能力及UI保留各自职责。
UCLASS(config = Game)
class EMBERTRIAL_API AEmberTrialCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

    // ---- 摄像机与输入资产；沿用模板默认值，Blueprint可配置覆盖 ----
    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera,
              meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera,
              meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input,
              meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input,
              meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input,
              meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input,
              meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction;

    // ---- 兼容原练习动作；移动 / 跳跃共用门控，不另建动作状态副本 ----
    UPROPERTY(Transient)
    TObjectPtr<UETPracticeActionComponent> PracticeActionComponent;

    bool CanAcceptLocomotionInput() const;

    // ---- 本身体拥有的ASC与资源子对象；名称是既有资产引用契约 ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Abilities",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    // EndPlay先置位，阻止取消 / UnPossess的同步回调重新建立关联。
    bool bAbilitySystemShuttingDown = false;

    void RefreshEmberAbilityActorInfo(const TCHAR* Reason);

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Health")
    TObjectPtr<UETHealthAttributeSet> HealthAttributes;

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Stamina")
    TObjectPtr<UETStaminaAttributeSet> StaminaAttributes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Stamina",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETStaminaRecoveryComponent> StaminaRecoveryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Healing",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETHealingResourceComponent> HealingResourceComponent;

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Poise")
    TObjectPtr<UETPoiseAttributeSet> PoiseAttributes;

    // ---- 出生配置；不是运行值副本，同身体换控不会再次写入 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "1.0"))
    float InitialMaxHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "0.0"))
    float InitialHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "1.0"))
    float InitialMaxStamina = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "0.0"))
    float InitialStamina = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "1.0"))
    float InitialMaxPoise = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "0.0"))
    float InitialPoise = 100.0f;

    // 自动化夹具只访问既有启动配置 / 句柄，不增加生产测试开关。
    friend class FETStaminaFoundationTest;
    friend class FETPoiseFoundationTest;
    friend class FETStaggerAbilityTest;
    friend class FETDodgeMovementTest;
    friend class FETDodgeInvulnerabilityTest;
    friend class FETLightComboFlowTest;
    friend class FETLightComboCancelTest;
    friend class FETHealingResourceTest;
    friend class FETHealingAbilityTest;
    friend class FETPoiseReactionTest;
    friend class FETBreakPulseAbilityTest;
    friend class FETBreakPulseRangeTest;

    // Ready只表示三组属性链初始化成功；不代表存活、受控或能力已授予。
    bool bStartupAttributesInitialized = false;
    bool bGameplayReady = false;

    // ---- 各能力独立持有授予句柄；玩家专属与敌人共享规则保持分离 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Abilities")
    TSubclassOf<UETGA_LightAttack> LightAttackAbilityClass;

    FGameplayAbilitySpecHandle LightAttackAbilityHandle;

    void GrantLightAttackAbility();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Abilities")
    TSubclassOf<UETGA_Dodge> DodgeAbilityClass;

    FGameplayAbilitySpecHandle DodgeAbilityHandle;
    void GrantDodgeAbility();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing")
    TSubclassOf<UETGA_Heal> HealAbilityClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing")
    TObjectPtr<UInputAction> HealAction;

    FGameplayAbilitySpecHandle HealAbilityHandle;

    void GrantHealAbility();

    void HandleHealStarted();

    // ---- 破势施放：玩家专属，只由原生Started入口请求 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TSubclassOf<UETGA_BreakPulse> BreakPulseAbilityClass;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TObjectPtr<UInputAction> BreakPulseAction;
    FGameplayAbilitySpecHandle BreakPulseAbilityHandle;
    void GrantBreakPulseAbility();
    void HandleBreakPulseStarted();

    void SetGameplayReady(bool bNewReady);

    // ---- 战斗组件与阵营；伤害 / 受击 / 死亡不放进输入处理 ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETCombatTraceComponent> CombatTraceComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETCombatLifeComponent> CombatLifeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Poise",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETCombatPoiseComponent> CombatPoiseComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    EETCombatTeam CombatTeam = EETCombatTeam::Neutral;

public:
    // ---- 只读快照 / 就绪事件与正式动作命令 ----
    AEmberTrialCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool IsGameplayReady() const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool GetGameplayHealthSnapshot(float& OutHealth, float& OutMaxHealth) const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool GetGameplayStaminaSnapshot(float& OutStamina, float& OutMaxStamina) const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool GetGameplayPoiseSnapshot(float& OutPoise, float& OutMaxPoise) const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Gameplay")
    bool EnsureGameplayInitialized();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestLightAttack();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestDodge();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Healing")
    bool RequestHeal();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestBreakPulse();

    // 输入轴按镜头Yaw转换；低于既有死区时使用身体前方。
    FVector GetDodgeWorldDirection() const;
    static FVector MakeDodgeWorldDirection(const FVector2D& Axis, float ControlYaw,
                                           const FVector& Forward);

    // AI以授予句柄核对自己的请求；活动攻击仍可查询，不按Idle过滤。
    FGameplayAbilitySpecHandle GetLightAttackSpecHandle() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    void CancelLightAttack();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    EETCombatTeam GetCombatTeam() const;

    bool CanDealCombatDamageTo(const AEmberTrialCharacter* TargetBody) const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestHitReact();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestStagger();

    // 开发期只在PIE接受；0查询、负数测试真实Poise GE，不是正式技能入口。
    UFUNCTION(Exec)
    void ET_Poise(float Delta = 0.0f);

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    bool IsFormalActionIdle() const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    bool IsCombatDead() const;

    // 先提交Ready状态再广播；观察者仍需订阅后立即读取快照。
    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Gameplay")
    FETGameplayReadyChangedSignature OnGameplayReadyChanged;

protected:
    /** Called for movement input */
    void Move(const FInputActionValue& Value);
    void StartPracticeAwareJump();

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    // ---- Pawn输入绑定与世界 / 控制权生命周期 ----
    // APawn interface
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // To add mapping context
    virtual void BeginPlay() override;

    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const
    {
        return CameraBoom;
    }
    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent* GetFollowCamera() const
    {
        return FollowCamera;
    }
};
