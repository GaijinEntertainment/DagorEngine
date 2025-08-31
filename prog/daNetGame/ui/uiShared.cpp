// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiShared.h"
#include "uiEvents.h"
#include "userUi.h"
#include "ui/overlay.h"
#include "ui/bhv/bhvUiStateControl.h"

#include "main/circuit.h"
#include "camera/sceneCam.h"

#include <daRg/dag_behavior.h>
#include <daRg/dag_browser.h>
#include <daRg/dag_joystick.h>
#include <daRg/dag_guiScene.h>

#include <ecs/core/entityManager.h>

#include <ioSys/dag_dataBlock.h>

#include <startup/dag_globalSettings.h>
#include <gui/dag_visualLog.h>
#include <gui/dag_stdGuiRender.h>
#include <3d/dag_picMgr.h>
#include <drv/3d/dag_decl.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiJoystick.h>
#include <startup/dag_inpDevClsDrv.h>

#include <daInput/input_api.h>
#include "input/globInput.h"

#include <gui/dag_imgui.h>
#include <imgui/imguiInput.h>

#include <sqrat.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <json/json.h>

#include <daEditorE/editorCommon/inGameEditor.h>
#include <daEditorE/editorCommon/entityEditor.h>


namespace uishared
{

static bool need_to_apply_resolution_change = false;
static bool apply_resolution_change_after_reset = false;
static bool need_initial_dev_type_notify = false;

static bool prev_apply_resolution_change_after_reset = false;

static int prev_gamepad_vendor_id = HumanInput::GAMEPAD_VENDOR_UNKNOWN;

static dainput::action_set_handle_t break_input_action_set_id = dainput::BAD_ACTION_SET_HANDLE;
static int break_input_depth = 0;
static bool break_input_action_set_is_active = false;

eastl::unique_ptr<darg::JoystickHandler> joystick_handler; // inited once at start


static Driver3dPerspective world_3d_persp;
TMatrix view_tm = TMatrix::ZERO;
TMatrix view_itm = TMatrix::ZERO;


static void apply_break_input_action_set()
{
  bool needStop = break_input_depth > 0;
  if (break_input_action_set_is_active != needStop)
  {
    break_input_action_set_is_active = needStop;
    dainput::activate_action_set(break_input_action_set_id, needStop);
  }
}

static void activate_user_ui_break_unput(bool set)
{
  if (set)
    ++break_input_depth;
  else
    --break_input_depth;
  G_ASSERT(break_input_depth >= 0);
  G_ASSERT(break_input_depth <= 50);
  apply_break_input_action_set();
}


void activate_ui_elem_action_set(dainput::action_set_handle_t ash, bool on)
{
  if (ash != dainput::BAD_ACTION_SET_HANDLE)
  {
    if (ash == break_input_action_set_id)
      activate_user_ui_break_unput(on);
    else
      dainput::activate_action_set(ash, on);
  }
}


static bool is_inited_once = false;
void init_early()
{
  G_ASSERT(!is_inited_once);
  is_inited_once = true;

  g_entity_mgr->broadcastEventImmediate(EventUiRegisterRendObjs{});

  darg::register_browser_rendobj_factories();

  joystick_handler.reset(new darg::JoystickHandler());

  visuallog::setFont("small_text");
  visuallog::setDrawDisabled(
    ::dgs_get_settings()->getBool("disableVisualLog", false) || circuit::get_conf()->getBool("disableVisualLog", false));
}


bool is_ui_available_in_build() { return true; }


void init()
{
  TIME_PROFILE(ui_shared_init);
  term();

  visuallog::setMaxItems(::dgs_get_settings()->getBlockByNameEx("debug")->getInt("visualLogItems", DEFAULT_VISUALLOG_MAX_ITEMS));

  apply_resolution_change_after_reset = false;
  need_initial_dev_type_notify = true;

  {
    dainput::action_set_handle_t ash = dainput::get_action_set_handle("StopInput");
    G_ASSERT(ash != dainput::BAD_ACTION_SET_HANDLE || dainput::get_actions_count() == 0);
    dainput::set_breaking_action_set(ash);
    break_input_action_set_id = ash;
  }

  imgui_register_global_input_handler(glob_input_process_top_level_key);
}


void term()
{
  // actually not needed, because called only on shutdown
  visuallog::reset();

  // actually not needed, because called only on shutdown
  apply_resolution_change_after_reset = false;
}


static void apply_resolution_change()
{
  overlay_ui::reload_scripts(true);
  user_ui::reload_user_ui_script(true);

  if (HumanInput::IGenPointing *mouse = global_cls_drv_pnt->getDevice(0))
    mouse->setClipRect(0, 0, StdGuiRender::screen_width(), StdGuiRender::screen_height());
}


void on_settings_changed(const FastNameMap &changed_fields, bool apply_after_device_reset)
{
  if (changed_fields.getNameId("video/monitor") >= 0 || changed_fields.getNameId("video/resolution") >= 0 ||
      changed_fields.getNameId("video/vsync") >= 0 || changed_fields.getNameId("video/nvidia_latency") >= 0 ||
      changed_fields.getNameId("video/amd_latency") >= 0 || changed_fields.getNameId("video/intel_latency") >= 0 ||
      changed_fields.getNameId("video/mode") >= 0 || changed_fields.getNameId("video/enableHdr") >= 0)
  {
    if (apply_after_device_reset)
      apply_resolution_change_after_reset = true;
    else
      need_to_apply_resolution_change = true;
  }
}

void window_resized() { apply_resolution_change_after_reset = true; }

void before_device_reset() { PictureManager::before_d3d_reset(); }


void after_device_reset()
{
  if (apply_resolution_change_after_reset)
  {
    apply_resolution_change_after_reset = false;
    apply_resolution_change();
  }
  PictureManager::after_d3d_reset();
}


static void notify_gamepad_type_change()
{
  int vendor_id = HumanInput::GAMEPAD_VENDOR_UNKNOWN;
  if (global_cls_drv_joy)
    vendor_id = global_cls_drv_joy->getVendorId();

  if (vendor_id != prev_gamepad_vendor_id && vendor_id != HumanInput::GAMEPAD_VENDOR_UNKNOWN)
  {
    prev_gamepad_vendor_id = vendor_id;
    Json::Value data;
    data["ctype"] = vendor_id;
    sqeventbus::send_event("input_gamepad_type", data);
  }
}


static void notify_input_devices_used()
{
  // detect and publicate to SQ changes in input devices used
  static unsigned last_mask = 0;
  static int last_type = 0;
  unsigned mask = dainput::get_last_used_device_mask();
  if ((mask && mask != last_mask) || (last_type && need_initial_dev_type_notify))
  {
    bool needEvent = need_initial_dev_type_notify;
    if (mask)
    {
      int type = (mask & (dainput::DEV_USED_mouse | dainput::DEV_USED_kbd | dainput::DEV_USED_touch)) ? 1 : 0;

#if _TARGET_ANDROID
      const bool isSensorInput = global_cls_drv_joy && global_cls_drv_joy->getVendorId() == HumanInput::GAMEPAD_VENDOR_UNKNOWN;
      if ((mask & dainput::DEV_USED_gamepad) && !isSensorInput)
#else
      if (mask & dainput::DEV_USED_gamepad)
#endif
      {
        type = 2;
      }

      last_mask = mask;
      if (last_type != type) //-V1051
        needEvent = true;
      last_type = type;
    }

    if (needEvent)
    {
      Json::Value msg;
      msg["val"] = last_type;
      sqeventbus::send_event("input_dev_used", msg);
      need_initial_dev_type_notify = false;
    }
  }
}


void update()
{
  TIME_PROFILE(ui_shared_update);

  if (need_to_apply_resolution_change)
  {
    need_to_apply_resolution_change = false;
    apply_resolution_change();
  }

  const bool isSceneFreeCam = !get_da_editor4().isActive() && is_free_camera_enabled();
  const bool isEditorFreeCam = get_da_editor4().isFreeCameraActive();
  const bool imGuiMustConsumeInput = imgui_get_state() == ImGuiState::ACTIVE && imgui_want_capture_mouse();
  const bool relativeDueToFreeCam = (isEditorFreeCam || isSceneFreeCam) && !BhvUiStateControl::overrideFreeCam;
  const bool setUiScenesActive = !relativeDueToFreeCam && !imGuiMustConsumeInput;

  bool forcedUserUiCursorActive = false;
  bool forcedOverlayCursorActive = false;

  const bool isUserUiCursorModeForced = user_ui::gui_scene && user_ui::gui_scene->getForcedCursorMode(forcedUserUiCursorActive);
  const bool isOverlayCursorModeForced =
    overlay_ui::gui_scene() && overlay_ui::gui_scene()->getForcedCursorMode(forcedOverlayCursorActive);

  if (overlay_ui::gui_scene())
    overlay_ui::gui_scene()->setSceneInputActive(setUiScenesActive || (isOverlayCursorModeForced && forcedOverlayCursorActive));

  if (user_ui::gui_scene)
    user_ui::gui_scene->setSceneInputActive(setUiScenesActive || (isUserUiCursorModeForced && forcedUserUiCursorActive));

  if (HumanInput::IGenPointing *mouse = global_cls_drv_pnt ? global_cls_drv_pnt->getDevice(0) : nullptr)
  {
    auto isSceneClickable = [](darg::IGuiScene *scene) -> bool {
      if (scene == nullptr)
        return false;

      bool forcedCursorMode = false;
      if (scene->getForcedCursorMode(forcedCursorMode))
        return forcedCursorMode;

      return scene->hasInteractiveElements(darg::Behavior::F_HANDLE_MOUSE);
    };

    const bool hasDargClickableGui = isSceneClickable(overlay_ui::gui_scene()) || isSceneClickable(user_ui::gui_scene.get());


    const bool absoluteForImGui = imgui_get_state() == ImGuiState::ACTIVE;

    const bool newRelative = (!hasDargClickableGui || relativeDueToFreeCam) && !absoluteForImGui;
    const bool isRelative = mouse->getRelativeMovementMode();

    // window rect for the cursor can be outdated at this point (driver mode reset is AFTER UI and cursor update),
    // capturing the cursor (with relative movement mode) can restrict the cursor on a different monitor,
    // hence we need to check for `apply_resolution_change_after_reset` in current and last frame
    if (prev_apply_resolution_change_after_reset || newRelative != isRelative)
    {
      mouse->setRelativeMovementMode(!apply_resolution_change_after_reset && newRelative);

      if (imgui_get_state() == ImGuiState::ACTIVE)
      {
        const IPoint2 &imguiSavedMousePos = imgui_get_saved_mouse_pos();
        mouse->setPosition(imguiSavedMousePos.x, imguiSavedMousePos.y);
      }
    }
  }

  prev_apply_resolution_change_after_reset = apply_resolution_change_after_reset; // 1 frame delay

  notify_gamepad_type_change();
  notify_input_devices_used();
}


void invalidate_world_3d_view()
{
  memset(&world_3d_persp, 0, sizeof(world_3d_persp));
  view_tm.zero();
  view_itm.zero();
}


void save_world_3d_view(const Driver3dPerspective &persp, const TMatrix &view_tm_, const TMatrix &view_itm_)
{
  G_ASSERT(persp.wk != 0.0f && persp.hk != 0.0f);
  world_3d_persp = persp;
  view_tm = view_tm_;
  view_itm = view_itm_;
}


bool has_valid_world_3d_view() { return world_3d_persp.wk != 0.0f; }


bool get_world_3d_persp(Driver3dPerspective &persp)
{
  persp = world_3d_persp;
  return has_valid_world_3d_view();
}

} // namespace uishared
