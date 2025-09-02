// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiInput.h"
#include "globInput.h"

#include "ui/userUi.h"
#include "ui/uiShared.h"

#include <daEditorE/editorCommon/inGameEditor.h>
#include "ui/overlay.h"

#include <daRg/dag_guiScene.h>
#include <daRg/dag_joystick.h>
#include <generic/dag_relocatableFixedVector.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <gui/dag_visConsole.h>
#include <daInput/input_api.h>
#include <memory/dag_framemem.h>


namespace uiinput
{


#define ADD_KEY(KEY) {dainput::DEV_kbd, HumanInput::KEY}
static dainput::SingleButtonId keys_disable_by_console[] = {
  ADD_KEY(DKEY_1),
  ADD_KEY(DKEY_2),
  ADD_KEY(DKEY_3),
  ADD_KEY(DKEY_4),
  ADD_KEY(DKEY_5),
  ADD_KEY(DKEY_6),
  ADD_KEY(DKEY_7),
  ADD_KEY(DKEY_8),
  ADD_KEY(DKEY_9),
  ADD_KEY(DKEY_0),
  ADD_KEY(DKEY_MINUS),
  ADD_KEY(DKEY_EQUALS),
  ADD_KEY(DKEY_BACK),
  ADD_KEY(DKEY_Q),
  ADD_KEY(DKEY_W),
  ADD_KEY(DKEY_E),
  ADD_KEY(DKEY_R),
  ADD_KEY(DKEY_T),
  ADD_KEY(DKEY_Y),
  ADD_KEY(DKEY_U),
  ADD_KEY(DKEY_I),
  ADD_KEY(DKEY_O),
  ADD_KEY(DKEY_P),
  ADD_KEY(DKEY_LBRACKET),
  ADD_KEY(DKEY_RBRACKET),
  ADD_KEY(DKEY_RETURN),
  ADD_KEY(DKEY_A),
  ADD_KEY(DKEY_S),
  ADD_KEY(DKEY_D),
  ADD_KEY(DKEY_F),
  ADD_KEY(DKEY_G),
  ADD_KEY(DKEY_H),
  ADD_KEY(DKEY_J),
  ADD_KEY(DKEY_K),
  ADD_KEY(DKEY_L),
  ADD_KEY(DKEY_SEMICOLON),
  ADD_KEY(DKEY_APOSTROPHE),
  ADD_KEY(DKEY_GRAVE),
  ADD_KEY(DKEY_LSHIFT),
  ADD_KEY(DKEY_BACKSLASH),
  ADD_KEY(DKEY_Z),
  ADD_KEY(DKEY_X),
  ADD_KEY(DKEY_C),
  ADD_KEY(DKEY_V),
  ADD_KEY(DKEY_B),
  ADD_KEY(DKEY_N),
  ADD_KEY(DKEY_M),
  ADD_KEY(DKEY_COMMA),
  ADD_KEY(DKEY_PERIOD),
  ADD_KEY(DKEY_SLASH),
  ADD_KEY(DKEY_RSHIFT),
  ADD_KEY(DKEY_MULTIPLY),
  ADD_KEY(DKEY_SPACE),
  ADD_KEY(DKEY_CAPITAL),
  ADD_KEY(DKEY_NUMPAD7),
  ADD_KEY(DKEY_NUMPAD8),
  ADD_KEY(DKEY_NUMPAD9),
  ADD_KEY(DKEY_SUBTRACT),
  ADD_KEY(DKEY_NUMPAD4),
  ADD_KEY(DKEY_NUMPAD5),
  ADD_KEY(DKEY_NUMPAD6),
  ADD_KEY(DKEY_ADD),
  ADD_KEY(DKEY_NUMPAD1),
  ADD_KEY(DKEY_NUMPAD2),
  ADD_KEY(DKEY_NUMPAD3),
  ADD_KEY(DKEY_NUMPAD0),
  ADD_KEY(DKEY_DECIMAL),
  ADD_KEY(DKEY_NUMPADCOMMA),
  ADD_KEY(DKEY_DIVIDE),
  ADD_KEY(DKEY_HOME),
  ADD_KEY(DKEY_LEFT),
  ADD_KEY(DKEY_RIGHT),
  ADD_KEY(DKEY_END),
  ADD_KEY(DKEY_INSERT),
  ADD_KEY(DKEY_DELETE),

  ADD_KEY(DKEY_ESCAPE),
  ADD_KEY(DKEY_LCONTROL),
  ADD_KEY(DKEY_TAB),
  ADD_KEY(DKEY_UP),
  ADD_KEY(DKEY_PRIOR),
  ADD_KEY(DKEY_DOWN),
  ADD_KEY(DKEY_NEXT),
};
#undef ADD_KEY

dag::ConstSpan<dainput::SingleButtonId> get_keys_consumed_by_console()
{
  return make_span_const(keys_disable_by_console, countof(keys_disable_by_console));
}

dag::ConstSpan<dainput::SingleButtonId> get_keys_consumed_by_editbox()
{
  return make_span_const(keys_disable_by_console, countof(keys_disable_by_console) - 7);
}

void mask_dainput_buttons(dag::ConstSpan<darg::HotkeyButton> buttons, bool consume_text_input, int layer)
{
  Tab<dainput::SingleButtonId> keys_disable(framemem_ptr());
  dag::ConstSpan<dainput::SingleButtonId> editbox_keys = get_keys_consumed_by_editbox();

  keys_disable.reserve(buttons.size() + (consume_text_input ? editbox_keys.size() : 0));
  if (consume_text_input)
    append_items(keys_disable, editbox_keys.size(), editbox_keys.data());
  for (const darg::HotkeyButton &hk : buttons)
    keys_disable.push_back(dainput::SingleButtonId{(uint16_t)hk.devId, (uint16_t)hk.btnId});

  dainput::set_initial_usage_mask(keys_disable, layer);
}

void mask_dainput_pointers(int iflags, int layer)
{
  using namespace HumanInput;
  using namespace darg;

  // Mask mouse and gamepad stick

  Tab<dainput::SingleButtonId> keys_disable;
  keys_disable.reserve((iflags & IGuiSceneCallback::IF_MOUSE) ? 7 + 4 + 2 + 6 : 0);
  if (iflags & IGuiSceneCallback::IF_MOUSE)
  {
    for (uint16_t i = 0; i <= 6; i++)
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_pointing, i}); // 7 mouse buttons
    for (uint16_t i = 0; i < 4; i++)
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_pointing | 0x10, i}); // all mouse axes
    // dirpad navigation
    if (iflags & IGuiSceneCallback::IF_DIRPAD_NAV)
    {
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad, JOY_XINPUT_REAL_BTN_D_UP});
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad, JOY_XINPUT_REAL_BTN_D_DOWN});
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad, JOY_XINPUT_REAL_BTN_D_LEFT});
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad, JOY_XINPUT_REAL_BTN_D_RIGHT});
    }
    if (iflags & IGuiSceneCallback::IF_GAMEPAD_AS_MOUSE)
    {
      for (uint16_t i = 0; i < 2; i++)
        keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad | 0x10, i}); // left gamepad stick
      // gamepad clicks
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad, JOY_XINPUT_REAL_BTN_A});
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad, JOY_XINPUT_REAL_BTN_B});
    }
  }
  if (iflags & IGuiSceneCallback::IF_GAMEPAD_STICKS)
  {
    for (uint16_t i = 0; i < 4; i++)
      keys_disable.push_back(dainput::SingleButtonId{dainput::DEV_gamepad | 0x10, i}); // both gamepad sticks
  }
  dainput::set_initial_usage_mask(keys_disable, layer);
}

