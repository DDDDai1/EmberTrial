#include "Interaction/ETRelicAltarAccess.h"
#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Kismet/GameplayStatics.h"

TUniquePtr<FETRelicAltarAccess> FETRelicAltarAccess::TryOpen(
    APlayerController* Controller, AETRelicAltar* Altar)
{
    // 必须先以原来的非暂停资格打开；不能在普通P页临时制造许可。
    AEmberTrialCharacter* Body = IsValid(Controller)
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    UWorld* World = IsValid(Controller) ? Controller->GetWorld() : nullptr;
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UETRunSessionSubsystem* Session = IsValid(GI)
        ? GI->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!IsValid(World) || World->bIsTearingDown || !IsValid(Controller) ||
        Controller->IsActorBeingDestroyed() || !IsValid(Altar) ||
        Altar->GetWorld() != World || !Altar->CanUseAltar(Body) || Body->GetController() != Controller ||
        !IsValid(Session) || Session->IsTravelPending())
    {
        return nullptr;
    }

    // 私有构造只能在此调用；MakeUnique模板本身没有私有构造访问权。
    TUniquePtr<FETRelicAltarAccess> Access(new FETRelicAltarAccess());
    Access->BoundWorld = World;
    Access->BoundController = Controller;
    Access->BoundBody = Body;
    Access->BoundAltar = Altar;
    Access->BoundSession = Session;
    Access->bOpen = true;
    return Access;
}

bool FETRelicAltarAccess::CanCommitFor(const UETRelicEquipmentComponent* Equipment) const
{
    UWorld* World = BoundWorld.Get();
    APlayerController* Controller = BoundController.Get();
    AEmberTrialCharacter* Body = BoundBody.Get();
    AETRelicAltar* Altar = BoundAltar.Get();
    UETRunSessionSubsystem* Session = BoundSession.Get();
    if (!bOpen || !IsValid(Equipment) || !IsValid(World) || World->bIsTearingDown ||
        !IsValid(Controller) || Controller->IsActorBeingDestroyed() ||
        !IsValid(Body) || !IsValid(Altar) || !IsValid(Session))
    {
        return false;
    }

    // 每次确认都重新读当前身份、距离 / 高差 / Enabled / Visibility与会话。
    // 只把“暂停检查”换成已打开页面的契约，其他运行态资格保持相同。
    return Controller->GetWorld() == World && Body->GetWorld() == World &&
        Altar->GetWorld() == World && Controller->GetPawn() == Body &&
        Body->GetController() == Controller && Equipment->GetOwner() == Body &&
        Equipment->GetWorld() == World && Session->GetGameInstance() == World->GetGameInstance() &&
        !Session->IsTravelPending() && UGameplayStatics::IsGamePaused(Body) &&
        UETRelicAltarSubsystem::IsEligibleBody(Body) && Altar->IsInReach(Body);
}

void FETRelicAltarAccess::Close()
{
    // Close后不可重新发放同一对象；新页面必须经TryOpen创建新上下文。
    bOpen = false;
    BoundSession.Reset();
    BoundAltar.Reset();
    BoundBody.Reset();
    BoundController.Reset();
    BoundWorld.Reset();
}