// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_cm.h"

enum
{
  CM_ENTITY_CHANGE_LOD_AUTO = CM_PLUGIN_BASE + 1,
  CM_ENTITY_CHANGE_LOD_0,
  CM_ENTITY_CHANGE_LOD_1,
  CM_ENTITY_CHANGE_LOD_2,
  CM_ENTITY_CHANGE_LOD_3,
  CM_ENTITY_CHANGE_LOD_4,
  CM_ENTITY_CHANGE_LOD_5,
  CM_ENTITY_CHANGE_LOD_6,
  CM_ENTITY_CHANGE_LOD_7,
  CM_ENTITY_CHANGE_LOD_8,
  CM_ENTITY_CHANGE_LOD_9,

  CM_COMPOSITE_EDITOR_ADD_NODE,
  CM_COMPOSITE_EDITOR_ADD_RANDOM_ENTITY,
  CM_COMPOSITE_EDITOR_CHANGE_ASSET,
  CM_COMPOSITE_EDITOR_OPEN_ASSET,
  CM_COMPOSITE_EDITOR_COPY_ASSET_FILEPATH_TO_CLIPBOARD,
  CM_COMPOSITE_EDITOR_COPY_ASSET_NAME_TO_CLIPBOARD,
  CM_COMPOSITE_EDITOR_DELETE_NODE,
  CM_COMPOSITE_EDITOR_REVEAL_ASSET_IN_EXPLORER,

  CM_COMPOSITE_EDITOR_COPY_ASSET,
  CM_COMPOSITE_EDITOR_PASTE_ASSET,
  CM_COMPOSITE_EDITOR_DUPLICATE_ASSET,

  CM_COMPOSITE_EDITOR_MAKE_PARENT,
  CM_COMPOSITE_EDITOR_CLEAR_PARENT,

  CM_COMPOSITE_EDITOR_EDIT_SUB_COMPOSITE,
  CM_COMPOSITE_EDITOR_EXIT_SUB_COMPOSITE,

  CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE,
  CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE_UNIQUE,
  CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT,
};

namespace EditorCommandIds
{

static constexpr const char *ENTITY_CHANGE_LOD_AUTO = "Plugin.Entity.ChangeLod.Auto";
static constexpr const char *ENTITY_CHANGE_LOD_0 = "Plugin.Entity.ChangeLod.0";
static constexpr const char *ENTITY_CHANGE_LOD_1 = "Plugin.Entity.ChangeLod.1";
static constexpr const char *ENTITY_CHANGE_LOD_2 = "Plugin.Entity.ChangeLod.2";
static constexpr const char *ENTITY_CHANGE_LOD_3 = "Plugin.Entity.ChangeLod.3";
static constexpr const char *ENTITY_CHANGE_LOD_4 = "Plugin.Entity.ChangeLod.4";
static constexpr const char *ENTITY_CHANGE_LOD_5 = "Plugin.Entity.ChangeLod.5";
static constexpr const char *ENTITY_CHANGE_LOD_6 = "Plugin.Entity.ChangeLod.6";
static constexpr const char *ENTITY_CHANGE_LOD_7 = "Plugin.Entity.ChangeLod.7";
static constexpr const char *ENTITY_CHANGE_LOD_8 = "Plugin.Entity.ChangeLod.8";
static constexpr const char *ENTITY_CHANGE_LOD_9 = "Plugin.Entity.ChangeLod.9";

static constexpr const char *ENTITY_CREATE_NODE = "Plugin.Entity.CreateNode";

static constexpr const char *ENTITY_COPY_ASSET = "Plugin.Entity.CopyAsset";
static constexpr const char *ENTITY_PASTE_ASSET = "Plugin.Entity.PasteAsset";
static constexpr const char *ENTITY_DUPLICATE_ASSET = "Plugin.Entity.DuplicateAsset";

static constexpr const char *ENTITY_MAKE_PARENT = "Plugin.Entity.MakeParent";
static constexpr const char *ENTITY_CLEAR_PARENT = "Plugin.Entity.ClearParent";

} // namespace EditorCommandIds
