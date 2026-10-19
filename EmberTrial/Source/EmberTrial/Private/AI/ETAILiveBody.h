#pragma once

#include "CoreMinimal.h"
#include "Characters/EmberTrialCharacter.h"

namespace ETAIHealth
{
// 共享AI存活规则；每次读取Ready快照，不缓存可能已被伤害 / 死亡回调改变的属性。
// 玩家身份、阵营与Controller归属由调用方另行验证。
inline bool IsLiveBody(const AEmberTrialCharacter* BodyCharacter)
{
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    return IsValid(BodyCharacter) && !BodyCharacter->IsActorBeingDestroyed() &&
           !BodyCharacter->IsCombatDead() &&
           BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) && FMath::IsFinite(Health) &&
           FMath::IsFinite(MaxHealth) && MaxHealth >= 1.0f && Health > 0.0f && Health <= MaxHealth;
}
} // namespace ETAIHealth
