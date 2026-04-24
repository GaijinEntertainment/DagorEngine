// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_cm.h>

// commands
enum
{
  CM_IMPORT = CM_PLUGIN_BASE,
  CM_CLEAR_DAG_LIST,
  CM_COLLISION_SHOW_PROPS,
  CM_COMPILE_COLLISION,
  CM_COMPILE_GAME_COLLISION,
  CM_VIEW_DAG_LIST,
  CM_TOOL
};

namespace EditorCommandIds
{

static constexpr const char *IMPORT = "Plugin.Collision.AddCollisionFromDAG";
static constexpr const char *VIEW_DAG_LIST = "Plugin.Collision.ViewDAGList";
static constexpr const char *CLEAR_DAG_LIST = "Plugin.Collision.ClearDAGList";
static constexpr const char *COLLISION_SHOW_PROPS = "Plugin.Collision.TogglePropertiesPanel";
static constexpr const char *COMPILE_COLLISION = "Plugin.Collision.CompileCollision";
static constexpr const char *COMPILE_GAME_COLLISION = "Plugin.Collision.CompileGameCollision";

} // namespace EditorCommandIds