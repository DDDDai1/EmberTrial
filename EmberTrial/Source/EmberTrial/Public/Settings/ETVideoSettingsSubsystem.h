#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETVideoSettingsSubsystem.generated.h"

class UGameViewportClient;
class UETGameUserSettings;
class UWorld;
class SWindow;

DECLARE_MULTICAST_DELEGATE_OneParam(FETVideoPreviewChanged, const FETVideoPreviewSnapshot&);

/** 单GI视频预览适配；不拥有UI / 暂停，不把候选写入持久配置。 */
UCLASS()
class EMBERTRIAL_API UETVideoSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool GetActualMode(FETVideoMode& Out) const;
    bool GetAvailableResolutions(EETVideoWindowMode Mode, TArray<FIntPoint>& Out) const;
    uint64 BeginPreview(UObject* Owner, const FETVideoMode& Request);
    bool ConfirmPreview(UObject* Owner, uint64 ExpectedSerial);
    void CancelPreview(UObject* Owner, uint64 ExpectedSerial);

    FETVideoPreviewSnapshot GetSnapshot() const { return Transaction.GetSnapshot(); }
    double GetRemainingSeconds() const;
    FETVideoPreviewChanged OnChanged;

private:
    bool IsStandaloneContext() const;
    bool IsLocalOwner(const UObject* Owner) const;
    bool ReadViewport(UGameViewportClient* Viewport, FETVideoMode& Out) const;
    bool ReadBoundActual(FETVideoMode& Out) const;
    bool NeedsTick() const;
    void EnsureTicker();
    bool TickPreview(float DeltaSeconds);
    void Execute(const FETVideoPreviewCommand& Command);
    void CancelInternal();
    void Publish();
    void ReleaseContext();
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

    FETVideoPreviewTransaction Transaction;
    TWeakObjectPtr<UObject> PreviewOwner;
    TWeakObjectPtr<UWorld> PreviewWorld;
    TWeakObjectPtr<UGameViewportClient> BoundViewport;
    TWeakObjectPtr<UETGameUserSettings> BoundSettings;
    TWeakPtr<SWindow> BoundWindow;
    FTSTicker::FDelegateHandle TickHandle;
    FDelegateHandle WorldCleanupHandle;

    FETVideoPreviewSnapshot LastPublished;
    int32 LastRemaining = INDEX_NONE;
    bool bPublished = false;
    bool bOperating = false;
    bool bPublishing = false;
    bool bInsideTicker = false;
    bool bCancelDeferred = false;
    bool bEnding = false;
};