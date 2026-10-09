#include "Combat/ETCombatTraceComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatTrace, Log, All);

UETCombatTraceComponent::UETCombatTraceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bTickEvenWhenPaused = false;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    bAutoActivate = true;
}

void UETCombatTraceComponent::BeginPlay()
{
    Super::BeginPlay();
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        SourceMesh = Character->GetMesh();
        if (SourceMesh.IsValid())
        {
            AddTickPrerequisiteComponent(SourceMesh.Get());
        }
    }
    SetComponentTickEnabled(false);
}

bool UETCombatTraceComponent::CanStartAttack() const
{
    const USkeletalMeshComponent* Mesh = SourceMesh.Get();
    return IsActive() && IsValid(GetOwner()) && IsValid(Mesh) &&
        ActiveSerial == 0 && Mesh->GetBoneIndex(TraceBoneName) != INDEX_NONE &&
        !TraceLocalOffset.ContainsNaN() &&
        FMath::IsFinite(TraceRadius) && TraceRadius >= 1.0f &&
        FMath::IsFinite(MaxSampleTravel) && MaxSampleTravel >= 1.0f;
}

bool UETCombatTraceComponent::IsExpectedMontageActive() const
{
    const USkeletalMeshComponent* Mesh = SourceMesh.Get();
    const UAnimInstance* Anim = IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr;
    const FAnimMontageInstance* Instance = IsValid(Anim) && ExpectedMontage.IsValid()
        ? Anim->GetActiveInstanceForMontage(ExpectedMontage.Get()) : nullptr;
    return Instance && Instance->GetInstanceID() == ActiveMontageInstanceID;
}

uint32 UETCombatTraceComponent::BeginAttack(UAnimMontage* Montage,
    int32 MontageInstanceID)
{
    if (!CanStartAttack() || !IsValid(Montage) || MontageInstanceID == INDEX_NONE)
    {
        return 0;
    }
    ExpectedMontage = Montage;
    ActiveMontageInstanceID = MontageInstanceID;
    if (!IsExpectedMontageActive())
    {
        ExpectedMontage.Reset();
        ActiveMontageInstanceID = INDEX_NONE;
        return 0;
    }
    ++SerialCounter;
    if (SerialCounter == 0)
    {
        ++SerialCounter;
    }
    ActiveSerial = SerialCounter;
    SeenTargets.Reset();
    WindowCount = 0;
    SweepCount = 0;
    bBlockedByWorld = false;
    bHasPreviousPoint = false;
    bWindowOpen = false;
    ActiveWindow.Reset();
    SetComponentTickEnabled(false);
    UE_LOG(LogETCombatTrace, Display,
        TEXT("[M1-04B1] Arm Owner=%s Serial=%u MontageID=%d"),
        *GetNameSafe(GetOwner()), ActiveSerial, ActiveMontageInstanceID);
    return ActiveSerial;
}

void UETCombatTraceComponent::SetAttackWindow(USkeletalMeshComponent* Mesh,
    UAnimSequenceBase* Animation, int32 MontageInstanceID,
    UAnimNotifyState* WindowToken, bool bOpen)
{
    if (ActiveSerial == 0 || Mesh != SourceMesh.Get() ||
        Animation != ExpectedMontage.Get() ||
        MontageInstanceID != ActiveMontageInstanceID || !IsValid(WindowToken))
    {
        return;
    }
    if (!bOpen)
    {
        if (bWindowOpen && ActiveWindow.Get() == WindowToken)
        {
            CloseWindow(TEXT("NotifyEnd"));
        }
        return;
    }
    if (bBlockedByWorld || !IsExpectedMontageActive())
    {
        return;
    }
    if (bWindowOpen)
    {
        UE_LOG(LogETCombatTrace, Warning,
            TEXT("[M1-04B1] Overlapping window ignored Owner=%s Serial=%u"),
            *GetNameSafe(GetOwner()), ActiveSerial);
        return;
    }
    ActiveWindow = WindowToken;
    bWindowOpen = true;
    bHasPreviousPoint = false;
    ++WindowCount;
    SetComponentTickEnabled(true);
    UE_LOG(LogETCombatTrace, Display,
        TEXT("[M1-04B1] WindowOpen Owner=%s Serial=%u MontageID=%d Window=%d"),
        *GetNameSafe(GetOwner()), ActiveSerial, ActiveMontageInstanceID, WindowCount);
}

void UETCombatTraceComponent::CloseWindow(const TCHAR* Reason)
{
    if (bWindowOpen)
    {
        UE_LOG(LogETCombatTrace, Display,
            TEXT("[M1-04B1] WindowClose Owner=%s Serial=%u Reason=%s"),
            *GetNameSafe(GetOwner()), ActiveSerial, Reason);
    }
    bWindowOpen = false;
    bHasPreviousPoint = false;
    ActiveWindow.Reset();
    SetComponentTickEnabled(false);
}

