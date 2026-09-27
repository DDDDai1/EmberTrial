// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETPracticeActionComponent.generated.h"

UENUM(BlueprintType)
enum class EETPracticeActionState : uint8
{
	Idle,
	Attacking,
	Dodging,
	HitReact
};

UCLASS( ClassGroup=(EmberTrial), meta=(BlueprintSpawnableComponent) )
class EMBERTRIAL_API UETPracticeActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UETPracticeActionComponent();

	UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
	EETPracticeActionState GetPracticeActionState() const;

	UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
	bool IsPracticeActionIdle() const;

protected:
	
	virtual void BeginPlay() override;

public:	
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "M0|Practice Action", meta = (AllowPrivateAccess = "true"))
	EETPracticeActionState CurrentState = EETPracticeActionState::Idle;

};
