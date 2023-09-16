//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <anim/dag_animDecl.h>
#include <EASTL/bitvector.h>
#include <generic/dag_smallTab.h>

class PhysVars;

class AnimatedPhys // TODO: combine this class with PhysVars
{
  eastl::bitvector<> pullBitmap;
  SmallTab<int16_t> remappedVars; // parallel to PhysVars::vars
  void addRemap(int animNid, int physNid, bool pullable);

public:
  void init(const AnimV20::AnimcharBaseComponent &anim_char, const PhysVars &vars);
  void appendVar(const char *var_name, const AnimV20::AnimcharBaseComponent &anim_char, const PhysVars &phys_vars);
  void update(AnimV20::AnimcharBaseComponent &anim_char, PhysVars &vars);
};