void UETCombatTraceComponent::BlockAttack(const FHitResult& Hit,
    const TCHAR* Reason)
{
    bBlockedByWorld = true;
    UE_LOG(LogETCombatTrace, Display,
        TEXT("[M1-04B1] Blocked Owner=%s Serial=%u Actor=%s Reason=%s"),
        *GetNameSafe(GetOwner()), ActiveSerial, *GetNameSafe(Hit.GetActor()), Reason);
#if ENABLE_DRAW_DEBUG
    if (bDrawDebug)
    {
        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 8.0f, 8,
            FColor::Red, false, 0.35f);
    }
#endif
    CloseWindow(Reason);
}

void UETCombatTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bWindowOpen || ActiveSerial == 0 || UGameplayStatics::IsGamePaused(this))
    {
        return;
    }
    UWorld* World = GetWorld();
    USkeletalMeshComponent* Mesh = SourceMesh.Get();
    if (!IsValid(World) || !IsValid(Mesh) || !IsValid(GetOwner()) ||
        !IsExpectedMontageActive() || Mesh->GetBoneIndex(TraceBoneName) == INDEX_NONE)
    {
        CloseWindow(TEXT("SourceLost"));
        return;
    }

    const FVector Point = Mesh->GetSocketTransform(TraceBoneName, RTS_World)
        .TransformPosition(TraceLocalOffset);
    if (Point.ContainsNaN())
    {
        CloseWindow(TEXT("InvalidPoint"));
        return;
    }
    if (bHasPreviousPoint &&
        FVector::DistSquared(PreviousPoint, Point) > FMath::Square(MaxSampleTravel))
    {
        CloseWindow(TEXT("SampleJump"));
        return;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETAttackTrace), false, GetOwner());
    Params.bFindInitialOverlaps = true;
    FHitResult OcclusionHit;
    if (World->LineTraceSingleByChannel(OcclusionHit,
        GetOwner()->GetActorLocation(), Point, ECC_Visibility, Params))
    {
        BlockAttack(OcclusionHit, TEXT("BodyToHandBlocked"));
        return;
    }

    const FVector Start = bHasPreviousPoint ? PreviousPoint : Point;
    PreviousPoint = Point;
    bHasPreviousPoint = true;
    TArray<FHitResult> Hits;
    World->SweepMultiByChannel(Hits, Start, Point, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(TraceRadius), Params);
    ++SweepCount;
#if ENABLE_DRAW_DEBUG
    if (bDrawDebug)
    {
        DrawDebugLine(World, Start, Point, FColor::Yellow, false, 0.12f, 0, 1.0f);
        DrawDebugSphere(World, Start, TraceRadius, 12, FColor::Yellow, false, 0.12f);
        DrawDebugSphere(World, Point, TraceRadius, 12, FColor::Yellow, false, 0.12f);
    }
#endif

    const uint32 SampleSerial = ActiveSerial;
    for (const FHitResult& Hit : Hits)
    {
        APawn* Target = Cast<APawn>(Hit.GetActor());
        if (IsValid(Target) && Target != GetOwner())
        {
            const TWeakObjectPtr<APawn> Key(Target);
            if (!SeenTargets.Contains(Key))
            {
                SeenTargets.Add(Key);
                UE_LOG(LogETCombatTrace, Display,
                    TEXT("[M1-04B1] Candidate Owner=%s Serial=%u Target=%s"),
                    *GetNameSafe(GetOwner()), SampleSerial, *GetNameSafe(Target));
                OnTraceHit.Broadcast(SampleSerial, Hit);
                // Future damage callbacks may cancel the source ability or destroy a body.
                if (ActiveSerial != SampleSerial || !bWindowOpen || !IsValid(GetOwner()))
                {
                    return;
                }
            }
        }
        if (Hit.bBlockingHit)
        {
            BlockAttack(Hit, TEXT("SweepBlocked"));
            return;
        }
    }
}

void UETCombatTraceComponent::CancelActiveAttack()
{
    EndAttack(ActiveSerial);
    CloseWindow(TEXT("OwnerCancelled"));
}

void UETCombatTraceComponent::EndAttack(uint32 Serial)
{
    if (Serial == 0 || Serial != ActiveSerial)
    {
        return;
    }
    CloseWindow(TEXT("AttackEnd"));
    UE_LOG(LogETCombatTrace, Display,
        TEXT("[M1-04B1] Disarm Owner=%s Serial=%u Windows=%d Sweeps=%d Unique=%d Blocked=%d Tick=0"),
        *GetNameSafe(GetOwner()), ActiveSerial, WindowCount, SweepCount,
        SeenTargets.Num(), bBlockedByWorld ? 1 : 0);
    ActiveSerial = 0;
    ActiveMontageInstanceID = INDEX_NONE;
    ExpectedMontage.Reset();
    SeenTargets.Reset();
}

void UETCombatTraceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndAttack(ActiveSerial);
    OnTraceHit.Clear();
    SetComponentTickEnabled(false);
    if (SourceMesh.IsValid())
    {
        RemoveTickPrerequisiteComponent(SourceMesh.Get());
    }
    SourceMesh.Reset();
    Super::EndPlay(EndPlayReason);
}