//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EditorCore/ec_cm.h>

enum
{
  // menu items
  CM_FILE_OPEN_AND_LOCK = CM__LAST_USED_BY_CORE + 1,
  CM_FILE_REOPEN,
  CM_FILE_SAVE_AS,

  CM_FILE_EXPORT_TO_GAME_PC,
  CM_FILE_EXPORT_TO_GAME_XBOX360,
  CM_FILE_EXPORT_TO_GAME_PS3,
  CM_FILE_EXPORT_TO_GAME_PS4,
  CM_FILE_EXPORT_TO_GAME_iOS,
  CM_FILE_EXPORT_TO_GAME_AND,
  CM_FILE_EXPORT_TO_GAME_ALL,
  CM_FILE_EXPORT_TO_DAG,
  CM_FILE_EXPORT_COLLISION_TO_DAG,

  CM_BATCH_RUN,

  CM_FILE_SETTINGS,

  CM_LOAD_DEFAULT_LAYOUT,
  CM_LOAD_LAYOUT,
  CM_SAVE_LAYOUT,
  CM_OPTIONS_TOTAL,
  CM_OPTIONS_VERSION_CONTROL,
  CM_OPTIONS_ENABLE_DEVELOPER_TOOLS,
  CM_OPTIONS_PREFERENCES,
  CM_OPTIONS_PREFERENCES_ASSET_TREE,
  CM_OPTIONS_PREFERENCES_ASSET_TREE_COPY_SUBMENU,
  CM_OPTIONS_EDIT_PREFERENCES,
  CM_CUR_PLUGIN_VIS_CHANGE,

  CM_WINDOW_TOOLBAR,
  CM_WINDOW_TABBAR,
  CM_WINDOW_PLUGTOOLS,
  CM_WINDOW_VIEWPORT,
  CM_WINDOW_TAGMANAGER,

  CM_THEME,
  CM_THEME_LIGHT,
  CM_THEME_DARK,

  CM_HELP,
  CM_EDITOR_HELP,
  CM_PLUGIN_HELP,
  CM_TUTORIALS,
  CM_ABOUT,
  CM_ANIMATE_VIEWPORTS,
  CM_USE_OCCLUDERS,
  CM_VIEW_DEVELOPER_TOOLS,
  CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES,
  CM_VIEW_DEVELOPER_TOOLS_CONTROL_GALLERY,
  CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER,
  CM_VIEW_DEVELOPER_TOOLS_TOAST_MANAGER,
  CM_VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG,
  CM_VIEW_DEVELOPER_TOOLS_NODE_DEPS,
  CM_VIEW_DEVELOPER_TOOLS_CLIPMAP_DEBUG,

  CM_SWITCH_PLUGIN_F5,
  CM_SWITCH_PLUGIN_F6,
  CM_SWITCH_PLUGIN_F7,
  CM_SWITCH_PLUGIN_F8,
  CM_SWITCH_PLUGIN_SF1,
  CM_SWITCH_PLUGIN_SF2,
  CM_SWITCH_PLUGIN_SF3,
  CM_SWITCH_PLUGIN_SF4,
  CM_SWITCH_PLUGIN_SF5,
  CM_SWITCH_PLUGIN_SF6,
  CM_SWITCH_PLUGIN_SF7,
  CM_SWITCH_PLUGIN_SF8,
  CM_SWITCH_PLUGIN_SF9,
  CM_SWITCH_PLUGIN_SF10,
  CM_SWITCH_PLUGIN_SF11,
  CM_SWITCH_PLUGIN_SF12,

  CM_PLUGINS_MENU,
  CM_PLUGIN_PRIVATE_MENU,

  CM_PLUGINS_MENU_EDITOR,

  CM_PLUGINS_MENU_1ST,
  CM_PLUGINS_MENU__LAST = CM_PLUGINS_MENU_1ST + 100,

  CM_NEXT_PLUGIN,
  CM_PREV_PLUGIN,

  CM_DEBUG_FLUSH,

  CM_PLUGIN_BASE = 20000,
  CM_TAB_PRESS = CM_PLUGIN_BASE + 2000,
  CM_PLUGIN__LAST = CM_PLUGIN_BASE + 3000,

  CM_PLUGIN_SETTINGS_BASE = 30000,
};


