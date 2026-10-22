#include "Abilities/ETGameplayTags.h"

// 保持注册顺序、名字与说明字符串；只在此定义，不在ini重复注册。
namespace ETGameplayTags
{
    // ---- 攻击能力与状态 ----
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Light, "Ability.Attack.Light",
                               "Identifies the single light attack ability.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking",
                               "Owned by GAS while a formal attack is active.");
    // ---- 有符号属性增量；调用方提供幅度，不在Tag定义中存数值 ----
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HealthDelta, "SetByCaller.HealthDelta",
                               "Signed Health delta carried by an effect spec.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_StaminaDelta, "SetByCaller.StaminaDelta",
                               "Signed stamina change carried by an Instant effect spec.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PoiseDelta, "SetByCaller.PoiseDelta",
                                   "Signed poise change carried by an Instant effect spec.");
    // ---- 受击与死亡状态 ----
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact, "Ability.HitReact",
                               "Identifies the formal hit reaction ability.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HitReact, "State.HitReact",
                               "Owned by GAS while a formal hit reaction is active.");
    UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
    // ---- 闪避与短无敌 ----
    UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(State_Dodging, "State.Dodging");
    UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Invulnerable");
    // ---- 破势反应 ----
    UE_DEFINE_GAMEPLAY_TAG(Ability_Stagger, "Ability.Stagger");
    UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned");
    // ---- 治疗动作 ----
    UE_DEFINE_GAMEPLAY_TAG(Ability_Heal, "Ability.Heal");
    UE_DEFINE_GAMEPLAY_TAG(State_Healing, "State.Healing");

} // namespace ETGameplayTags
