#pragma once

#include "CoreMinimal.h"
#include "Save/ETRunSaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ETSaveSubsystem.generated.h"

enum class EETSaveStatus : uint8
{
    Success,
    NoSave,
    Corrupt,
    UnsupportedVersion,
    IOError,
    InvalidSnapshot
};

struct FETSaveResult
{
    EETSaveStatus Status = EETSaveStatus::NoSave;
    FETRunSaveSnapshot Snapshot;
    bool bOtherSlotUnavailable = false;
};

enum class EETSaveScope : uint8
{
    Run,
    EditorPreview
};

DECLARE_DELEGATE_OneParam(FETSaveCompleted, const FETSaveResult&);

struct FETSaveOperation;

UCLASS()
class EMBERTRIAL_API UETSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    // Call from the game thread. False means busy or shutting down.
    // Accepted requests report a result on the game thread.
    bool RequestLoad(FETSaveCompleted Completion, EETSaveScope Scope = EETSaveScope::Run);
    bool RequestSave(FETRunSaveSnapshot Snapshot, FETSaveCompleted Completion,
                     EETSaveScope Scope = EETSaveScope::Run);
    bool RequestRetire(FString MapPackage, FETSaveCompleted Completion, EETSaveScope Scope);

private:
    friend class FETRunSaveSlotsTest;

    bool BeginRequest(TOptional<FETRunSaveSnapshot> Snapshot, FETSaveCompleted Completion,
                      EETSaveScope Scope, FString RetireMap = {});
    void ReadSlot(int32 Index);
    void ResolveSlots();
    void WriteSlot(int32 Index, FETRunSaveSnapshot Snapshot);
    void Finish(FETSaveResult Result);

    static bool Encode(const FETRunSaveSnapshot& Snapshot, TArray<uint8>& OutBytes);
    static EETSaveStatus Decode(const TArray<uint8>& Bytes, FETRunSaveSnapshot& OutSnapshot);

    // The test uses a unique prefix and never touches these default slots.
    FString SlotPrefix = TEXT("EmberTrial_Run");
    TSharedPtr<FETSaveOperation> ActiveRequest;
    bool bShuttingDown = false;
};
