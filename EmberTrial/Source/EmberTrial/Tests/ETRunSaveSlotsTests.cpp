#if WITH_DEV_AUTOMATION_TESTS

#include "Save/ETSaveSubsystem.h"

#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRunSaveSlotsTest, "EmberTrial.M5.Save.Slots",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRunSaveSlotsTest::RunTest(const FString& Parameters)
{
    struct FFixture
    {
        TStrongObjectPtr<UGameInstance> GameInstance;
        TStrongObjectPtr<UETSaveSubsystem> Store;
        TOptional<FETSaveResult> Result;
        bool bAborted = false;
    };

    const TSharedRef<FFixture> Fixture = MakeShared<FFixture>();
    Fixture->GameInstance.Reset(NewObject<UGameInstance>());
    Fixture->Store.Reset(NewObject<UETSaveSubsystem>(Fixture->GameInstance.Get()));
    const FString Prefix = TEXT("EmberTrial_Auto_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString SlotA = Prefix + TEXT("_A");
    const FString SlotB = Prefix + TEXT("_B");
    const FString PreviewSlotA = Prefix + TEXT("_PIE_A");
    Fixture->Store->SlotPrefix = Prefix;

    FETRunSaveSnapshot Snapshot;
    Snapshot.SchemaVersion = FETRunSaveSnapshot::CurrentSchemaVersion;
    Snapshot.MapPackage = TEXT("/Game/EmberTrial/Maps/L_ET_TrialRoute");
    Snapshot.CheckpointId = TEXT("Checkpoint_A");
    Snapshot.HealingCharges = 0;

    const auto QueueStep = [this, Fixture](const TCHAR* Label,
                                           TFunction<bool(FETSaveCompleted)> Start,
                                           TFunction<void(const FETSaveResult&)> Verify)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(
            [this, Fixture, Label, Start = MoveTemp(Start), Verify = MoveTemp(Verify), StartedAt = 0.0]() mutable
            {
                if (Fixture->bAborted)
                {
                    return true;
                }
                if (StartedAt == 0.0)
                {
                    StartedAt = FPlatformTime::Seconds();
                    Fixture->Result.Reset();
                    const TWeakPtr<FFixture> WeakFixture = Fixture;
                    const bool bAccepted = Start(FETSaveCompleted::CreateLambda(
                        [WeakFixture](const FETSaveResult& Result)
                        {
                            if (const TSharedPtr<FFixture> State = WeakFixture.Pin())
                            {
                                State->Result = Result;
                            }
                        }));
                    if (!TestTrue(Label, bAccepted))
                    {
                        Fixture->bAborted = true;
                        return true;
                    }
                }

                if (Fixture->Result.IsSet())
                {
                    Verify(Fixture->Result.GetValue());
                    return true;
                }
                if (FPlatformTime::Seconds() - StartedAt > 15.0)
                {
                    AddError(FString::Printf(TEXT("Timed out: %s"), Label));
                    Fixture->bAborted = true;
                    return true;
                }
                return false;
            }));
    };

    const auto Load = [Fixture](FETSaveCompleted Done)
    {
        return Fixture->Store->RequestLoad(MoveTemp(Done));
    };
    QueueStep(TEXT("Load missing slots"), Load, [this](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Missing slots report NoSave"), Result.Status == EETSaveStatus::NoSave);
    });
    QueueStep(TEXT("Save checkpoint A"), [this, Fixture, Snapshot](FETSaveCompleted Done)
    {
        const bool bAccepted = Fixture->Store->RequestSave(Snapshot, MoveTemp(Done));
        TestFalse(TEXT("Overlapping request is rejected"), Fixture->Store->RequestLoad({}));
        return bAccepted;
    }, [this, SlotA, SlotB](const FETSaveResult& Result)
    {
        TestTrue(TEXT("First write verified"), Result.Status == EETSaveStatus::Success);
        TestEqual(TEXT("First sequence"), Result.Snapshot.Sequence, int64(1));
        TestTrue(TEXT("First write targets A"), UGameplayStatics::DoesSaveGameExist(SlotA, 0));
        TestFalse(TEXT("B stays empty"), UGameplayStatics::DoesSaveGameExist(SlotB, 0));
    });

    Snapshot.CheckpointId = TEXT("Checkpoint_B");
    QueueStep(TEXT("Save checkpoint B"), [Fixture, Snapshot](FETSaveCompleted Done)
    {
        return Fixture->Store->RequestSave(Snapshot, MoveTemp(Done));
    }, [this, SlotA, SlotB](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Second write verified"), Result.Status == EETSaveStatus::Success);
        TestEqual(TEXT("Second sequence"), Result.Snapshot.Sequence, int64(2));
        TestTrue(TEXT("Second write keeps both slots"), UGameplayStatics::DoesSaveGameExist(SlotA, 0) &&
                                                        UGameplayStatics::DoesSaveGameExist(SlotB, 0));
    });
    QueueStep(TEXT("Load latest slot"), Load, [this, SlotB](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Read succeeded"), Result.Status == EETSaveStatus::Success);
        TestEqual(TEXT("Highest sequence wins"), Result.Snapshot.CheckpointId, FName(TEXT("Checkpoint_B")));
        TArray<uint8> Bytes;
        if (TestTrue(TEXT("Read bytes for corruption test"), UGameplayStatics::LoadDataFromSlot(Bytes, SlotB, 0)) &&
            TestTrue(TEXT("Nonempty slot"), !Bytes.IsEmpty()))
        {
            Bytes.Last() ^= 1;
            TestTrue(TEXT("Flip one byte in latest test slot"), UGameplayStatics::SaveDataToSlot(Bytes, SlotB, 0));
        }
    });
    QueueStep(TEXT("Load previous valid slot"), Load, [this, Fixture, SlotB](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Previous slot is readable"), Result.Status == EETSaveStatus::Success);
        TestTrue(TEXT("Corrupt sibling is reported"), Result.bOtherSlotUnavailable);
        TestEqual(TEXT("Previous checkpoint wins"), Result.Snapshot.CheckpointId, FName(TEXT("Checkpoint_A")));
        FETRunSaveSnapshot Future = Result.Snapshot;
        ++Future.SchemaVersion;
        TArray<uint8> Bytes;
        if (TestTrue(TEXT("Encode fixture using future schema"), Fixture->Store->Encode(Future, Bytes)))
        {
            TestTrue(TEXT("Write future format test slot"), UGameplayStatics::SaveDataToSlot(Bytes, SlotB, 0));
        }
    });
    QueueStep(TEXT("Save while a future version exists"), [Fixture, Snapshot](FETSaveCompleted Done)
    {
        return Fixture->Store->RequestSave(Snapshot, MoveTemp(Done));
    }, [this](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Protected slot blocks replacement"), Result.Status == EETSaveStatus::UnsupportedVersion);
    });
    QueueStep(TEXT("Read future version status"), Load, [this](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Future slot remains intact"), Result.Status == EETSaveStatus::UnsupportedVersion);
    });

    QueueStep(TEXT("Save into isolated PIE scope"), [Fixture, Snapshot](FETSaveCompleted Done)
    {
        return Fixture->Store->RequestSave(Snapshot, MoveTemp(Done), EETSaveScope::EditorPreview);
    }, [this, PreviewSlotA](const FETSaveResult& Result)
    {
        TestTrue(TEXT("PIE save is independent of the protected run slots"), Result.Status == EETSaveStatus::Success);
        TestEqual(TEXT("PIE starts its own sequence"), Result.Snapshot.Sequence, int64(1));
        TestTrue(TEXT("PIE chooses the preview filename"), UGameplayStatics::DoesSaveGameExist(PreviewSlotA, 0));
    });
    QueueStep(TEXT("Read isolated PIE scope"), [Fixture](FETSaveCompleted Done)
    {
        return Fixture->Store->RequestLoad(MoveTemp(Done), EETSaveScope::EditorPreview);
    }, [this](const FETSaveResult& Result)
    {
        TestTrue(TEXT("PIE read returns the preview save"), Result.Status == EETSaveStatus::Success);
        TestEqual(TEXT("PIE round trip preserves checkpoint"), Result.Snapshot.CheckpointId,
                  FName(TEXT("Checkpoint_B")));
    });
    QueueStep(TEXT("Check run scope after PIE write"), Load, [this](const FETSaveResult& Result)
    {
        TestTrue(TEXT("Run slot is still protected"), Result.Status == EETSaveStatus::UnsupportedVersion);
    });

    ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Fixture, SlotA, SlotB, PreviewSlotA]()
    {
        Fixture->Store->Deinitialize();
        UGameplayStatics::DeleteGameInSlot(SlotA, 0);
        UGameplayStatics::DeleteGameInSlot(SlotB, 0);
        UGameplayStatics::DeleteGameInSlot(PreviewSlotA, 0);
        return true;
    }));
    return true;
}

#endif