namespace EditorCommandIds
{

static constexpr const char *DEBUG_FLUSH = "Main.DebugFlush";

static constexpr const char *FILE_SAVE = "Main.SaveProject";
static constexpr const char *FILE_SAVE_AS = "Main.SaveProjectAs";
static constexpr const char *FILE_OPEN = "Main.OpenProject";
static constexpr const char *FILE_REOPEN = "Main.ReopenCurrentProject";
static constexpr const char *FILE_EXPORT_TO_GAME_PC = "Main.ExportToGame";

static constexpr const char *UNDO = "Main.Undo";
static constexpr const char *REDO = "Main.Redo";
static constexpr const char *SELECT_ALL = "Main.SelectAll";
static constexpr const char *DESELECT_ALL = "Main.DeselectAll";
static constexpr const char *INVERT_SELECTION = "Main.InvertSelection";

static constexpr const char *VIEWPORT_LAYOUT_4 = "Main.ChangeViewport4";
static constexpr const char *VIEWPORT_LAYOUT_1 = "Main.ChangeViewport1";
static constexpr const char *LOAD_DEFAULT_LAYOUT = "Main.LoadDefaultLayout";
static constexpr const char *LOAD_LAYOUT = "Main.LoadLayout";
static constexpr const char *SAVE_LAYOUT = "Main.SaveLayout";
static constexpr const char *OPTIONS_TOTAL = "Main.OptionsTotal";
static constexpr const char *OPTIONS_GRID = "Main.EditGridSettings";
static constexpr const char *USE_OCCLUDERS = "Main.ToggleUseOccluders";
static constexpr const char *NAV_COMPASS = "Main.ToggleCompass";
static constexpr const char *SHOW_COLLISION = "Main.ShowCollision";
static constexpr const char *DISCARD_TEX_MODE = "Main.ToggleDiscardAssetTextures";
static constexpr const char *TOGGLE_TAG_MANAGER = "Main.ToggleTagManager";
static constexpr const char *VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES = "Main.View.ConsoleCommandsAndVariables";
static constexpr const char *VIEW_DEVELOPER_TOOLS_CONTROL_GALLERY = "Main.View.ControlGallery";
static constexpr const char *VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER = "Main.View.ImguiDebugger";
static constexpr const char *VIEW_DEVELOPER_TOOLS_TOAST_MANAGER = "Main.View.ToastManager";
static constexpr const char *VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG = "Main.View.TextureDebug";
static constexpr const char *VIEW_DEVELOPER_TOOLS_NODE_DEPS = "Main.View.NodeDependencies";
static constexpr const char *VIEW_DEVELOPER_TOOLS_CLIPMAP_DEBUG = "Main.View.ClipmapDebug";
static constexpr const char *CUR_PLUGIN_VIS_CHANGE = "Main.ToggleCurrentPluginVisibility";

static constexpr const char *BATCH_RUN = "Main.RunBatchFile";
static constexpr const char *FILE_EXPORT_TO_DAG = "Main.ExportSceneGeometryToDag";
static constexpr const char *FILE_EXPORT_COLLISION_TO_DAG = "Main.ExportSceneCollisionGeometryToDag";
static constexpr const char *CREATE_ORTHOGONAL_SCREENSHOT = "Main.CreateOrthogonalScreenshot";
static constexpr const char *UPSCALE_HEIGHTMAP = "Main.UpscaleHeightmapAndRebake";

static constexpr const char *OPTIONS_CAMERAS = "Main.Settings.Camera";
static constexpr const char *OPTIONS_SCREENSHOT = "Main.Settings.Screenshot";
static constexpr const char *OPTIONS_STAT_DISPLAY_SETTINGS = "Main.Settings.Stats";
static constexpr const char *FILE_SETTINGS = "Main.Settings.Project";
static constexpr const char *OPTIONS_EDIT_PREFERENCES = "Main.Settings.EditPreferences";

static constexpr const char *SWITCH_PLUGIN_F5 = "Main.SwitchPlugin.F5";
static constexpr const char *SWITCH_PLUGIN_F6 = "Main.SwitchPlugin.F6";
static constexpr const char *SWITCH_PLUGIN_F7 = "Main.SwitchPlugin.F7";
static constexpr const char *SWITCH_PLUGIN_F8 = "Main.SwitchPlugin.F8";
static constexpr const char *SWITCH_PLUGIN_SF1 = "Main.SwitchPlugin.ShiftF1";
static constexpr const char *SWITCH_PLUGIN_SF2 = "Main.SwitchPlugin.ShiftF2";
static constexpr const char *SWITCH_PLUGIN_SF3 = "Main.SwitchPlugin.ShiftF3";
static constexpr const char *SWITCH_PLUGIN_SF4 = "Main.SwitchPlugin.ShiftF4";
static constexpr const char *SWITCH_PLUGIN_SF5 = "Main.SwitchPlugin.ShiftF5";
static constexpr const char *SWITCH_PLUGIN_SF6 = "Main.SwitchPlugin.ShiftF6";
static constexpr const char *SWITCH_PLUGIN_SF7 = "Main.SwitchPlugin.ShiftF7";
static constexpr const char *SWITCH_PLUGIN_SF8 = "Main.SwitchPlugin.ShiftF8";
static constexpr const char *SWITCH_PLUGIN_SF9 = "Main.SwitchPlugin.ShiftF9";
static constexpr const char *SWITCH_PLUGIN_SF10 = "Main.SwitchPlugin.ShiftF10";
static constexpr const char *SWITCH_PLUGIN_SF11 = "Main.SwitchPlugin.ShiftF11";
static constexpr const char *SWITCH_PLUGIN_SF12 = "Main.SwitchPlugin.ShiftF12";
static constexpr const char *SWITCH_TO_PLUGIN = "Main.SwitchToPlugin."; // This is just a prefix.

static constexpr const char *NEXT_PLUGIN = "Main.NextPlugin";
static constexpr const char *PREV_PLUGIN = "Main.PreviousPlugin";

static constexpr const char *TAB_PRESS = "Main.TabPress";

} // namespace EditorCommandIds


namespace WindowIds
{

static constexpr const char *MAIN_SETTINGS_ENVIRONMENT = "Main.Settings.Environment";
static constexpr const char *MAIN_SETTINGS_EDIT_PREFERENCES = "Main.Settings.EditPreferences";

} // namespace WindowIds
