#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "ETRunSessionSubsystem.generated.h"

class AEmberTrialCharacter;
class APlayerStart;
class UWorld;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETRunTravelFailed, UWorld*, const FText&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FETRunCheckpointChanged, UWorld*, FName);

enum class EETRunTravelIntent : uint8
{
    None,
    NewRun,
    Retry,
    Continue,
    MainMenu
};

// 当前进程的地图 / 检查点快照；不保存Actor / ASC，不负责Widget或磁盘存档。
UCLASS()
class EMBERTRIAL_API UETRunSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ---- GI生命周期与全局引擎委托；解绑只处理本实例持有的Handle ----
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- 稳定出生ID与已确认快照；Entry名称属于地图装配契约 ----
    static FName GetEntryCheckpointId()
    {
        return FName(TEXT("Entry"));
    }
    FName GetSpawnCheckpointId(UWorld* SpawnWorld) const;
    void NotifyGameplayWorldReady(UWorld* GameplayWorld, FName SpawnId);
    bool TryActivateCheckpoint(UWorld* RequestWorld, APlayerStart* CheckpointStart,
                               AEmberTrialCharacter* PlayerBody);
    bool IsCheckpointActive(UWorld* CheckWorld, FName CheckpointId) const;
    bool HasContinueSnapshot() const;
    bool SetCheckpointIdForDevelopmentTest(UWorld* TestWorld, FName TestId);

    // ---- 单次非无缝切图意图；接受请求不等于已经到达 / 提交快照 ----
    bool RequestNewRun(UWorld* RequestWorld, const FSoftObjectPath& Map, FText& OutError);
    bool RequestRetry(UWorld* RequestWorld, FText& OutError);
    bool RequestContinue(UWorld* RequestWorld, FText& OutError);
    bool RequestMainMenu(UWorld* RequestWorld, const FSoftObjectPath& Map, FText& OutError);
    bool IsTravelPending() const
    {
        return bTravelPending;
    }

    FETRunTravelFailed OnTravelFailed;
    FETRunCheckpointChanged OnCheckpointChanged;

private:
    // ---- 切图预检、到达确认、失败释放与PIE包名归一化 ----
    bool BeginTravel(UWorld* RequestWorld, const FString& TargetPackage, FName SpawnId,
                     EETRunTravelIntent Intent, FText& OutError);
    bool RejectRequest(const TCHAR* Reason, FText& OutError);
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void HandleTravelFailure(UWorld* FailedWorld, ETravelFailure::Type FailureType,
                             const FString& ErrorString);
    void ResetPendingTravel();
    static FString GetWorldPackage(UWorld* World);
    static bool IsGameplayIntent(EETRunTravelIntent Intent);
    static const TCHAR* GetIntentName(EETRunTravelIntent Intent);

    // 已确认快照跨菜单保留；只有本进程有效，失败请求不覆盖它。
    FString GameplayPackage;
    FName ActiveCheckpointId = NAME_None;
    // 待定请求与实际出生报告，确认到达后才提交为快照。
    FString PendingTargetPackage;
    FName PendingCheckpointId = NAME_None;
    EETRunTravelIntent PendingIntent = EETRunTravelIntent::None;
    // 只在请求期间弱引用来源世界，不阻止旧地图回收。
    TWeakObjectPtr<UWorld> OriginWorld;
    // 引擎全局事件可能来自别的PIE实例，回调必须核对所属GI。
    FDelegateHandle PostLoadHandle;
    FDelegateHandle TravelFailureHandle;
    bool bPendingGameplayReady = false;
    bool bTravelPending = false;
    bool bShuttingDown = false;
};
