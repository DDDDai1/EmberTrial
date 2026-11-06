#pragma once

#include "CoreMinimal.h"
#include "Settings/ETAudioSettingsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ETAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundClass;
class USoundMix;
struct FSoundClassAdjuster;

DECLARE_MULTICAST_DELEGATE(FETAudioRoutingChanged);

/** 本机游戏音量路由；资产从共享配置读取，偏好由GameUserSettings持有。 */
UCLASS(Config = Game, DefaultConfig)
class EMBERTRIAL_API UETAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 成功表示混音请求已提交给引擎，不表示声卡已完成播放。
    bool ApplyRuntime(const FETAudioPreferences& Value, UWorld* World);
    bool IsRoutingApplied(const UWorld* World) const;
    void StopProbe();

    // 路由可用性事件；监听者只刷新资格，不在回调里再次ApplyRuntime。
    FETAudioRoutingChanged OnRoutingChanged;

#if !UE_BUILD_SHIPPING
    bool PlayProbe(bool bMusic, UWorld* World);
#endif

    // 无声音副作用；构造一次Master递归 + 两个叶子独立增益。
    static bool BuildAdjusters(const FETAudioPreferences& Value,
        USoundClass* Master, USoundClass* Music, USoundClass* Effects,
        USoundMix* Mix, TArray<FSoundClassAdjuster>& Out);

private:
    void HandlePostLoadMap(UWorld* World);
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    void ReleaseWorld();

    UPROPERTY(Config)
    TSoftObjectPtr<USoundClass> MasterClass;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundClass> MusicClass;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundClass> EffectsClass;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundMix> UserMix;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundBase> MusicProbe;
    UPROPERTY(Config)
    TSoftObjectPtr<USoundBase> EffectsProbe;

    // 保活本服务资源；不修改资产默认Volume，不清除其他系统的Mix。
    UPROPERTY(Transient)
    TObjectPtr<USoundClass> LoadedMaster;
    UPROPERTY(Transient)
    TObjectPtr<USoundClass> LoadedMusic;
    UPROPERTY(Transient)
    TObjectPtr<USoundClass> LoadedEffects;
    UPROPERTY(Transient)
    TObjectPtr<USoundMix> LoadedMix;
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ProbeComponent;

    TWeakObjectPtr<UWorld> BoundWorld;
    FDelegateHandle PostMapHandle;
    FDelegateHandle CleanupHandle;
    bool bMixPushed = false;
};