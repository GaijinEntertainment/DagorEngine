// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum EditorCoreCM
{
  // menu items
  CM_FILE = 99,
  CM_FILE_NEW,
  CM_FILE_OPEN,
  CM_FILE_SAVE,

  CM_FILE_RECENT_BASE,
  CM_FILE_RECENT_LAST = CM_FILE_RECENT_BASE + 20,

  CM_VIEWPORT_LAYOUT_4,
  CM_VIEWPORT_LAYOUT_2HOR,
  CM_VIEWPORT_LAYOUT_2VERT,
  CM_VIEWPORT_LAYOUT_1,

  CM_VIEW,
  CM_VIEW_PERSPECTIVE,
  CM_VIEW_FRONT,
  CM_VIEW_BACK,
  CM_VIEW_TOP,
  CM_VIEW_BOTTOM,
  CM_VIEW_LEFT,
  CM_VIEW_RIGHT,
  CM_VIEW_CUSTOM_ORTHOGONAL,
  CM_VIEW_CUSTOM_CAMERA,
  CM_VIEW_WIREFRAME,
  CM_VIEW_VIEWPORT_AXIS,
  CM_VIEW_EDGED,
  CM_VIEW_GIZMO,
  CM_VIEW_GRID_SHOW,
  CM_VIEW_GRID_INC_STEP,
  CM_VIEW_GRID_DEC_STEP,
  CM_VIEW_GRID_MOVE_SNAP,
  CM_VIEW_GRID_ANGLE_SNAP,
  CM_VIEW_GRID_SCALE_SNAP,
  CM_VIEW_SHOW_STATS,
  CM_VIEW_STAT3D_OPAQUE,
  CM_VIEW_STAT3D_COLOR_HISTORGAM,
  CM_VIEW_STAT3D_BRIGHTNESS_HISTORGAM,

  CM_GROUP_GRID,
  CM_GROUP_CLIPPING,
  CM_GROUP_STATS,

  CM_CAMERAS_FREE,
  CM_CAMERAS_FPS,
  CM_CAMERAS_TPS,
  CM_CAMERAS_CAR,
  CM_CAMERA_MOVE,

  CM_PLUGIN_FIRST_CAMERA,
  CM_PLUGIN_LAST_CAMERA = CM_PLUGIN_FIRST_CAMERA + 200,

  CM_OPTIONS,
  CM_OPTIONS_CAMERAS,
  CM_OPTIONS_GRID,
  CM_OPTIONS_SCREENSHOT,
  CM_OPTIONS_STAT_DISPLAY_SETTINGS,

  CM_TOOLS,
  CM_CREATE_SCREENSHOT,
  CM_CREATE_CUBE_SCREENSHOT,
  CM_CREATE_ORTHOGONAL_SCREENSHOT,
  CM_UPSCALE_HEIGHTMAP,

  CM_EXIT,

  CM_EDIT,
  CM_UNDO,
  CM_REDO,
  CM_SELECT_ALL,
  CM_DESELECT_ALL,
  CM_INVERT_SELECTION,

  CM_GIZMO_X,
  CM_GIZMO_Y,
  CM_GIZMO_Z,
  CM_GIZMO_BASIS,
  CM_GIZMO_CENTER,

  CM_ROTATE_CENTER_AND_OBJ,

  CM_ZOOM_AND_CENTER,
  CM_NAVIGATE,
  CM_STATS,
  CM_CHANGE_VIEWPORT,
  CM_CHANGE_FOV,
  CM_ENVIRONMENT_SETTINGS,
  CM_CONSOLE,
  CM_PALETTE,

  CM_VR_ENABLE,

  CM_THEME_TOGGLE,

  CM_STATS_SETTINGS_TREE,
  CM_STATS_SETTINGS_STAT3D_GROUP,
  CM_STATS_SETTINGS_STAT3D_ITEM0,
  CM_STATS_SETTINGS_STAT3D_ITEM_LAST = CM_STATS_SETTINGS_STAT3D_ITEM0 + 100,
  CM_STATS_SETTINGS_CAMERA_GROUP,
  CM_STATS_SETTINGS_CAMERA_POS,
  CM_STATS_SETTINGS_CAMERA_DIST,
  CM_STATS_SETTINGS_CAMERA_FOV,
  CM_STATS_SETTINGS_FLY_CAMERA_GROUP,
  CM_STATS_SETTINGS_FLY_CAMERA_SPEED,
  CM_STATS_SETTINGS_FLY_CAMERA_TURBO_SPEED,

  // ObjectEditor commands. Put it between CM_OBJED_FIRST and CM_OBJED_LAST enum keys.
  CM_OBJED_FIRST,

  CM_OBJED_MODE_SELECT,
  CM_OBJED_MODE_MOVE,
  CM_OBJED_MODE_SURF_MOVE,
  CM_OBJED_MODE_ROTATE,
  CM_OBJED_MODE_SCALE,

  CM_OBJED_DROP,
  CM_OBJED_OBJPROP_PANEL,
  CM_OBJED_SELECT_BY_NAME,
  CM_OBJED_SELECT_FILTER,

  CM_OBJED_DELETE,
  CM_OBJED_CANCEL_GIZMO_TRANSFORM,

  CM_OBJED_LAST,

  CM_SAVE_ACTIVE_VIEW,
  CM_RESTORE_ACTIVE_VIEW,

  CM_SET_VIEWPORT_CLIPPING,
  CM_SET_DEFAULT_VIEWPORT_CLIPPING,

  CM__LAST_USED_BY_CORE
};


