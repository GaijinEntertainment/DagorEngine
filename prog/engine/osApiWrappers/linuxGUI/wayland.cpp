// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <wayland/wayland-client.h>
#include <wayland/xdg-shell-client-protocol.h>
#include <wayland/viewporter-client-protocol.h>
#include <wayland/relative-pointer-unstable-v1-client-protocol.h>
#include <wayland/pointer-constraints-unstable-v1-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/setProgGlobals.h>
#include <math/dag_lsbVisitor.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_bits.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <perfMon/dag_statDrv.h>
#include "wayland.h"

namespace workcycle_internal
{
extern intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
}

namespace
{

// #define DUMP_HANDLE() debug("%s", __FUNCTION__)
#define DUMP_HANDLE()

const char *mime_text_utf8 = "text/plain;charset=utf-8";

static uint32_t utf32symb_to_utf16symb(uint32_t sym)
{
  if (sym >= 0x10000)
  {
    uint32_t t = sym - 0x10000;
    uint32_t h = (((t << 12) >> 22) + 0xD800);
    uint32_t l = (((t << 22) >> 22) + 0xDC00);
    sym = ((h << 16) | (l & 0x0000FFFF));
  }
  return sym;
}

static void handle_wl_registry_global(void *data, wl_registry *, uint32_t name, const char *interface, uint32_t version)
{
  DUMP_HANDLE();
  debug("interface: '%s', version: %d, name: %d\n", interface, version, name);
  Wayland::InputGlobal global{name, interface, version};
  ((Wayland *)data)->cbRegistryGlobal(global);
}

static void handle_wl_handle_global_remove(void *data, wl_registry *, uint32_t name)
{
  DUMP_HANDLE();
  ((Wayland *)data)->cbRegistryGlobalRemove(name);
}

static void handle_wl_surface_enter(void *data, wl_surface *, wl_output *output)
{
  DUMP_HANDLE();
  Wayland::Window &info = *((Wayland::Window *)data);
  Wayland::Output *oi = info.parent->outputs.find(output);
  info.cbEnterOutput(oi);
}

static void handle_wl_surface_leave(void *data, wl_surface *, wl_output *output)
{
  DUMP_HANDLE();
  Wayland::Window &info = *((Wayland::Window *)data);
  Wayland::Output *oi = info.parent->outputs.find(output);
  info.cbLeaveOutput(oi);
}


static void handle_xdg_surface_configure(void *data, xdg_surface *xdg_surface, uint32_t serial)
{
  DUMP_HANDLE();
  Wayland::Window &info = *((Wayland::Window *)data);
  info.cbConfigure();
  xdg_surface_ack_configure(xdg_surface, serial);
}

static void handle_xdg_wm_base_ping(void *, xdg_wm_base *xdg_wm_base, uint32_t serial)
{
  DUMP_HANDLE();
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static void handle_wl_output_geometry(void *data, wl_output *, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
  int32_t subpixel, const char *make, const char *model, int32_t transform)
{
  DUMP_HANDLE();
  Wayland::Output &info = *((Wayland::Output *)data);
  info.x = x;
  info.y = y;
  info.physical_width = physical_width;
  info.physical_height = physical_height;
  info.subpixel = subpixel;
  info.manufacturer.setStr(make);
  info.model.setStr(model);
  info.transform = transform;
}

static void handle_wl_output_mode(void *data, wl_output *, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
  DUMP_HANDLE();
  Wayland::Output &info = *((Wayland::Output *)data);
  Wayland::Output::Mode mode = {width, height, refresh};
  if (flags & wl_output_mode::WL_OUTPUT_MODE_CURRENT)
    info.mode.current = mode;
  if (flags & wl_output_mode::WL_OUTPUT_MODE_PREFERRED)
    info.mode.preferred = mode;
}

static void handle_wl_output_done(void *data, wl_output *)
{
  DUMP_HANDLE();
  Wayland::Output &info = *((Wayland::Output *)data);
  info.dumpToLog();
}

static void handle_wl_output_name(void *data, wl_output *, const char *name)
{
  DUMP_HANDLE();
  Wayland::Output &info = *((Wayland::Output *)data);
  info.name.setStr(name);
}

static void handle_wl_output_description(void *data, wl_output *, const char *description)
{
  DUMP_HANDLE();
  Wayland::Output &info = *((Wayland::Output *)data);
  info.desc.setStr(description);
}

static void handle_wl_output_scale(void *data, wl_output *, int32_t factor)
{
  DUMP_HANDLE();
  Wayland::Output &info = *((Wayland::Output *)data);
  info.scaleFactor = factor;
}

static void handle_xdg_toplevel_configure(void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *)
{
  DUMP_HANDLE();
  Wayland::Window &info = *((Wayland::Window *)data);
  info.reportedWidth = width;
  info.reportedHeight = height;
}

static void handle_xdg_toplevel_close(void *data, xdg_toplevel *)
{
  DUMP_HANDLE();
  Wayland::Window &info = *((Wayland::Window *)data);
  info.cbClose();
}

static void handle_xdg_toplevel_configure_bounds(void *data, xdg_toplevel *, int32_t width, int32_t height)
{
  DUMP_HANDLE();
  Wayland::Window &info = *((Wayland::Window *)data);
  info.boundsWidth = width;
  info.boundsHeight = height;
}

static void handle_xdg_toplevel_wm_capabilities(void *, xdg_toplevel *, wl_array *) { DUMP_HANDLE(); }

static void handle_wl_seat_capabilities(void *data, wl_seat *, uint32_t capabilities)
{
  DUMP_HANDLE();
  Wayland::Seat &info = *((Wayland::Seat *)data);
  info.cbSeatCaps(capabilities);
}

static void handle_wl_seat_name(void *data, wl_seat *, const char *name)
{
  DUMP_HANDLE();
  Wayland::Seat &info = *((Wayland::Seat *)data);
  info.name.setStr(name);
}

static void handle_wl_pointer_enter(void *data, wl_pointer *, uint32_t serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbPointerFocus(surface, x, y, true, serial);
}

static void handle_wl_pointer_leave(void *data, wl_pointer *, uint32_t, wl_surface *surface)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbPointerFocus(surface, 0, 0, false, 0);
}

static void handle_wl_pointer_motion(void *data, wl_pointer *, uint32_t, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbSetPointerPos(surface_x, surface_y);
}

static void handle_wl_pointer_button(void *data, wl_pointer *, uint32_t, uint32_t, uint32_t button, uint32_t state)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbSetPointerButton(button, state);
}

