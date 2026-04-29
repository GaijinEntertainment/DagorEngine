// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_cm.h>

enum
{
  CM_ECS_EDITOR_CREATE_ENTITY = CM_PLUGIN_BASE + 1,
  CM_ECS_EDITOR_SET_PARENT,
  CM_ECS_EDITOR_CLEAR_PARENT,
  CM_ECS_EDITOR_TOGGLE_FREE_TRANSFORM,
  CM_ECS_EDITOR_SCENE_OUTLINER,
  CM_ECS_EDITOR_CURRENT_SCENE_TITLE,
  CM_ECS_EDITOR_CURRENT_SCENE_PATH,
  CM_ECS_EDITOR_LOAD_FROM_DEFAULT_LOCATION,
  CM_ECS_EDITOR_LOAD_FROM_CUSTOM_LOCATION,
  CM_ECS_EDITOR_RECENT_FILES_TITLE,
  CM_ECS_EDITOR_RECENT_FILE_START,                                      // inclusive
  CM_ECS_EDITOR_RECENT_FILE_END = CM_ECS_EDITOR_RECENT_FILE_START + 10, // non-inclusive
};

namespace EditorCommandIds
{

static constexpr const char *ECS_EDITOR_CREATE_ENTITY = "Plugin.ECSEditor.CreateEntity";
static constexpr const char *ECS_EDITOR_SET_PARENT = "Plugin.ECSEditor.SetParent";
static constexpr const char *ECS_EDITOR_CLEAR_PARENT = "Plugin.ECSEditor.ClearParent";
static constexpr const char *ECS_EDITOR_TOGGLE_FREE_TRANSFORM = "Plugin.ECSEditor.ToggleFreeTransform";
static constexpr const char *ECS_EDITOR_SCENE_OUTLINER = "Plugin.ECSEditor.SceneOutliner";
static constexpr const char *ECS_EDITOR_LOAD_FROM_DEFAULT_LOCATION = "Plugin.ECSEditor.LoadSceneFromDefaultPath";
static constexpr const char *ECS_EDITOR_LOAD_FROM_CUSTOM_LOCATION = "Plugin.ECSEditor.LoadSceneFromCustomPath";

} // namespace EditorCommandIds