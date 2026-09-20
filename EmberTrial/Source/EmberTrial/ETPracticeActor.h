// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETPracticeActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class EMBERTRIAL_API AETPracticeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AETPracticeActor();

	UFUNCTION(BlueprintCallable, Category = "Practice")
	void PrintPracticeInfo();

	UFUNCTION(BlueprintPure, Category = "Practice")
	FText GetPracticeLabel() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Practice", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Practice", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice", meta = (AllowPrivateAccess = "true"))
	FText PracticeLabel;

};
