//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <gamePhys/phys/walker/humanAnimState.h>

MAKE_TYPE_FACTORY(HumanAnimState, HumanAnimState);
MAKE_TYPE_FACTORY(HumanAnimStateResult, HumanAnimState::StateResult);
MAKE_TYPE_FACTORY(HumanAnimUpperState, HumanAnimState::UpperState);
MAKE_TYPE_FACTORY(HumanAnimLowerState, HumanAnimState::LowerState);
MAKE_TYPE_FACTORY(HumanAnimComboState, HumanAnimState::ComboState);

namespace das
{
template <>
struct cast<HumanAnimState::StateResult> : cast_iVec_half<HumanAnimState::StateResult>
{};
} // namespace das

namespace bind_dascript
{
inline HumanAnimState::StateResult human_anim_state_update_state(HumanAnimState &animState, HumanStatePos state_pos,
  HumanStateMove state_move, StateJump state_jump, HumanStateUpperBody state_upper_body, uint32_t state_flags)
{
  return animState.updateState(state_pos, state_move, state_jump, state_upper_body, HumanAnimStateFlags(state_flags));
}
} // namespace bind_dascript