static struct ConsoleAndGlobalMouseKbdHandler : public ecs::IGenHidEventHandler
{
  virtual bool ehIsActive() const { return true; }

  virtual bool gmehMove(const Context &, float, float) { return false; }
  virtual bool gmehButtonDown(const Context &, int) { return false; }
  virtual bool gmehButtonUp(const Context &, int) { return false; }
  virtual bool gmehWheel(const Context &, int) { return false; }
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wc)
  {
    bool prev_con = console::is_visible();
    if (console::get_visual_driver() && console::get_visual_driver()->processKey(btn_idx, wc, true))
    {
      if (console::is_visible() && !prev_con)
        dainput::set_initial_usage_mask(get_keys_consumed_by_console(), int(DaInputMaskLayers::Console));
      else if (!console::is_visible() && prev_con)
        dainput::set_initial_usage_mask(dag::ConstSpan<dainput::SingleButtonId>(), int(DaInputMaskLayers::Console));
      return true;
    }
    if (!repeat)
      return ::glob_input_process_top_level_key(true, btn_idx, ctx.keyModif);
    return false;
  }
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx)
  {
    if (console::is_visible())
      return true;
    return ::glob_input_process_top_level_key(false, btn_idx, ctx.keyModif);
  }
  virtual bool gjehStateChanged(const Context &ctx, HumanInput::IGenJoystick *joy, int joy_ord_id)
  {
    G_UNUSED(ctx);
    G_UNUSED(joy_ord_id);
    return ::glob_input_process_controller(joy);
  }

} console_eh;


MouseKbdHandler::MouseKbdHandler()
{
  register_hid_event_handler(this, 25);
  // #if DAGOR_DBGLEVEL>0//temporary, until we will figure out what console command leave to players and what not. Also, it should not
  // be that easy to call console
  register_hid_event_handler(&console_eh, 100);
  // #endif
}


