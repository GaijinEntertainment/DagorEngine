//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <forceFeedback/forceFeedback.h>

typedef force_feedback::rumble::EventParams RumbleEventParams;

MAKE_TYPE_FACTORY(RumbleEventParams, RumbleEventParams);

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(force_feedback::rumble::EventParams::Band, RumbleBand);
DAS_BASE_BIND_ENUM_98(force_feedback::rumble::EventParams::Band, RumbleBand, LO, HI)

namespace bind_dascript
{
template <typename Fill>
inline void set_trigger_effect(int trigger_mask, Fill fill)
{
  HumanInput::TriggerEffectParam param;
  param.triggerMask = trigger_mask;
  for (HumanInput::TriggerEffectCommand &cmd : param.command)
    fill(cmd);
  force_feedback::trigger_effect::set(param);
}

inline void force_feedback_trigger_vibration(int trigger_mask, int position, int amplitude, int frequency)
{
  set_trigger_effect(trigger_mask, [&](HumanInput::TriggerEffectCommand &cmd) {
    cmd.mode = HumanInput::TRIGGER_EFFECT_MODE_VIBRATION;
    cmd.vibration.position = position;
    cmd.vibration.amplitude = amplitude;
    cmd.vibration.frequency = frequency;
  });
}

inline void force_feedback_trigger_feedback(int trigger_mask, int position, int strength)
{
  set_trigger_effect(trigger_mask, [&](HumanInput::TriggerEffectCommand &cmd) {
    cmd.mode = HumanInput::TRIGGER_EFFECT_MODE_FEEDBACK;
    cmd.feedback.position = position;
    cmd.feedback.strength = strength;
  });
}

inline void force_feedback_trigger_weapon(int trigger_mask, int start_position, int end_position, int strength)
{
  set_trigger_effect(trigger_mask, [&](HumanInput::TriggerEffectCommand &cmd) {
    cmd.mode = HumanInput::TRIGGER_EFFECT_MODE_WEAPON;
    cmd.weapon.startPosition = start_position;
    cmd.weapon.endPosition = end_position;
    cmd.weapon.strength = strength;
  });
}

inline void force_feedback_trigger_off(int trigger_mask)
{
  set_trigger_effect(trigger_mask, [](HumanInput::TriggerEffectCommand &) {});
}
} // namespace bind_dascript
