#include "ETPracticeActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

// 原生网格默认无碰撞、无Tick；蓝图可另装配交互组件。
AETPracticeActor::AETPracticeActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot.Get());

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot.Get());
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 本地化Namespace / Key是既有文本身份，不作为任意字符串改名。
    PracticeLabel = NSLOCTEXT("EmberTrial", "PracticeLabel_Default", "Practice Actor");
}

// 保留历史诊断入口与日志文本；C++无调用不能证明蓝图未使用。
void AETPracticeActor::PrintPracticeInfo()
{
    UE_LOG(LogTemp, Display, TEXT("[M0-03] Actor=%s Label=%s"), *GetName(),
           *PracticeLabel.ToString());
}

FText AETPracticeActor::GetPracticeLabel() const
{
    return PracticeLabel;
}
