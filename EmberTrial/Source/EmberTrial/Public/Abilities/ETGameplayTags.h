#pragma once

#include "NativeGameplayTags.h"

// 原生Tag名字属于稳定契约；能力 / Life持有状态，消费者不临时拼字符串。
namespace ETGameplayTags
{
    // ---- 攻击能力与状态 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Light);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);
    // ---- 有符号属性增量；Instant GE的SetByCaller通道 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_HealthDelta);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_StaminaDelta);
    // ---- 受击与死亡状态 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_HitReact);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
    // ---- 闪避与短无敌 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);
    // ---- 破势反应 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Stagger);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
    // ---- 治疗动作 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Heal);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Healing);
} // namespace ETGameplayTags
