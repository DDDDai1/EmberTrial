// Fill out your copyright notice in the Description page of Project Settings.


#include "ETPracticeActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"


AETPracticeActor::AETPracticeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot.Get());

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot.Get());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PracticeLabel = NSLOCTEXT("EmberTrial", "PracticeLabel_Default", "Practice Actor");
}

void AETPracticeActor::PrintPracticeInfo()
{
	UE_LOG(LogTemp, Display, TEXT("[M0-03] Actor=%s Label=%s"), *GetName(), *PracticeLabel.ToString());
}

FText AETPracticeActor::GetPracticeLabel() const
{
	return PracticeLabel;
}

