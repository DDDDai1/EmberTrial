#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Save/ETRunSaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "ETRunSessionSubsystem.generated.h"

class AEmberTrialCharacter;
class AETTrialExit;
class APlayerStart;
class UWorld;
struct FETSaveResult;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETRunTravelFailed, UWorld*, const FText&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FETRunCheckpointChanged, UWorld*, FName);
DECLARE_MULTICAST_DELEGATE_OneParam(FETTrialCompleted, UWorld*);

enum class EETRunTravelIntent : uint8
{
    None,
    NewRun,
    Retry,
    Continue,
    MainMenu
};

// 只有本GI、目标地图与出生报告确认后才发布；失败请求不发布成功。
DECLARE_MULTICAST_DELEGATE_TwoParams(FETRunTravelConfirmed, UWorld*, EETRunTravelIntent);

// 会话负责地图、检查点与快照采集，存档子系统负责写盘。
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
    bool ProbeContinueSave(UWorld* MenuWorld, const FSoftObjectPath& ExpectedMap,
                           TFunction<void(bool, const FText&)> Completion);
    bool IsSavedEncounterCleared(UWorld* World, FName EncounterId, bool& bOutAuthoritative) const;
    int32 GetCompletedEncounterCount(const UWorld* World) const;
    bool SetCheckpointIdForDevelopmentTest(UWorld* TestWorld, FName TestId);

    // ---- 单次非无缝切图意图；接受请求不等于已经到达 / 提交快照 ----
    bool RequestNewRun(UWorld* RequestWorld, const FSoftObjectPath& Map, FText& OutError);
    bool RequestRetry(UWorld* RequestWorld, FText& OutError);
    bool RequestReplay(UWorld* RequestWorld, FText& OutError);
    bool RequestContinue(UWorld* RequestWorld, FText& OutError);
    bool RequestMainMenu(UWorld* RequestWorld, const FSoftObjectPath& Map, FText& OutError);
    bool IsTravelPending() const
    {
        return bTravelPending;
    }

    FETRunTravelFailed OnTravelFailed;
    FETRunCheckpointChanged OnCheckpointChanged;
    FETRunTravelConfirmed OnTravelConfirmed;
    FETTrialCompleted OnTrialCompleted;

private:
    friend class FETRelicEquipmentTransactionTest;
    friend class FETEncounterCheckpointResumeTest;
    friend class FETTrialVictoryTest;
    friend class AETTrialExit;

    bool TryCompleteTrial(UWorld* World, AEmberTrialCharacter* Player, int32 ClearedEncounters);
    void CaptureCheckpointSave(UWorld& World, const AEmberTrialCharacter& Body);
    void SubmitCheckpointSave();
    void HandleCheckpointSaveFinished(const FETSaveResult& Result);
    void QueueSaveRetirement(UWorld& World);
    bool SubmitSaveRetirement();
    void HandleSaveRetirementFinished(const FETSaveResult& Result);
    bool ApplyDiskRestore(UWorld& World, const FETRunSaveSnapshot& Snapshot, FName SpawnId);

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
    TWeakObjectPtr<UWorld> CompletedWorld;
    int32 CompletedEncounterCount = 0;
    bool bContinueSaveProbed = false;
    TOptional<FETRunSaveSnapshot> OfferedSave;
    TOptional<FETRunSaveSnapshot> PendingDiskRestore;
    TOptional<TArray<FName>> RestoredClearedIds;

    struct FPendingCheckpointSave
    {
        FETRunSaveSnapshot Snapshot;
        bool bEditorPreview = false;
    };
    // Keep the latest activation if another checkpoint write is still in flight.
    TOptional<FPendingCheckpointSave> PendingCheckpointSave;
    bool bCheckpointSaveInFlight = false;
    struct FPendingSaveRetirement
    {
        FString MapPackage;
        bool bEditorPreview = false;
    };
    TOptional<FPendingSaveRetirement> PendingSaveRetirement;
    bool bSaveRetirementInFlight = false;
    bool bRetireSaveOnNewRun = false;

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
