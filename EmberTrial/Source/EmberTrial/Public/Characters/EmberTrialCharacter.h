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
class UETCombatTraceComponent;
class UETCombatLifeComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FETGameplayReadyChangedSignature, AEmberTrialCharacter*, Character, bool, bReady);

UENUM(BlueprintType)
enum class EETCombatTeam : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Player  UMETA(DisplayName = "Player"),
	Enemy   UMETA(DisplayName = "Enemy")
};

UCLASS(config=Game)
class EMBERTRIAL_API AEmberTrialCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UETPracticeActionComponent> PracticeActionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Abilities",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	bool bAbilitySystemShuttingDown = false;

	void RefreshEmberAbilityActorInfo(const TCHAR* Reason);

	UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Health")
	TObjectPtr<UETHealthAttributeSet> HealthAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "1.0"))
	float InitialMaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "0.0"))
	float InitialHealth = 100.0f;

	bool bStartupAttributesInitialized = false;
	bool bGameplayReady = false;

	UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Abilities")
	TSubclassOf<UETGA_LightAttack> LightAttackAbilityClass;

	FGameplayAbilitySpecHandle LightAttackAbilityHandle;

	void GrantLightAttackAbility();

	void SetGameplayReady(bool bNewReady);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Combat",
	meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UETCombatTraceComponent> CombatTraceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Combat",
	meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UETCombatLifeComponent> CombatLifeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EmberTrial|Combat",
	meta = (AllowPrivateAccess = "true"))
	EETCombatTeam CombatTeam = EETCombatTeam::Neutral;

public:
	AEmberTrialCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
	bool IsGameplayReady() const;

	UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
	bool GetGameplayHealthSnapshot(float& OutHealth, float& OutMaxHealth) const;

	UFUNCTION(BlueprintCallable, Category = "EmberTrial|Gameplay")
	bool EnsureGameplayInitialized();

	UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
	bool RequestLightAttack();

	FGameplayAbilitySpecHandle GetLightAttackSpecHandle() const;

	UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
	void CancelLightAttack();

	UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
	EETCombatTeam GetCombatTeam() const;

	bool CanDealCombatDamageTo(const AEmberTrialCharacter* TargetBody) const;

	UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
	bool RequestHitReact();

	UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
	bool IsFormalActionIdle() const;

	UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
	bool IsCombatDead() const;

	UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Gameplay")
	FETGameplayReadyChangedSignature OnGameplayReadyChanged;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	void StartPracticeAwareJump();

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

