#pragma once

#include "CoreMinimal.h"
#include "Inventory/ETRelicViewModels.h"
#include "UObject/Object.h"
#include "ETRelicPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETRelicEquipmentComponent;
class UETRelicInventorySubsystem;
class UETUILocalPlayerSubsystem;
struct FETRelicInventorySnapshot;
struct FOnAttributeChangeData;

enum class EETRelicPanelState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    Waiting,
    Ready
};
enum class EETRelicPreviewState : uint8
{
    Unavailable,
    Available,
    AlreadyEquipped,
    Rejected
};

// 原生View快照；不含世界对象或确认权限，不把库存版本转成Blueprint不支持的uint64属性。
struct EMBERTRIALUI_API FETRelicPanelData
{
    EETRelicPanelState State = EETRelicPanelState::Unavailable;
    EETRelicPreviewState PreviewState = EETRelicPreviewState::Unavailable;
    FName SelectedId = NAME_None;
    FText Name;
    FText Description;
    bool bSlotB = false;
    float BeforeMax = 0.0f;
    float AfterMax = 0.0f;
    float BeforeAttack = 0.0f;
    float AfterAttack = 0.0f;
    bool operator==(const FETRelicPanelData& Other) const;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicPanelChanged, const FETRelicPanelData&);

// 页面作用域的只读Presenter：观察数据源、复用稳定ID的Item、保存页面选择。
UCLASS()
class EMBERTRIALUI_API UETRelicPresenter : public UObject
{
    GENERATED_BODY()

public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();
    bool SelectId(FName Id);
    bool SetPreviewSlotB(bool bSecondSlot);
    bool SetQuery(const FETRelicListQuery& NewQuery);
    const FETRelicListQuery& GetQuery() const { return Query; }
    const FETRelicPanelData& GetSnapshot() const { return Snapshot; }
    const TArray<TObjectPtr<UETRelicListItem>>& GetItems() const { return VisibleItems; }
    UETRelicListItem* FindItem(FName Id) const;
    FETRelicPanelChanged OnChanged;

protected:
    virtual void BeginDestroy() override;

private:
    friend class FETRelicPresenterTest;
    void StopObserving(bool bNotify);
    void ClearBodyBindings();
    void BindCurrentPawn();
    void Refresh();
    void ResolveInventory();
    FETRelicPanelData BuildData() const;

    UFUNCTION()
    void HandlePawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);
    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);
    void HandleInventoryChanged(const FETRelicInventorySnapshot& Data,
                                TWeakObjectPtr<UETRelicInventorySubsystem> Source);
    void HandleEquipmentChanged(UETRelicEquipmentComponent* Source);
    void HandleAttributeChanged(const FOnAttributeChangeData& Data,
                               TWeakObjectPtr<UAbilitySystemComponent> Source);

    // 页面强持有Item；底层世界 / 库存只有弱引用和本实例的订阅Handle。
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UETRelicListItem>> ItemsById;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UETRelicListItem>> VisibleItems;
    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<UETRelicInventorySubsystem> BoundInventory;
    TWeakObjectPtr<AEmberTrialCharacter> BoundBody;
    TWeakObjectPtr<UETRelicEquipmentComponent> BoundEquipment;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle InventoryHandle;
    FDelegateHandle EquipmentHandle;
    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHandle;
    FDelegateHandle BonusHandle;
    FETRelicListQuery Query;
    FETRelicPanelData Snapshot;
    FName SelectedId = NAME_None;
    uint64 ObservedRevision = 0;
    bool bSlotB = false;
    bool bObserving = false;
    bool bRefreshing = false;
};