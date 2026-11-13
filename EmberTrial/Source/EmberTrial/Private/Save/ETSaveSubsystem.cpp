#include "Save/ETSaveSubsystem.h"

#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
constexpr uint32 RunEnvelopeTag = 0x31535445; // ETS1
constexpr int32 RunEnvelopeHeaderBytes = sizeof(uint32) + sizeof(uint32);
constexpr int32 MaxRunSaveBytes = 1024 * 1024;

FPlatformUserId RunSaveUser()
{
    return FPlatformMisc::GetPlatformUserForUserIndex(0);
}
}

struct FETSaveOperation
{
    FETSaveResult Slots[2];
    FString SlotPrefix;
    TOptional<FETRunSaveSnapshot> PendingSnapshot;
    FETSaveCompleted Completion;
    FString RetireMap;
    bool bMirrorRetirement = false;
};

void UETSaveSubsystem::Deinitialize()
{
    bShuttingDown = true;
    ActiveRequest.Reset();
    Super::Deinitialize();
}

bool UETSaveSubsystem::RequestLoad(FETSaveCompleted Completion, EETSaveScope Scope)
{
    return BeginRequest({}, MoveTemp(Completion), Scope);
}

bool UETSaveSubsystem::RequestSave(FETRunSaveSnapshot Snapshot, FETSaveCompleted Completion,
                                   EETSaveScope Scope)
{
    return BeginRequest(MoveTemp(Snapshot), MoveTemp(Completion), Scope);
}

bool UETSaveSubsystem::RequestRetire(FString MapPackage, FETSaveCompleted Completion, EETSaveScope Scope)
{
    return !MapPackage.IsEmpty() && BeginRequest({}, MoveTemp(Completion), Scope, MoveTemp(MapPackage));
}

bool UETSaveSubsystem::BeginRequest(TOptional<FETRunSaveSnapshot> Snapshot,
                                   FETSaveCompleted Completion, EETSaveScope Scope, FString RetireMap)
{
    check(IsInGameThread());
    if (bShuttingDown || ActiveRequest.IsValid())
    {
        return false;
    }

    ActiveRequest = MakeShared<FETSaveOperation>();
    ActiveRequest->SlotPrefix = Scope == EETSaveScope::EditorPreview ? SlotPrefix + TEXT("_PIE") : SlotPrefix;
    ActiveRequest->PendingSnapshot = MoveTemp(Snapshot);
    ActiveRequest->RetireMap = MoveTemp(RetireMap);
    ActiveRequest->Completion = MoveTemp(Completion);
    ReadSlot(0);
    return true;
}

void UETSaveSubsystem::ReadSlot(const int32 Index)
{
    ISaveGameSystem* Storage = IPlatformFeaturesModule::Get().GetSaveGameSystem();
    if (!Storage)
    {
        Finish({EETSaveStatus::IOError});
        return;
    }

    const FString Slot = ActiveRequest->SlotPrefix + (Index == 0 ? TEXT("_A") : TEXT("_B"));
    const TWeakObjectPtr<UETSaveSubsystem> WeakThis(this);
    const auto CompleteRead = [WeakThis, Index](EETSaveStatus Status, const TArray<uint8>& Bytes)
    {
        UETSaveSubsystem* Self = WeakThis.Get();
        if (!Self || !Self->ActiveRequest.IsValid())
        {
            return;
        }

        FETSaveResult& Read = Self->ActiveRequest->Slots[Index];
        Read.Status = Status == EETSaveStatus::Success ? Decode(Bytes, Read.Snapshot) : Status;
        if (Index == 0)
        {
            Self->ReadSlot(1);
        }
        else
        {
            Self->ResolveSlots();
        }
    };

    Storage->DoesSaveGameExistAsync(*Slot, RunSaveUser(),
        [Storage, Slot, WeakThis, CompleteRead](const FString&, FPlatformUserId User,
                                                ISaveGameSystem::ESaveExistsResult Exists)
        {
            UETSaveSubsystem* Self = WeakThis.Get();
            if (!Self || !Self->ActiveRequest.IsValid())
            {
                return;
            }

            using EExists = ISaveGameSystem::ESaveExistsResult;
            switch (Exists)
            {
            case EExists::OK:
                Storage->LoadGameAsync(false, *Slot, User,
                    [CompleteRead](const FString&, FPlatformUserId, bool bRead, const TArray<uint8>& Bytes)
                    {
                        CompleteRead(bRead ? EETSaveStatus::Success : EETSaveStatus::IOError, Bytes);
                    });
                break;
            case EExists::DoesNotExist:
                CompleteRead(EETSaveStatus::NoSave, {});
                break;
            case EExists::Corrupt:
                CompleteRead(EETSaveStatus::Corrupt, {});
                break;
            default:
                CompleteRead(EETSaveStatus::IOError, {});
                break;
            }
        });
}