namespace EditorCommandIds
{

static constexpr const char *ZOOM_AND_CENTER = "Main.ZoomAndCenter";
static constexpr const char *ZOOM_AND_CENTER_IN_FLY_MODE = "Main.ZoomAndCenterInFlyMode";
static constexpr const char *NAVIGATE = "Main.NavigationHelp";
static constexpr const char *STATS = "Main.ShowLevelStatistics";
static constexpr const char *CHANGE_VIEWPORT = "Main.ChangeViewport";
static constexpr const char *CONSOLE = "Main.Console";
static constexpr const char *CAMERAS_FREE = "Main.Camera.Free";
static constexpr const char *CAMERAS_FPS = "Main.Camera.FPS";
static constexpr const char *CAMERAS_TPS = "Main.Camera.TPS";
static constexpr const char *CAMERAS_CAR = "Main.Camera.Car";
static constexpr const char *CHANGE_FOV = "Main.ChangeFov";
static constexpr const char *CREATE_SCREENSHOT = "Main.CreateScreenshot";
static constexpr const char *CREATE_CUBE_SCREENSHOT = "Main.CreateCubeScreenshot";
static constexpr const char *ENVIRONMENT_SETTINGS = "Main.EnvironmentSettings";
static constexpr const char *PALETTE = "Main.Palette";
static constexpr const char *VR_ENABLE = "Main.ToggleVR";
static constexpr const char *THEME_TOGGLE = "Main.ToggleTheme";

static constexpr const char *OBJED_SELECT_BY_NAME = "ObjectEditor.SelectByName";
static constexpr const char *OBJED_OBJPROP_PANEL = "ObjectEditor.TogglePropertiesPanel";
static constexpr const char *OBJED_MODE_SELECT = "ObjectEditor.Mode.Select";
static constexpr const char *OBJED_MODE_MOVE = "ObjectEditor.Mode.Move";
static constexpr const char *OBJED_MODE_SURF_MOVE = "ObjectEditor.Mode.SurfMove";
static constexpr const char *OBJED_MODE_ROTATE = "ObjectEditor.Mode.Rotate";
static constexpr const char *OBJED_MODE_SCALE = "ObjectEditor.Mode.Scale";
static constexpr const char *OBJED_DROP = "ObjectEditor.Drop";
static constexpr const char *OBJED_DELETE = "ObjectEditor.Delete";
static constexpr const char *OBJED_CANCEL_GIZMO_TRANSFORM = "ObjectEditor.CancelGizmoTransform";

static constexpr const char *VIEW_GRID_MOVE_SNAP = "Main.Grid.MoveSnap";
static constexpr const char *VIEW_GRID_ANGLE_SNAP = "Main.Grid.AngleSnap";
static constexpr const char *VIEW_GRID_SCALE_SNAP = "Main.Grid.ScaleSnap";

static constexpr const char *VIEW_GRID_SHOW = "Viewport.ToggleGrid";
static constexpr const char *VIEW_WIREFRAME = "Viewport.ToggleWireframe";
static constexpr const char *VIEW_PERSPECTIVE = "Viewport.View.Perspective";
static constexpr const char *VIEW_FRONT = "Viewport.View.Front";
static constexpr const char *VIEW_BACK = "Viewport.View.Back";
static constexpr const char *VIEW_TOP = "Viewport.View.Top";
static constexpr const char *VIEW_BOTTOM = "Viewport.View.Bottom";
static constexpr const char *VIEW_LEFT = "Viewport.View.Left";
static constexpr const char *VIEW_RIGHT = "Viewport.View.Right";

} // namespace EditorCommandIds

namespace WindowIds
{

static constexpr const char *MAIN_SETTINGS_CAMERA = "Main.Settings.Camera";
static constexpr const char *MAIN_SETTINGS_SCREENSHOT = "Main.Settings.Screenshot";
static constexpr const char *VIEWPORT_GRID_SETTINGS_PREFIX = "Viewport.GridSettings."; // Not the full ID.
static constexpr const char *VIEWPORT_STAT_SETTINGS_PREFIX = "Viewport.StatSettings."; // Not the full ID.
static constexpr const char *VIEWPORT_GIZMO_SETTINGS = "Viewport.GizmoSettings";

} // namespace WindowIds
