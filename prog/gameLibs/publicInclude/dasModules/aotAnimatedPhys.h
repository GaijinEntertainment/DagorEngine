//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotAnimchar.h>
#include <dasModules/aotPhysVars.h>
#include <ecs/phys/animPhys.h>

MAKE_TYPE_FACTORY(AnimatedPhys, AnimatedPhys);

namespace bind_dascript
{
inline void anim_phys_init(AnimatedPhys &anim_phys, const AnimV20::AnimcharBaseComponent &anim_char, const PhysVars &phys_vars)
{
  anim_phys.init(anim_char, phys_vars);
}

inline void anim_phys_append_var(AnimatedPhys &anim_phys, const char *var_name, const AnimV20::AnimcharBaseComponent &anim_char,
  const PhysVars &phys_vars)
{
  anim_phys.appendVar(var_name ? var_name : "", anim_char, phys_vars);
}

inline void anim_phys_update(AnimatedPhys &anim_phys, AnimV20::AnimcharBaseComponent &anim_char, PhysVars &phys_vars)
{
  anim_phys.update(anim_char, phys_vars);
}
}; // namespace bind_dascript