static void handle_wl_pointer_axis(void *data, wl_pointer *, uint32_t, uint32_t axis, wl_fixed_t value)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbAxis(axis, value);
}

static void handle_wl_pointer_frame(void *data, wl_pointer *)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbFrame();
}

static void handle_wl_pointer_axis_source(void *data, wl_pointer *, uint32_t axis_source)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbAxisSrc(axis_source);
}

static void handle_wl_pointer_axis_stop(void *, wl_pointer *, uint32_t, uint32_t) { DUMP_HANDLE(); }
static void handle_wl_pointer_axis_discrete(void *, wl_pointer *, uint32_t, int32_t) { DUMP_HANDLE(); }
static void handle_wl_pointer_axis_value120(void *, wl_pointer *, uint32_t, int32_t) { DUMP_HANDLE(); }
static void handle_wl_pointer_axis_relative_direction(void *, wl_pointer *, uint32_t, uint32_t) { DUMP_HANDLE(); }

static void handle_wl_keyboard_keymap(void *data, wl_keyboard *, uint32_t format, int32_t fd, uint32_t size)
{
  DUMP_HANDLE();
  Wayland::Keyboard &info = *((Wayland::Keyboard *)data);
  info.cbKeymap(format, fd, size);
}

static void handle_wl_keyboard_enter(void *data, wl_keyboard *, uint32_t serial, wl_surface *surface, wl_array *keys)
{
  DUMP_HANDLE();
  Wayland::Keyboard &info = *((Wayland::Keyboard *)data);
  info.cbEnter(surface, keys, serial);
}

static void handle_wl_keyboard_leave(void *data, wl_keyboard *, uint32_t, wl_surface *surface)
{
  DUMP_HANDLE();
  Wayland::Keyboard &info = *((Wayland::Keyboard *)data);
  info.cbLeave(surface);
}

static void handle_wl_keyboard_key(void *data, wl_keyboard *, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
  DUMP_HANDLE();
  Wayland::Keyboard &info = *((Wayland::Keyboard *)data);
  info.cbKey(key, state, time, serial);
}

static void handle_wl_keyboard_modifiers(void *data, wl_keyboard *, uint32_t, uint32_t mods_depressed, uint32_t mods_latched,
  uint32_t mods_locked, uint32_t group)
{
  DUMP_HANDLE();
  Wayland::Keyboard &info = *((Wayland::Keyboard *)data);
  info.cbModifier(mods_depressed, mods_latched, mods_locked, group);
}

static void handle_wl_keyboard_repeat_info(void *data, wl_keyboard *, int32_t rate, int32_t delay)
{
  DUMP_HANDLE();
  Wayland::Keyboard &info = *((Wayland::Keyboard *)data);
  info.cbRepeatInfo(delay, rate);
}

static void handle_zwp_relative_pointer_v1_relative_motion(void *data, zwp_relative_pointer_v1 *, uint32_t, uint32_t, wl_fixed_t dx,
  wl_fixed_t dy, wl_fixed_t, wl_fixed_t)
{
  DUMP_HANDLE();
  Wayland::Pointer &info = *((Wayland::Pointer *)data);
  info.cbRelativeMotion(dx, dy);
}

static void handle_wl_data_device_data_offer(void *data, wl_data_device *, wl_data_offer *offer)
{
  DUMP_HANDLE();
  Wayland::Seat &info = *((Wayland::Seat *)data);
  info.cbNewOffer(offer);
}

static void handle_wl_data_device_enter(void *, wl_data_device *, uint32_t, wl_surface *, wl_fixed_t, wl_fixed_t, wl_data_offer *)
{
  DUMP_HANDLE();
}

static void handle_wl_data_device_leave(void *, wl_data_device *) { DUMP_HANDLE(); }
static void handle_wl_data_device_motion(void *, wl_data_device *, uint32_t, wl_fixed_t, wl_fixed_t) { DUMP_HANDLE(); }
static void handle_wl_data_device_drop(void *, wl_data_device *) { DUMP_HANDLE(); }

static void handle_wl_data_device_selection(void *data, wl_data_device *, wl_data_offer *offer)
{
  DUMP_HANDLE();
  Wayland::Seat &info = *((Wayland::Seat *)data);
  info.cbSelection(offer);
}

static void handle_wl_data_offer_offer(void *data, wl_data_offer *offer, const char *mime)
{
  DUMP_HANDLE();
  Wayland::Seat &info = *((Wayland::Seat *)data);
  info.cbOfferType(offer, mime);
}

static void handle_wl_data_offer_source_actions(void *, wl_data_offer *, uint32_t) { DUMP_HANDLE(); }
static void handle_wl_data_offer_action(void *, wl_data_offer *, uint32_t) { DUMP_HANDLE(); }
static void handle_wl_data_source_target(void *, wl_data_source *, const char *) { DUMP_HANDLE(); }

static void handle_wl_data_source_send(void *data, wl_data_source *wl_data_source, const char *mime_type, int32_t fd)
{
  DUMP_HANDLE();
  Wayland::Seat &info = *((Wayland::Seat *)data);
  G_UNUSED(wl_data_source);
  G_ASSERT(wl_data_source == info.outboundClipboard);
  info.cbSendData(mime_type, fd);
}

static void handle_wl_data_source_cancelled(void *, wl_data_source *) { DUMP_HANDLE(); }
static void handle_wl_data_source_dnd_drop_performed(void *, wl_data_source *) { DUMP_HANDLE(); }
static void handle_wl_data_source_dnd_finished(void *, wl_data_source *) { DUMP_HANDLE(); }
static void handle_wl_data_source_action(void *, wl_data_source *, uint32_t) { DUMP_HANDLE(); }

