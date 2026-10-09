#include "Abilities/ETGameplayTags.h"

namespace ETGameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Light,
        "Ability.Attack.Light", "Identifies the single light attack ability.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking,
        "State.Attacking", "Owned by GAS while a formal attack is active.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HealthDelta,
    "SetByCaller.HealthDelta", "Signed Health delta carried by an effect spec.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact,
    "Ability.HitReact", "Identifies the formal hit reaction ability.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HitReact,
        "State.HitReact", "Owned by GAS while a formal hit reaction is active.");
    UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
}