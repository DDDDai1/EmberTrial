#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ETCombatTraceComponent.generated.h"

class APawn;
class UAnimMontage;
class UAnimSequenceBase;
class UAnimNotifyState;
class USkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETCombatTraceHitSignature, uint32, const FHitResult&);

// 身体独有的近战采样状态；只广播候选，不决定阵营、伤害或资源。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatTraceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatTraceComponent();

    // ---- 攻击登记；0表示未武装，Serial与Montage实例共同隔离旧通知 ----
    bool CanStartAttack() const;
    uint32 BeginAttack(UAnimMontage* Montage, int32 MontageInstanceID);
    void EndAttack(uint32 Serial);

    void CancelActiveAttack();

    // Notify资产只转发身份；开关状态与每次攻击的目标集合由本组件持有。
    void SetAttackWindow(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                         int32 MontageInstanceID, UAnimNotifyState* WindowToken, bool bOpen);

    // 同步回调可能取消攻击或销毁身体；分发后必须重验当前Serial / 窗口。
    FETCombatTraceHitSignature OnTraceHit;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ---- 蓝图可调采样；世界遮挡与调试绘制沿用既有规则 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace")
    FName TraceBoneName = TEXT("hand_r");

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace")
    FVector TraceLocalOffset = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace",
              meta = (ClampMin = "1.0", Units = "cm"))
    float TraceRadius = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace",
              meta = (ClampMin = "1.0", Units = "cm"))
    float MaxSampleTravel = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace")
    bool bDrawDebug = true;

    // ---- 每次攻击独立状态；窗口关闭不清SeenTargets，结束才释放 ----
    TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
    TWeakObjectPtr<UAnimMontage> ExpectedMontage;
    TWeakObjectPtr<UAnimNotifyState> ActiveWindow;
    TSet<TWeakObjectPtr<APawn>> SeenTargets;

    uint32 SerialCounter = 0;
    uint32 ActiveSerial = 0;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    int32 WindowCount = 0;
    int32 SweepCount = 0;
    bool bWindowOpen = false;
    bool bHasPreviousPoint = false;
    bool bBlockedByWorld = false;
    FVector PreviousPoint = FVector::ZeroVector;

    // ---- 采样结果分发与统一关闭路径 ----
    bool IsExpectedMontageActive() const;
    void ProcessHitResults(const TArray<FHitResult>& Hits);
    void CloseWindow(const TCHAR* Reason);
    void BlockAttack(const FHitResult& Hit, const TCHAR* Reason);
};