struct Listeners
{
  struct
  {
    wl_registry_listener registry = {
      .global = handle_wl_registry_global,
      .global_remove = handle_wl_handle_global_remove,
    };
    wl_surface_listener surface = {
      .enter = handle_wl_surface_enter,
      .leave = handle_wl_surface_leave,
    };
    wl_output_listener output = {
      .geometry = handle_wl_output_geometry,
      .mode = handle_wl_output_mode,
      .done = handle_wl_output_done,
      .scale = handle_wl_output_scale,
      .name = handle_wl_output_name,
      .description = handle_wl_output_description,
    };
    wl_seat_listener seat = {.capabilities = handle_wl_seat_capabilities, .name = handle_wl_seat_name};
    wl_pointer_listener pointer = {
      .enter = handle_wl_pointer_enter,
      .leave = handle_wl_pointer_leave,
      .motion = handle_wl_pointer_motion,
      .button = handle_wl_pointer_button,
      .axis = handle_wl_pointer_axis,
      .frame = handle_wl_pointer_frame,
      .axis_source = handle_wl_pointer_axis_source,
      .axis_stop = handle_wl_pointer_axis_stop,
      .axis_discrete = handle_wl_pointer_axis_discrete,
      .axis_value120 = handle_wl_pointer_axis_value120,
      .axis_relative_direction = handle_wl_pointer_axis_relative_direction,
    };
    wl_keyboard_listener keyboard = {
      .keymap = handle_wl_keyboard_keymap,
      .enter = handle_wl_keyboard_enter,
      .leave = handle_wl_keyboard_leave,
      .key = handle_wl_keyboard_key,
      .modifiers = handle_wl_keyboard_modifiers,
      .repeat_info = handle_wl_keyboard_repeat_info,
    };
    wl_data_device_listener dataDevice = {
      .data_offer = handle_wl_data_device_data_offer,
      .enter = handle_wl_data_device_enter,
      .leave = handle_wl_data_device_leave,
      .motion = handle_wl_data_device_motion,
      .drop = handle_wl_data_device_drop,
      .selection = handle_wl_data_device_selection,
    };
    wl_data_source_listener dataSource = {
      .target = handle_wl_data_source_target,
      .send = handle_wl_data_source_send,
      .cancelled = handle_wl_data_source_cancelled,
      .dnd_drop_performed = handle_wl_data_source_dnd_drop_performed,
      .dnd_finished = handle_wl_data_source_dnd_finished,
      .action = handle_wl_data_source_action,
    };
    wl_data_offer_listener dataOffer = {
      .offer = handle_wl_data_offer_offer,
      .source_actions = handle_wl_data_offer_source_actions,
      .action = handle_wl_data_offer_action,
    };
  } wl;

  struct
  {
    xdg_surface_listener surface = {
      .configure = handle_xdg_surface_configure,
    };
    xdg_wm_base_listener wmBase = {
      .ping = handle_xdg_wm_base_ping,
    };
    xdg_toplevel_listener toplevel = {
      .configure = handle_xdg_toplevel_configure,
      .close = handle_xdg_toplevel_close,
      .configure_bounds = handle_xdg_toplevel_configure_bounds,
      .wm_capabilities = handle_xdg_toplevel_wm_capabilities,
    };
  } xdg;

  struct
  {
    zwp_relative_pointer_v1_listener relativePointerV1 = {
      .relative_motion = handle_zwp_relative_pointer_v1_relative_motion,
    };
  } zwp;
};

static const Listeners listeners{};

} // namespace

void Wayland::Output::dumpToLog()
{
  debug("wayland: output %u:\n%s[%s] / %s[%s]\n%imm x %imm, pos [%i:%i], scale %u, subpixel %u, transform %u\ncurrent: %ux%u@%0.2f "
        "preferred: %ux%u@%0.2f",
    idx, name, desc, manufacturer, model, physical_width, physical_height, x, y, scaleFactor, subpixel, transform, mode.current.width,
    mode.current.height, mode.current.refresh / 1000.0f, mode.preferred.width, mode.preferred.height,
    mode.preferred.refresh / 1000.0f);
}

void Wayland::Output::destroy()
{
  uint32_t oidx = parent->outputs.toIndex(this);
  for (uint32_t i : LsbVisitor{parent->windows.validMask()})
  {
    Window &wnd = parent->windows.arr[i];
    if ((1 << oidx) & wnd.onOutputs)
      wnd.cbLeaveOutput(this);
  }
  parent->outputs.remove(this);
}

void Wayland::Window::destroy()
{
  if (activePointer)
    activePointer->onWindow = nullptr;
  if (activeKeyboard)
    activeKeyboard->focusedOn = nullptr;

  if (wp.viewport)
    wp_viewport_destroy(wp.viewport);
  if (xdg.toplevel)
    xdg_toplevel_destroy(xdg.toplevel);
  if (xdg.surface)
    xdg_surface_destroy(xdg.surface);
  if (obj)
    wl_surface_destroy(obj);
  parent->windows.remove(this);
}

void Wayland::Window::cbClose()
{
  if (this == parent->mainWindow)
  {
    debug("main window close signal received!");
    d3d::window_destroyed(this);
    win32_set_main_wnd(NULL);
    quit_game(0);
  }
  destroy();
}

void Wayland::Window::processChangedOutputs()
{
  if (!outputsChanged)
    return;

  resetupFullscreen();
  outputsChanged = false;
}

void Wayland::Window::resetupFullscreen()
{
  // window is on multiple outputs, leave it windowed till it settled down to single output
  if (onOutputs && __popcount(onOutputs) > 1)
  {
    if (!fullscreenOnOutput)
      return;
    xdg_toplevel_unset_fullscreen(xdg.toplevel);
    wp_viewport_set_destination(wp.viewport, -1, -1);
    fullscreenOnOutput = nullptr;
    wl_surface_commit(obj);
    return;
  }

  if (!fullscreen)
  {
    if (!fullscreenOnOutput)
      return;
    xdg_toplevel_unset_fullscreen(xdg.toplevel);
    wp_viewport_set_destination(wp.viewport, -1, -1);
    fullscreenOnOutput = nullptr;
  }
  else
  {
    Wayland::Output *targetOutput = getFirstOutput();
    if (fullscreenOnOutput == targetOutput)
      return;

    if (fullscreenOnOutput)
    {
      xdg_toplevel_unset_fullscreen(xdg.toplevel);
      wp_viewport_set_destination(wp.viewport, -1, -1);
    }
    fullscreenOnOutput = targetOutput;
    if (!targetOutput)
      return;

    xdg_toplevel_set_fullscreen(xdg.toplevel, targetOutput->obj);
    wp_viewport_set_destination(wp.viewport, targetOutput->mode.current.width, targetOutput->mode.current.height);
  }
  wl_surface_commit(obj);
}

void Wayland::Window::changeFullscreen(bool enable)
{
  if (enable == fullscreen)
    return;
  fullscreen = enable;
  resetupFullscreen();
}

