#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETPracticeActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// 练习目标的原生可视 / 文本基础；交互范围与激活逻辑仍由既有蓝图装配。
UCLASS(Blueprintable)
class EMBERTRIAL_API AETPracticeActor : public AActor
{
    GENERATED_BODY()

public:
    // ---- 反射查询 / 日志入口仍供蓝图与历史排障使用 ----

    AETPracticeActor();

    UFUNCTION(BlueprintCallable, Category = "Practice")
    void PrintPracticeInfo();

    UFUNCTION(BlueprintPure, Category = "Practice")
    FText GetPracticeLabel() const;

private:
    // ---- 默认子对象身份与本地化键保持，Label允许实例独立配置 ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Practice",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Practice",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice",
              meta = (AllowPrivateAccess = "true"))
    FText PracticeLabel;
};
