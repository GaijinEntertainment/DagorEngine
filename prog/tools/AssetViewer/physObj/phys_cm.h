// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_cm.h"

enum
{
  CM_PHYSOBJ_SIM_TOGGLE_DRIVER = CM_PLUGIN_BASE + 1,
  CM_PHYSOBJ_CHANGE_LOD_AUTO,
  CM_PHYSOBJ_CHANGE_LOD_0,
  CM_PHYSOBJ_CHANGE_LOD_1,
  CM_PHYSOBJ_CHANGE_LOD_2,
  CM_PHYSOBJ_CHANGE_LOD_3,
  CM_PHYSOBJ_CHANGE_LOD_4,
  CM_PHYSOBJ_CHANGE_LOD_5,
  CM_PHYSOBJ_CHANGE_LOD_6,
  CM_PHYSOBJ_CHANGE_LOD_7,
  CM_PHYSOBJ_CHANGE_LOD_8,
  CM_PHYSOBJ_CHANGE_LOD_9,
};

namespace EditorCommandIds
{

static constexpr const char *PHYSOBJ_SIM_TOGGLE_DRIVER = "Plugin.PhysObj.Simulation.ToggleDriver";
static constexpr const char *PHYSOBJ_CHANGE_LOD_AUTO = "Plugin.PhysObj.ChangeLod.Auto";
static constexpr const char *PHYSOBJ_CHANGE_LOD_0 = "Plugin.PhysObj.ChangeLod.0";
static constexpr const char *PHYSOBJ_CHANGE_LOD_1 = "Plugin.PhysObj.ChangeLod.1";
static constexpr const char *PHYSOBJ_CHANGE_LOD_2 = "Plugin.PhysObj.ChangeLod.2";
static constexpr const char *PHYSOBJ_CHANGE_LOD_3 = "Plugin.PhysObj.ChangeLod.3";
static constexpr const char *PHYSOBJ_CHANGE_LOD_4 = "Plugin.PhysObj.ChangeLod.4";
static constexpr const char *PHYSOBJ_CHANGE_LOD_5 = "Plugin.PhysObj.ChangeLod.5";
static constexpr const char *PHYSOBJ_CHANGE_LOD_6 = "Plugin.PhysObj.ChangeLod.6";
static constexpr const char *PHYSOBJ_CHANGE_LOD_7 = "Plugin.PhysObj.ChangeLod.7";
static constexpr const char *PHYSOBJ_CHANGE_LOD_8 = "Plugin.PhysObj.ChangeLod.8";
static constexpr const char *PHYSOBJ_CHANGE_LOD_9 = "Plugin.PhysObj.ChangeLod.9";

} // namespace EditorCommandIds