Wayland::Output *Wayland::Window::getFirstOutput()
{
  for (uint32_t i : LsbVisitor{onOutputs})
    if (parent->outputs.isValid(i))
      return &parent->outputs.arr[i];
  return nullptr;
}

void Wayland::Window::cbEnterOutput(Wayland::Output *output)
{
  if (!output)
    return;

  onOutputs |= 1 << parent->outputs.toIndex(output);
  outputsChanged |= true;
}

void Wayland::Window::cbLeaveOutput(Wayland::Output *output)
{
  if (!output)
    return;

  onOutputs &= ~(1 << parent->outputs.toIndex(output));
  outputsChanged |= true;
}

void Wayland::Window::cbConfigure()
{
  if (activePointer)
    activePointer->clipChanged |= true;
  notify_window_resized(reportedWidth, reportedHeight);
}

void Wayland::Pointer::resetupHideState()
{
  if (hidden)
    wl_pointer_set_cursor(obj, enterSerial, nullptr, 0, 0);
  else
    ; // TODO: restore system pointer somehow
}

void Wayland::Pointer::resetupClipState()
{
  if (!clipChanged)
    return;

  if (zwp.confinedPointerV1)
  {
    zwp_confined_pointer_v1_destroy(zwp.confinedPointerV1);
    zwp.confinedPointerV1 = nullptr;
  }

  if (clip && onWindow)
  {
    G_ASSERT(parent->parent->zwp.pointerConstraintsV1);
    G_ASSERT(!zwp.confinedPointerV1);

    // constrain is a hint, as well as lock is a hint with ability to pass hint...
    // "hope it works"
    zwp.confinedPointerV1 = zwp_pointer_constraints_v1_confine_pointer(parent->parent->zwp.pointerConstraintsV1, onWindow->obj, obj,
      nullptr, zwp_pointer_constraints_v1_lifetime::ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
  }
  if (onWindow && clipChanged)
    wl_surface_commit(onWindow->obj);
  clipChanged = false;
}

void Wayland::Pointer::cbSetPointerPos(wl_fixed_t_proxy _x, wl_fixed_t_proxy _y)
{
  posChanged = (x != _x) || (y != _y);
  x = _x;
  y = _y;
}

void Wayland::Pointer::cbSetPointerButton(uint32_t button, uint32_t state)
{
  static const int LBUTTON_BIT = 0;
  static const int RBUTTON_BIT = 1;
  static const int MBUTTON_BIT = 2;
  static_assert(BTN_LEFT - BTN_MOUSE == LBUTTON_BIT);
  static_assert(BTN_RIGHT - BTN_MOUSE == RBUTTON_BIT);
  static_assert(BTN_MIDDLE - BTN_MOUSE == MBUTTON_BIT);
  if (button >= BTN_MOUSE)
    button -= BTN_MOUSE;
  else
    return;
  if (state == wl_pointer_button_state::WL_POINTER_BUTTON_STATE_PRESSED)
  {
    workcycle_internal::main_window_proc(onWindow, GPCM_MouseBtnPress, button, 0);
    buttonsPressBits |= 1 << button;
  }
  else if (state == wl_pointer_button_state::WL_POINTER_BUTTON_STATE_RELEASED)
  {
    workcycle_internal::main_window_proc(onWindow, GPCM_MouseBtnRelease, button, 0);
    buttonsPressBits &= ~(1 << button);
  }
}

void Wayland::Pointer::cbFrame()
{
  if (posChanged)
  {
    workcycle_internal::main_window_proc(onWindow, GPCM_MouseMove, 0, 0);
    posChanged = false;
  }

  if (axis.dirty)
  {
    if (axis.axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
      workcycle_internal::main_window_proc(onWindow, GPCM_MouseWheel, -wl_fixed_to_double(axis.value) * 100, 0);
    axis.dirty = false;
  }
}

void Wayland::Pointer::cbPointerFocus(wl_surface *surf, wl_fixed_t_proxy _x, wl_fixed_t_proxy _y, bool enter, uint32_t serial)
{
  Wayland::Window *cbWnd = parent->parent->windows.find(surf);
  if (!cbWnd)
    return;
  if (enter)
  {
    onWindow = cbWnd;
    onWindow->activePointer = this;
    workcycle_internal::main_window_proc(onWindow, GPCM_Activate, GPCMP1_Activate, 0);
    posChanged = (x != _x) || (y != _y);
    x = _x;
    y = _y;
    enterSerial = serial;
    validDeltas = false;
    resetupHideState();
  }
  else
  {
    if (cbWnd->activePointer == this)
      cbWnd->activePointer = nullptr;
    if (onWindow == cbWnd)
      onWindow = nullptr;
    workcycle_internal::main_window_proc(cbWnd, GPCM_Activate, GPCMP1_Inactivate, 0);
  }
  clipChanged = true;
}

void Wayland::Pointer::cbAxis(uint32_t _axis, wl_fixed_t_proxy value)
{
  axis.dirty = true;
  axis.axis = _axis; // sigh...
  axis.value = value;
}

void Wayland::Pointer::cbAxisSrc(uint32_t axis_source)
{
  axis.dirty = true;
  axis.source = axis_source;
}

void Wayland::Pointer::cbRelativeMotion(wl_fixed_t_proxy x, wl_fixed_t_proxy y)
{
  dltX += x;
  dltY += y;
}

void Wayland::Keyboard::cbKeymap(uint32_t format, int32_t fd, uint32_t size)
{
  struct FdCleanup
  {
    int32_t fd;
    ~FdCleanup()
    {
      if (fd)
        ::close(fd);
    }
  };
  FdCleanup fdc{fd};

  if (xkbState)
  {
    xkb_state_unref(xkbState);
    xkbState = nullptr;
  }
  if (keymap)
  {
    xkb_keymap_unref(keymap);
    keymap = nullptr;
  }

  if (format == wl_keyboard_keymap_format::WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
  {
    // Read keymap data from file descriptor
    void *mappedStr = ::mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mappedStr == MAP_FAILED)
    {
      logerr("wayland: failed to map keymap err %u", errno);
      return;
    }
    // Create XKB keymap and state
    keymap = xkb_keymap_new_from_string(parent->parent->xkbCtx, (const char *)mappedStr, XKB_KEYMAP_FORMAT_TEXT_V1,
      XKB_KEYMAP_COMPILE_NO_FLAGS);
    ::munmap(mappedStr, size);
    if (!keymap)
    {
      logerr("wayland: failed to create keymap");
      return;
    }

    xkbState = xkb_state_new(keymap);
    if (!xkbState)
    {
      logerr("wayland: failed to create state");
      return;
    }
  }
}

void Wayland::Keyboard::sendKeyMsg(bool pressed, uint32_t keycode)
{
  uint32_t sym = keycode;
  if (keymap && xkbState)
  {
    keycode = keycode + 8; // magic xkb offset
    sym = utf32symb_to_utf16symb(xkb_state_key_get_utf32(xkbState, keycode));
  }
  workcycle_internal::main_window_proc(focusedOn, pressed ? GPCM_KeyPress : GPCM_KeyRelease, keycode, sym);
}

void Wayland::Keyboard::trackRepeat(bool pressed, uint32_t key)
{
  KeyRepeatTrigger *trig = nullptr;
  for (KeyRepeatTrigger &i : repeatStack)
    if (i.key == key)
    {
      trig = &i;
      break;
    }

  if (!trig)
  {
    if (pressed)
      repeatStack.push_back({key, ref_time_ticks() + repeatDelayTicks});
  }
  else if (!pressed)
  {
    if (trig != &repeatStack.back())
      *trig = repeatStack.back();
    repeatStack.pop_back();
  }
  else
    trig->repeatAfterTicks = ref_time_ticks() + repeatDelayTicks;
}

void Wayland::Keyboard::processRepeats()
{
  if (repeatAfterTicks == 0)
    return;

  int64_t ref = ref_time_ticks();
  for (KeyRepeatTrigger &i : repeatStack)
  {
    int64_t dlt = ref - i.repeatAfterTicks;
    if (dlt < 0)
      continue;

    uint32_t repeats = 1 + dlt / repeatAfterTicks;
    i.repeatAfterTicks += repeats * repeatAfterTicks;
    while (repeats)
    {
      sendKeyMsg(true, i.key);
      --repeats;
    }
  }
}

// time_ms has unknown base by spec and there is no reference value to compare on repeats
// so don't use it right now and just use "immediate" timestamps
void Wayland::Keyboard::cbKey(uint32_t key, uint32_t state, uint32_t /*time_ms*/, uint32_t serial)
{
  parent->lastKbSerial = serial;
  if (state != wl_keyboard_key_state::WL_KEYBOARD_KEY_STATE_PRESSED && state != wl_keyboard_key_state::WL_KEYBOARD_KEY_STATE_RELEASED)
  {
    // WL_KEYBOARD_KEY_STATE_REPEATED since 10, sets repeat rate to 0
    if (repeatAfterTicks == 0)
      sendKeyMsg(true, key);
    return;
  }

  bool pressed = state == wl_keyboard_key_state::WL_KEYBOARD_KEY_STATE_PRESSED;

  trackRepeat(pressed, key);
  sendKeyMsg(pressed, key);
}

#define wl_array_for_each_fixed(pos, array) for (; (const char *)pos < ((const char *)(array)->data + (array)->size); (pos)++)

void Wayland::Keyboard::cbEnter(wl_surface *surface, wl_array *keys, uint32_t serial)
{
  Wayland::Window *cbWnd = parent->parent->windows.find(surface);
  if (!cbWnd)
    return;
  focusedOn = cbWnd;
  focusedOn->activeKeyboard = this;
  workcycle_internal::main_window_proc(focusedOn, GPCM_Activate, GPCMP1_Activate, 0);


  uint32_t *key = (uint32_t *)keys->data;
  wl_array_for_each_fixed(key, keys) { cbKey(*key, wl_keyboard_key_state::WL_KEYBOARD_KEY_STATE_PRESSED, 0, serial); }
}

void Wayland::Keyboard::cbLeave(wl_surface *surface)
{
  Wayland::Window *cbWnd = parent->parent->windows.find(surface);
  if (cbWnd->activeKeyboard == this)
    cbWnd->activeKeyboard = nullptr;
  if (focusedOn == cbWnd)
  {
    focusedOn = nullptr;
    workcycle_internal::main_window_proc(cbWnd, GPCM_Activate, GPCMP1_Inactivate, 0);
  }
}

void Wayland::Keyboard::cbModifier(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
{
  if (xkbState)
    xkb_state_update_mask(xkbState, depressed, latched, locked, 0, 0, group);
}

void Wayland::Keyboard::cbRepeatInfo(uint32_t delay_ms, uint32_t rate_per_second)
{
  repeatDelayTicks = rel_ref_time_ticks(0, delay_ms * 1000);
  repeatAfterTicks = rate_per_second ? rel_ref_time_ticks(0, 1000 * 1000 / rate_per_second) : 0;
}

bool Wayland::bindGlobal(const Wayland::InputGlobal &in, Wayland::GlobalRequest rq)
{
  if (strcmp(in.interface, rq.interface.name) != 0)
    return false;

  if (in.version < rq.version)
    return false;

  *rq.dst = wl_registry_bind(wl.registry, in.name, &rq.interface, rq.version);

  if (globalsCache.size() <= in.name)
    globalsCache.resize(in.name + 1);
  globalsCache[in.name] = *rq.dst;

  return true;
}

void Wayland::cbRegistryGlobal(const Wayland::InputGlobal &in)
{
  bool cbRegistered = true;
  void *gptr = nullptr;

  bindGlobal(in, {wl_compositor_interface, RegistryMinVersions::COMPOSITOR, (void **)&wl.compositor});
  bindGlobal(in, {xdg_wm_base_interface, RegistryMinVersions::XDG_WM_BASE, (void **)&xdg.wmBase});

  if (bindGlobal(in, {wl_output_interface, RegistryMinVersions::OUTPUT, (void **)&gptr}))
  {
    wl_output *output = (wl_output *)gptr;
    Output *info = outputs.add(output, this);
    info->idx = outputs.toIndex(info);
    cbRegistered = wl_output_add_listener(output, &listeners.wl.output, info) == 0;
  }

  if (bindGlobal(in, {wl_seat_interface, RegistryMinVersions::SEAT, (void **)&gptr}))
  {
    wl_seat *seat = (wl_seat *)gptr;
    Seat *info = seats.add(seat, this);
    if (wl.dataDeviceManager)
    {
      info->dataDevice = wl_data_device_manager_get_data_device(wl.dataDeviceManager, seat);
      if (info->dataDevice)
      {
        if (wl_data_device_add_listener(info->dataDevice, &listeners.wl.dataDevice, info) != 0)
        {
          wl_data_device_destroy(info->dataDevice);
          info->dataDevice = nullptr;
        }
      }
      info->outboundClipboard = wl_data_device_manager_create_data_source(wl.dataDeviceManager);
      if (info->outboundClipboard)
      {
        if (wl_data_source_add_listener(info->outboundClipboard, &listeners.wl.dataSource, info) != 0)
        {
          wl_data_source_destroy(info->outboundClipboard);
          info->outboundClipboard = nullptr;
        }
        wl_data_source_offer(info->outboundClipboard, mime_text_utf8);
      }
    }
    cbRegistered = wl_seat_add_listener(seat, &listeners.wl.seat, info) == 0;
  }

  if (bindGlobal(in, {wp_viewporter_interface, RegistryMinVersions::VIEWPORTER, (void **)&gptr}))
    wp.viewporter = (wp_viewporter *)gptr;

  if (bindGlobal(in, {zwp_relative_pointer_manager_v1_interface, RegistryMinVersions::RELATIVE_POINTER, (void **)&gptr}))
    zwp.relativePointerManagerV1 = (zwp_relative_pointer_manager_v1 *)gptr;

  if (bindGlobal(in, {zwp_pointer_constraints_v1_interface, RegistryMinVersions::POINTER_CONSTRAINTS, (void **)&gptr}))
    zwp.pointerConstraintsV1 = (zwp_pointer_constraints_v1 *)gptr;

  if (bindGlobal(in, {wl_data_device_manager_interface, RegistryMinVersions::DATA_DEVICE_MANAGER, (void **)&gptr}))
    wl.dataDeviceManager = (wl_data_device_manager *)gptr;

  if (!cbRegistered)
    logerr("wayland: can't listen to %s %p:%u", in.interface, gptr, in.name);
}

void Wayland::cbRegistryGlobalRemove(uint32_t name)
{
  if (!globalsCache[name])
    return;

  if (Output *oi = outputs.find((wl_output *)globalsCache[name]))
    oi->destroy();
  globalsCache[name] = nullptr;
}

bool Wayland::Seat::setClipboardUTF8Text(const char *text)
{
  if (!outboundClipboard || !dataDevice)
    return false;

  wl_data_device_set_selection(dataDevice, outboundClipboard, lastKbSerial);
  outboundClipboardText.setStr(text);
  return true;
}

bool Wayland::Seat::getClipboardUTF8Text(char *dest, int buf_size)
{
  inbound.process();
  if (inbound.text.empty() || !inbound.completed)
    return false;
  memcpy(dest, inbound.text.data(), min(inbound.text.length(), buf_size));
  return true;
}

void Wayland::Seat::cbSeatCaps(uint32_t capabilities)
{
  if (capabilities & wl_seat_capability::WL_SEAT_CAPABILITY_POINTER)
  {
    Pointer *pi = parent->pointers.add(wl_seat_get_pointer(obj), this);
    if (parent->zwp.relativePointerManagerV1)
    {
      pi->zwp.relativePointerV1 = zwp_relative_pointer_manager_v1_get_relative_pointer(parent->zwp.relativePointerManagerV1, pi->obj);
      if (zwp_relative_pointer_v1_add_listener(pi->zwp.relativePointerV1, &listeners.zwp.relativePointerV1, pi) != 0)
      {
        zwp_relative_pointer_v1_destroy(pi->zwp.relativePointerV1);
        pi->zwp.relativePointerV1 = nullptr;
      }
      else
        debug("wayland: seat %p pointer %p using relative pointer motion", obj, pi->obj);
    }
    wl_pointer_add_listener(pi->obj, &listeners.wl.pointer, pi);
  }
  if (capabilities & wl_seat_capability::WL_SEAT_CAPABILITY_KEYBOARD)
  {
    Keyboard *ki = parent->keyboards.add(wl_seat_get_keyboard(obj), this);
    wl_keyboard_add_listener(ki->obj, &listeners.wl.keyboard, ki);
  }
}

void Wayland::Seat::cbSendData(const char *mime_type, int32_t fd)
{
  if (strcmp(mime_type, mime_text_utf8) == 0)
  {
    ssize_t r = ::write(fd, outboundClipboardText.data(), outboundClipboardText.length());
    G_UNUSED(r);
  }
  ::close(fd);
}

void Wayland::Seat::cbSelection(wl_data_offer *offer)
{
  if (!offer)
    return;
  if (lastOffer == offer)
  {
    if (!strstr(lastOfferMimeTypes.data(), mime_text_utf8))
    {
      wl_data_offer_destroy(offer);
      return;
    }
    lastOffer = nullptr;
  }
  else
  {
    // pretend offer can handle utf8 by default
  }
  inbound.recive(offer);
}

void Wayland::Seat::cbOfferType(wl_data_offer *offer, const char *mime)
{
  if (offer != lastOffer)
  {
    if (lastOffer)
    {
      wl_data_offer_destroy(lastOffer);
      lastOffer = nullptr;
    }
    lastOfferMimeTypes.clear();
    lastOffer = offer;
  }
  lastOfferMimeTypes.append(mime);
  lastOfferMimeTypes += " ";
}

void Wayland::Seat::cbNewOffer(wl_data_offer *offer) { wl_data_offer_add_listener(offer, &listeners.wl.dataOffer, this); }

void Wayland::Seat::InboundTransfer::recive(wl_data_offer *from)
{
  cleanup();

  offer = from;
  completed = false;
  text.clear();

  int rc = pipe2(pipe, O_NONBLOCK);
  if (rc < 0)
  {
    wl_data_offer_destroy(offer);
    return;
  }
  wl_data_offer_receive(offer, mime_text_utf8, pipe[1]);
  // if we don't close it, reading will always expect more data
  // and it looks off that we close it right away, but it works fine
  ::close(pipe[1]);
  pipe[1] = -1;
}

void Wayland::Seat::InboundTransfer::process()
{
  if (completed || !offer)
    return;

  const size_t BUF_SZ = 8192;
  char buf[BUF_SZ];
  ssize_t nread;
  while ((nread = ::read(pipe[0], buf, BUF_SZ)) > 0)
    text.append(buf, nread);

  if (nread < 0 && (errno == EAGAIN || errno == EINTR))
    return;

  completed = true;
  cleanup();
}

void Wayland::Seat::InboundTransfer::cleanup()
{
  if (pipe[0] >= 0)
    ::close(pipe[0]);
  if (pipe[1] >= 0)
    ::close(pipe[1]);
  if (offer)
    wl_data_offer_destroy(offer);

  pipe[0] = -1;
  pipe[1] = -1;
  offer = nullptr;
}

void Wayland::processMessages()
{
  if (!wl.display)
    return;

  {
    TIME_PROFILE(wayland_display_roundtrip);
    if (wl_display_roundtrip(wl.display) < 0)
    {
      debug("wayland: display disconnected");
      quit_game(0);
    }
  }

  for (uint32_t i : LsbVisitor{seats.validMask()})
    seats.arr[i].inbound.process();

  for (uint32_t i : LsbVisitor{keyboards.validMask()})
    keyboards.arr[i].processRepeats();

  for (uint32_t i : LsbVisitor{windows.validMask()})
    windows.arr[i].processChangedOutputs();

  for (uint32_t i : LsbVisitor{pointers.validMask()})
    pointers.arr[i].resetupClipState();
}

#define WL_CHECK(x)                  \
  if (!(x))                          \
  {                                  \
    debug("wayland: " #x " failed"); \
    return false;                    \
  }

#define WL_CHECK_A(x, y) \
  {                      \
    x = y;               \
    WL_CHECK(x);         \
  }


bool Wayland::init()
{
  if (!loadLib())
    return false;

  bool listner = false;

  WL_CHECK_A(xkbCtx, xkb_context_new(XKB_CONTEXT_NO_FLAGS));

  WL_CHECK_A(wl.display, wl_display_connect(nullptr));
  WL_CHECK_A(wl.registry, wl_display_get_registry(wl.display));
  WL_CHECK_A(listner, wl_registry_add_listener(wl.registry, &listeners.wl.registry, this) == 0);

  wl_display_roundtrip(wl.display);

  WL_CHECK(wl.compositor);
  WL_CHECK(xdg.wmBase);
  WL_CHECK(outputs.isValid(0));
  WL_CHECK(zwp.relativePointerManagerV1);
  WL_CHECK(zwp.pointerConstraintsV1);

  WL_CHECK_A(listner, xdg_wm_base_add_listener(xdg.wmBase, &listeners.xdg.wmBase, this) == 0);

  WL_CHECK_A(mainWindow, setupWindow());

  wl_surface_commit(mainWindow->obj);
  wl_display_roundtrip(wl.display);

  return true;
}

void Wayland::shutdown()
{
  windows.reset();
  outputs.reset();
  pointers.reset();
  keyboards.reset();
  seats.reset();

  wl_display_roundtrip(wl.display);
  wl_display_disconnect(wl.display);
  xdg = {};
  wl = {};

  if (xkbCtx)
  {
    xkb_context_unref(xkbCtx);
    xkbCtx = nullptr;
  }

  unloadLib();
}

bool Wayland::changeGamma(float /*value*/) { return false; }

Wayland::Output *Wayland::getDefaultOutput()
{
  Output *oi = nullptr;
  if (mainWindow)
    oi = mainWindow->getFirstOutput();

  if (!oi)
  {
    if (!outputs.isEmpty())
    {
      uint32_t idx = 0;
      while (!outputs.isValid(idx))
        ++idx;
      oi = &outputs.arr[idx];
    }
  }
  return oi;
}

void Wayland::getDisplaySize(int &width, int &height, bool)
{
  Output *oi = getDefaultOutput();
  if (!oi)
  {
    width = 0;
    height = 0;
  }
  else
  {
    width = oi->mode.current.width;
    height = oi->mode.current.height;
  }

  if (mainWindow && (mainWindow->reportedWidth > width || mainWindow->reportedHeight > height))
  {
    width = mainWindow->reportedWidth;
    height = mainWindow->reportedHeight;
  }
}

void Wayland::getVideoModeList(Tab<String> &list)
{
  Output *oi = getDefaultOutput();
  list.clear();
  if (!oi)
    return;

  auto &preferred = oi->mode.preferred;
  auto &current = oi->mode.current;
  list.push_back(String(64, "%d x %d", current.width, current.height));
  if (preferred.width != current.width || preferred.height != current.height)
    list.push_back(String(64, "%d x %d", preferred.width, preferred.height));
}

void *Wayland::getMainWindowPtrHandle() const { return mainWindow; }

bool Wayland::isMainWindow(void *wnd) const { return wnd == (void *)mainWindow; }

void Wayland::destroyMainWindow()
{
  mainWindow->destroy();
  win32_set_main_wnd(NULL);
  mainWindow = nullptr;
}

bool Wayland::initWindow(const char *title, int winWidth, int winHeight)
{
  if (!mainWindow)
  {
    mainWindow = setupWindow();
    if (!mainWindow)
      return false;
    wl_surface_commit(mainWindow->obj);
    wl_display_roundtrip(wl.display);
  }
  setTitle(title, nullptr);
  debug("Wayland::initWindow %s %u %u", title, winWidth, winHeight);
  return true;
}

void Wayland::getWindowPosition(void *, int &cx, int &cy)
{
  // we don't know window position by design of wayland
  // assume everything has local window coordinates and position is always zero
  cx = 0;
  cy = 0;
}

void Wayland::setTitle(const char *title, const char *)
{
  xdg_toplevel_set_title(mainWindow->xdg.toplevel, title);
  xdg_toplevel_set_app_id(mainWindow->xdg.toplevel, title);
}

void Wayland::setTitleUTF8(const char *title, const char *)
{
  xdg_toplevel_set_title(mainWindow->xdg.toplevel, title);
  xdg_toplevel_set_app_id(mainWindow->xdg.toplevel, title);
}

int Wayland::getScreenRefreshRate()
{
  Output *oi = getDefaultOutput();
  if (oi)
    return oi->mode.current.refresh / 1000;
  else
    return 0;
}

bool Wayland::getLastCursorPos(int *cx, int *cy, void *w)
{
  Window *wi = (Window *)w;
  if (!wi)
    wi = mainWindow;
  if (wi && wi->activePointer)
  {
    *cx = wl_fixed_to_int(wi->activePointer->x);
    *cy = wl_fixed_to_int(wi->activePointer->y);
    return true;
  }
  return false;
}

void Wayland::setCursor(void *, const char *) { G_ASSERTF(0, "Wayland::setCursor not implemented"); }

void Wayland::setCursorPosition(int cx, int cy, void *w)
{
  Window *wi = (Window *)w;
  if (!wi)
    wi = mainWindow;
  if (!zwp.pointerConstraintsV1 || !wi || !wi->activePointer)
    return;

  Pointer &pi = *wi->activePointer;
  if (pi.zwp.confinedPointerV1)
  {
    zwp_confined_pointer_v1_destroy(pi.zwp.confinedPointerV1);
    pi.zwp.confinedPointerV1 = nullptr;
  }

  zwp_locked_pointer_v1 *pointerLock = zwp_pointer_constraints_v1_lock_pointer(zwp.pointerConstraintsV1, wi->obj, pi.obj, NULL,
    ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
  zwp_locked_pointer_v1_set_cursor_position_hint(pointerLock, wl_fixed_from_int(cx), wl_fixed_from_int(cy));
  wl_surface_commit(wi->obj);
  zwp_locked_pointer_v1_destroy(pointerLock);

  pi.clipChanged = true;
  pi.resetupClipState();
}

void Wayland::getAbsoluteCursorPosition(int &cx, int &cy)
{
  if (mainWindow && mainWindow->activePointer)
  {
    cx = wl_fixed_to_int(mainWindow->activePointer->x);
    cy = wl_fixed_to_int(mainWindow->activePointer->y);
  }
  else
  {
    cx = 0;
    cy = 0;
  }
}

bool Wayland::getCursorDelta(int &cx, int &cy, void *w)
{
  Window *wi = (Window *)w;
  if (!wi)
    wi = mainWindow;

  if (!wi || !wi->activePointer || !wi->activePointer->zwp.relativePointerV1)
    return false;

  Pointer &pointer = *wi->activePointer;
  if (!pointer.validDeltas)
  {
    pointer.validDeltas = true;
    pointer.dltX = 0;
    pointer.dltY = 0;
  }
  else
  {
    cx = wl_fixed_to_int(pointer.dltX);
    cy = wl_fixed_to_int(pointer.dltY);
    pointer.dltX -= wl_fixed_from_int(cx);
    pointer.dltY -= wl_fixed_from_int(cy);
  }
  return true;
}

void Wayland::clipCursor()
{
  if (!zwp.pointerConstraintsV1 || !zwp.relativePointerManagerV1 || !mainWindow || !mainWindow->activePointer)
    return;

  Pointer &pointer = *mainWindow->activePointer;
  pointer.clipChanged = !pointer.clip;
  pointer.clip = true;
}

void Wayland::unclipCursor()
{
  if (!zwp.pointerConstraintsV1 || !zwp.relativePointerManagerV1 || !mainWindow || !mainWindow->activePointer)
    return;

  Pointer &pointer = *mainWindow->activePointer;
  pointer.clipChanged = pointer.clip;
  pointer.clip = false;
}

void *Wayland::getNativeDisplay() { return (void *)wl.display; }
void *Wayland::getNativeWindow(void *w) { return w ? ((Window *)w)->obj : nullptr; }

void Wayland::setFullscreenMode(bool enable)
{
  if (mainWindow)
    mainWindow->changeFullscreen(enable);
}

bool Wayland::getWindowScreenRect(void *w, linux_GUI::RECT *rect, linux_GUI::RECT *rect_unclipped)
{
  if (!w || !rect)
    return false;
  Window &wi = *(Window *)w;
  if (wi.fullscreen && wi.fullscreenOnOutput)
  {
    rect->left = 0;
    rect->top = 0;
    rect->right = wi.fullscreenOnOutput->mode.current.width;
    rect->bottom = wi.fullscreenOnOutput->mode.current.height;
  }
  else
  {
    rect->left = 0;
    rect->top = 0;
    rect->right = wi.reportedWidth ? wi.reportedWidth : wi.boundsWidth;
    rect->bottom = wi.reportedHeight ? wi.reportedHeight : wi.boundsHeight;
  }

  if (rect_unclipped)
    *rect_unclipped = *rect;

  return true;
}

void Wayland::hideCursor(bool hide)
{
  for (uint32_t i : LsbVisitor{pointers.validMask()})
  {
    pointers.arr[i].hidden = hide;
    if (pointers.arr[i].onWindow)
      pointers.arr[i].resetupHideState();
  }
}

bool Wayland::getClipboardUTF8Text(char *dest, int buf_size)
{
  for (uint32_t i : LsbVisitor{seats.validMask()})
  {
    if (seats.arr[i].getClipboardUTF8Text(dest, buf_size))
      return true;
  }
  return false;
}

bool Wayland::setClipboardUTF8Text(const char *text)
{
  bool ret = false;
  for (uint32_t i : LsbVisitor{seats.validMask()})
    ret |= seats.arr[i].setClipboardUTF8Text(text);
  return ret;
}

#define WL_CHECKP(x)                 \
  if (!(x))                          \
  {                                  \
    debug("wayland: " #x " failed"); \
    return nullptr;                  \
  }

#define WL_CHECKP_A(x, y) \
  {                       \
    x = y;                \
    WL_CHECKP(x);         \
  }

Wayland::Window *Wayland::setupWindow()
{
  bool listner;
  wl_surface *surf;
  WL_CHECKP_A(surf, wl_compositor_create_surface(wl.compositor));

  Window &out = *windows.add(surf, this);
  struct CleanupOnFailure
  {
    Window *ptr;
    ~CleanupOnFailure()
    {
      if (ptr)
        ptr->destroy();
    }
  };
  CleanupOnFailure cleanup{&out};

  WL_CHECKP_A(listner, wl_surface_add_listener(out.obj, &listeners.wl.surface, &out) == 0);
  WL_CHECKP_A(out.xdg.surface, xdg_wm_base_get_xdg_surface(xdg.wmBase, out.obj));
  WL_CHECKP_A(listner, xdg_surface_add_listener(out.xdg.surface, &listeners.xdg.surface, &out) == 0);
  WL_CHECKP_A(out.xdg.toplevel, xdg_surface_get_toplevel(out.xdg.surface));
  WL_CHECKP_A(listner, xdg_toplevel_add_listener(out.xdg.toplevel, &listeners.xdg.toplevel, &out) == 0);
  if (wp.viewporter)
    out.wp.viewport = wp_viewporter_get_viewport(wp.viewporter, out.obj);
  cleanup.ptr = nullptr;
  return &out;
}

#undef WL_CHECK_A
#undef WL_CHECK
#undef WL_CHECKP_A
#undef WL_CHECKP