void UETSaveSubsystem::ResolveSlots()
{
    const FETSaveResult& A = ActiveRequest->Slots[0];
    const FETSaveResult& B = ActiveRequest->Slots[1];
    if (A.Status == EETSaveStatus::UnsupportedVersion || B.Status == EETSaveStatus::UnsupportedVersion)
    {
        Finish({EETSaveStatus::UnsupportedVersion});
        return;
    }

    const bool bAValid = A.Status == EETSaveStatus::Success;
    const bool bBValid = B.Status == EETSaveStatus::Success;
    const bool bHasValidSlot = bAValid || bBValid;
    const int32 Preferred = bBValid && (!bAValid || B.Snapshot.Sequence > A.Snapshot.Sequence) ? 1 : 0;
    const bool bReadError = A.Status == EETSaveStatus::IOError || B.Status == EETSaveStatus::IOError;
    const bool bCorrupt = A.Status == EETSaveStatus::Corrupt || B.Status == EETSaveStatus::Corrupt;
    const EETSaveStatus Unavailable = bReadError ? EETSaveStatus::IOError :
        (bCorrupt ? EETSaveStatus::Corrupt : EETSaveStatus::NoSave);

    if (!ActiveRequest->RetireMap.IsEmpty())
    {
        const FETSaveResult& Latest = ActiveRequest->Slots[Preferred];
        if (bReadError || !bHasValidSlot)
        {
            Finish({Unavailable});
            return;
        }
        if (Latest.Snapshot.MapPackage != ActiveRequest->RetireMap)
        {
            Finish({EETSaveStatus::NoSave});
            return;
        }
        if (bAValid && bBValid && A.Snapshot.bRunClosed && B.Snapshot.bRunClosed)
        {
            Finish(Latest);
            return;
        }

        // Keep a verified copy while changing each slot. A verified newer marker hides the old checkpoint.
        FETRunSaveSnapshot Marker = Latest.Snapshot;
        if (!Marker.bRunClosed && Marker.Sequence == MAX_int64)
        {
            Finish({EETSaveStatus::InvalidSnapshot});
            return;
        }
        ActiveRequest->bMirrorRetirement = !Marker.bRunClosed;
        if (!Marker.bRunClosed)
        {
            ++Marker.Sequence;
            Marker.SavedAtUtc = FDateTime::UtcNow();
            Marker.bRunClosed = true;
        }
        WriteSlot(1 - Preferred, MoveTemp(Marker));
        return;
    }

    if (!ActiveRequest->PendingSnapshot.IsSet())
    {
        if (bHasValidSlot)
        {
            FETSaveResult Result = ActiveRequest->Slots[Preferred];
            if (Result.Snapshot.bRunClosed)
            {
                Finish({EETSaveStatus::NoSave});
                return;
            }
            Result.bOtherSlotUnavailable = bReadError || bCorrupt;
            Finish(MoveTemp(Result));
        }
        else
        {
            Finish({Unavailable});
        }
        return;
    }

    // Protect unreadable files, and keep both damaged files for explicit recovery.
    if (bReadError || (bCorrupt && !bHasValidSlot))
    {
        Finish({Unavailable});
        return;
    }

    const int64 PreviousSequence = bHasValidSlot ? ActiveRequest->Slots[Preferred].Snapshot.Sequence : 0;
    if (PreviousSequence == MAX_int64)
    {
        Finish({EETSaveStatus::InvalidSnapshot});
        return;
    }

    FETRunSaveSnapshot Snapshot = MoveTemp(ActiveRequest->PendingSnapshot.GetValue());
    Snapshot.Sequence = PreviousSequence + 1;
    Snapshot.SavedAtUtc = FDateTime::UtcNow();
    if (Snapshot.bRunClosed || Snapshot.Validate() != EETRunSaveValidation::Valid)
    {
        Finish({EETSaveStatus::InvalidSnapshot});
        return;
    }

    WriteSlot(bHasValidSlot ? 1 - Preferred : 0, MoveTemp(Snapshot));
}