MouseKbdHandler::~MouseKbdHandler()
{
  unregister_hid_event_handler(this);
  // #if DAGOR_DBGLEVEL>0//temporary, until we will figure out what console command leave to players and what not. Also, it should not
  // be that easy to call console
  unregister_hid_event_handler(&console_eh);
  // #endif
}

using all_darg_scenes_t = dag::RelocatableFixedVector<darg::IGuiScene *, 2, true, framemem_allocator>;
static all_darg_scenes_t get_all_scenes()
{
  all_darg_scenes_t scenes;
  if (auto oscn = overlay_ui::gui_scene())
    scenes.push_back(oscn);
  if (auto gscn = user_ui::gui_scene.get())
    scenes.push_back(gscn);
  return scenes;
}


bool MouseKbdHandler::gkcLocksChanged(const Context &, unsigned locks)
{
  for (darg::IGuiScene *scene : get_all_scenes())
    scene->onKeyboardLocksChanged(locks);
  return false; // this event should be handled by all handlers
}


bool MouseKbdHandler::gkcLayoutChanged(const Context &, const char *layout)
{
  for (darg::IGuiScene *scene : get_all_scenes())
    scene->onKeyboardLayoutChanged(layout);
  return false; // this event should be handled by all handlers
}


bool MouseKbdHandler::gmehMove(const Context &ctx, float /*dx*/, float /*dy*/)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onMouseEvent(darg::INP_EV_POINTER_MOVE, 0, ctx.msX, ctx.msY, ctx.msButtons, resFlags);
  return resFlags != 0;
}


bool MouseKbdHandler::gmehButtonDown(const Context &ctx, int btn_idx)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onMouseEvent(darg::INP_EV_PRESS, btn_idx, ctx.msX, ctx.msY, ctx.msButtons, resFlags);
  return resFlags != 0;
}


bool MouseKbdHandler::gmehButtonUp(const Context &ctx, int btn_idx)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onMouseEvent(darg::INP_EV_RELEASE, btn_idx, ctx.msX, ctx.msY, ctx.msButtons, resFlags);
  return resFlags != 0;
}


bool MouseKbdHandler::gmehWheel(const Context &ctx, int scroll)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onMouseEvent(darg::INP_EV_MOUSE_WHEEL, scroll, ctx.msX, ctx.msY, ctx.msButtons, resFlags);
  return resFlags != 0;
}


bool MouseKbdHandler::gtehTouchBegan(
  const Context &, HumanInput::IGenPointing *m, int touch_idx, const HumanInput::PointingRawState::Touch &touch)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onTouchEvent(darg::INP_EV_PRESS, m, touch_idx, touch, resFlags);
  return resFlags != 0;
}


bool MouseKbdHandler::gtehTouchMoved(
  const Context &, HumanInput::IGenPointing *m, int touch_idx, const HumanInput::PointingRawState::Touch &touch)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onTouchEvent(darg::INP_EV_POINTER_MOVE, m, touch_idx, touch, resFlags);
  return resFlags != 0;
}


bool MouseKbdHandler::gtehTouchEnded(
  const Context &, HumanInput::IGenPointing *m, int touch_idx, const HumanInput::PointingRawState::Touch &touch)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
    resFlags |= scene->onTouchEvent(darg::INP_EV_RELEASE, m, touch_idx, touch);
  return resFlags != 0;
}


bool MouseKbdHandler::gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wc)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
  {
    resFlags |= scene->onServiceKbdEvent(darg::INP_EV_PRESS, btn_idx, ctx.keyModif, repeat, wc);
    resFlags |= scene->onKbdEvent(darg::INP_EV_PRESS, btn_idx, ctx.keyModif, repeat, wc);
  }

  return resFlags != 0;
}


bool MouseKbdHandler::gkehButtonUp(const Context &ctx, int btn_idx)
{
  int resFlags = 0;
  for (darg::IGuiScene *scene : get_all_scenes())
  {
    resFlags |= scene->onServiceKbdEvent(darg::INP_EV_RELEASE, btn_idx, ctx.keyModif, false, 0, resFlags);
    resFlags |= scene->onKbdEvent(darg::INP_EV_RELEASE, btn_idx, ctx.keyModif, false, 0, resFlags);
  }

  return resFlags != 0;
}


bool MouseKbdHandler::gjehStateChanged(const Context &, HumanInput::IGenJoystick *joy, int joy_ord_id)
{
  if (uishared::joystick_handler) // check for null since this may be called befor ui is initialized
    uishared::joystick_handler->onJoystickStateChanged(get_all_scenes(), joy, joy_ord_id); // need to return result?
  return false;
}


void update_joystick_input() { uishared::joystick_handler->processPendingBtnStack(get_all_scenes()); }

} // namespace uiinput