void UETSaveSubsystem::WriteSlot(const int32 Index, FETRunSaveSnapshot Snapshot)
{
    ISaveGameSystem* Storage = IPlatformFeaturesModule::Get().GetSaveGameSystem();
    const TSharedRef<TArray<uint8>> Bytes = MakeShared<TArray<uint8>>();
    if (!Storage || !Encode(Snapshot, *Bytes))
    {
        Finish({EETSaveStatus::IOError});
        return;
    }

    const FString Slot = ActiveRequest->SlotPrefix + (Index == 0 ? TEXT("_A") : TEXT("_B"));
    const TWeakObjectPtr<UETSaveSubsystem> WeakThis(this);
    Storage->SaveGameAsync(false, *Slot, RunSaveUser(), Bytes,
        [Storage, Slot, WeakThis, Bytes, Index, Snapshot = MoveTemp(Snapshot)]
        (const FString&, FPlatformUserId User, bool bWritten) mutable
        {
            UETSaveSubsystem* Self = WeakThis.Get();
            if (!Self || !Self->ActiveRequest.IsValid())
            {
                return;
            }
            if (!bWritten)
            {
                Self->Finish({EETSaveStatus::IOError});
                return;
            }

            Storage->LoadGameAsync(false, *Slot, User,
                [WeakThis, Bytes, Index, Snapshot = MoveTemp(Snapshot)]
                (const FString&, FPlatformUserId, bool bRead, const TArray<uint8>& ReadBytes) mutable
                {
                    if (UETSaveSubsystem* Store = WeakThis.Get(); Store && Store->ActiveRequest.IsValid())
                    {
                        if (bRead && ReadBytes == *Bytes)
                        {
                            if (Store->ActiveRequest->bMirrorRetirement)
                            {
                                Store->ActiveRequest->bMirrorRetirement = false;
                                Store->WriteSlot(1 - Index, MoveTemp(Snapshot));
                            }
                            else
                            {
                                Store->Finish({EETSaveStatus::Success, MoveTemp(Snapshot)});
                            }
                        }
                        else
                        {
                            Store->Finish({EETSaveStatus::IOError});
                        }
                    }
                });
        });
}

void UETSaveSubsystem::Finish(FETSaveResult Result)
{
    FETSaveCompleted Completion = MoveTemp(ActiveRequest->Completion);
    ActiveRequest.Reset();
    Completion.ExecuteIfBound(Result);
}

bool UETSaveSubsystem::Encode(const FETRunSaveSnapshot& Snapshot, TArray<uint8>& OutBytes)
{
    UETRunSaveGame* Save = NewObject<UETRunSaveGame>();
    Save->Snapshot = Snapshot;
    TArray<uint8> Payload;
    if (!UGameplayStatics::SaveGameToMemory(Save, Payload) || Payload.IsEmpty() ||
        Payload.Num() > MaxRunSaveBytes - RunEnvelopeHeaderBytes)
    {
        return false;
    }

    OutBytes.Reset();
    FMemoryWriter Writer(OutBytes, true);
    uint32 Tag = RunEnvelopeTag;
    uint32 Checksum = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
    Writer << Tag << Checksum;
    Writer.Serialize(Payload.GetData(), Payload.Num());
    return !Writer.IsError();
}

EETSaveStatus UETSaveSubsystem::Decode(const TArray<uint8>& Bytes, FETRunSaveSnapshot& OutSnapshot)
{
    if (Bytes.Num() <= RunEnvelopeHeaderBytes || Bytes.Num() > MaxRunSaveBytes)
    {
        return EETSaveStatus::Corrupt;
    }

    // Check the envelope before passing any bytes to UE's object deserializer.
    FMemoryReader Reader(Bytes, true);
    uint32 Tag = 0;
    uint32 Checksum = 0;
    Reader << Tag << Checksum;
    const int32 PayloadSize = Bytes.Num() - RunEnvelopeHeaderBytes;
    const uint8* PayloadStart = Bytes.GetData() + RunEnvelopeHeaderBytes;
    if (Tag != RunEnvelopeTag || FCrc::MemCrc32(PayloadStart, PayloadSize) != Checksum)
    {
        return EETSaveStatus::Corrupt;
    }

    const TArray<uint8> Payload(PayloadStart, PayloadSize);
    UETRunSaveGame* Save = Cast<UETRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
    if (!Save)
    {
        return EETSaveStatus::Corrupt;
    }

    switch (Save->Snapshot.UpgradeFromDisk())
    {
    case EETRunSaveValidation::Valid:
        OutSnapshot = Save->Snapshot;
        return EETSaveStatus::Success;
    case EETRunSaveValidation::UnsupportedVersion:
        return EETSaveStatus::UnsupportedVersion;
    default:
        return EETSaveStatus::Corrupt;
    }
}
