// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "actionData.h"
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiVrInput.h>
#include <generic/dag_carray.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_adjpow2.h>
#include <util/dag_delayedAction.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_msgIo.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <atomic>

enum
{
  BS_OFS_kbd = 0,
  BS_OFS_pointing = 256,
  BS_OFS_gamepad = 288,
  BS_OFS_joy = 384,
  BS_OFS_end = 640
};
enum
{
  AS_OFS_kbd = 0,
  AS_OFS_pointing = 0,
  AS_OFS_gamepad = 4,
  AS_OFS_joy = 28,
  AS_OFS_end = 92
};

namespace
{
struct PressTimeAndClickCount
{
  uint32_t t0 : 30, clickCnt : 2;
};
struct UsedControlsMask
{
  ConstSizeBitArrayBase<BS_OFS_end> bt;
  ConstSizeBitArrayBase<AS_OFS_end> ax;
  UsedControlsMask() { reset(); }
  void reset()
  {
    bt.ctor();
    ax.ctor();
  }
};
struct BindingsRecorder
{
  dainput::action_handle_t a = dainput::BAD_ACTION_HANDLE;
  dainput::DigitalActionBinding db;
  dainput::AnalogAxisActionBinding ab;
  dainput::AnalogStickActionBinding sb;
  dainput::SingleButtonId *mod = nullptr;
  ConstSizeBitArrayBase<BS_OFS_end> btStateInitial;

  BindingsRecorder() { btStateInitial.ctor(); }
  void resetBindings(dainput::action_handle_t _a);
  void process();
};
} // namespace

enum
{
  INPMSG_btn = 1,
  INPMSG_mmove = 2,
  INPMSG_ax_btn = 3,
  INPMSG_tick = 4,

  INPMSG_time_shift = 3,
  INPMSG_type_mask = (1 << INPMSG_time_shift) - 1,
};
static ThreadSafeMsgIoEx input_queue(64 << 10);
static ConstSizeBitArrayBase<BS_OFS_end> input_queue_bt_state;
static carray<short, AS_OFS_end> input_queue_ax_state;
static int64_t last_processed_t_us = 0;
static std::atomic<int64_t> last_queued_t_us{0};
static std::atomic<int64_t> time0_us{0};

static inline unsigned get_rel_time_msec(int64_t t)
{
  int64_t time0US = time0_us.load();
  return t > time0US ? unsigned((t - time0US) / 1000) : 0;
}

static ConstSizeBitArrayBase<BS_OFS_end> btState, btStatePrev;
static carray<PressTimeAndClickCount, BS_OFS_end> clState;
static carray<float, AS_OFS_end> axState, axStatePrev;
static UsedControlsMask start_umask;
static constexpr int START_UM_LAYERS = 8;
static Tab<dainput::SingleButtonId> start_ubtn_layered[START_UM_LAYERS];
static int pending_cnt_start_ubtn[START_UM_LAYERS] = {0};

static float sysMouseVx = 0, sysMouseVy = 0, sysMouseResDx = 0, sysMouseResDy = 0;
static unsigned dev_kbd_lastInputOnFrameNo = 0;
static unsigned dev_mouse_lastInputOnFrameNo = 0;
static unsigned dev_gamepad_lastInputOnFrameNo = 0;
static unsigned dev_joy_lastInputOnFrameNo = 0;
static unsigned dev_touch_lastInputOnFrameNo = 0;
static unsigned dev_clicks_count[get_const_log2(dainput::DEV_USED_gamepad) + 1] = {0};
static unsigned dev_clicks_count_last[get_const_log2(dainput::DEV_USED_gamepad) + 1] = {0};

static BindingsRecorder binding_rec;

static Tab<dainput::DigitalActionProgress> lastReportedDaProgress;
static void report_digital_action_progress(dag::Span<dainput::DigitalActionProgress> daProgress, dag::ConstSpan<uint32_t> curEvents);

#define DEV_BTN_TO_GLOB_BS_IDX(DEV, BTN, DEF_RET)              \
  switch (DEV)                                                 \
  {                                                            \
    case dainput::DEV_kbd: BTN += BS_OFS_kbd; break;           \
    case dainput::DEV_pointing: BTN += BS_OFS_pointing; break; \
    case dainput::DEV_gamepad: BTN += BS_OFS_gamepad; break;   \
    case dainput::DEV_joy: BTN += BS_OFS_joy; break;           \
    default: return DEF_RET;                                   \
  }

#define DEV_AX_TO_GLOB_AS_IDX(DEV, AX, DEF_RET)               \
  if (AX == dainput::AXIS_NULL_ID)                            \
    return DEF_RET;                                           \
  switch (DEV)                                                \
  {                                                           \
    case dainput::DEV_kbd: AX += AS_OFS_kbd; break;           \
    case dainput::DEV_pointing: AX += AS_OFS_pointing; break; \
    case dainput::DEV_gamepad: AX += AS_OFS_gamepad; break;   \
    case dainput::DEV_joy: AX += AS_OFS_joy; break;           \
    default: return DEF_RET;                                  \
  }

static inline int get_btn_bs_idx(int dev, int btn)
{
  DEV_BTN_TO_GLOB_BS_IDX(dev, btn, -1);
  return btn;
}
static inline dainput::SingleButtonId get_btn_from_bs_idx(uint16_t idx)
{
#define CHECK_IDX_RANGE(ST, END)                \
  if (idx >= BS_OFS_##ST && idx < BS_OFS_##END) \
    return dainput::SingleButtonId{dainput::DEV_##ST, uint16_t(idx - BS_OFS_##ST)};
  CHECK_IDX_RANGE(kbd, pointing);
  CHECK_IDX_RANGE(pointing, gamepad);
  CHECK_IDX_RANGE(gamepad, joy);
  CHECK_IDX_RANGE(joy, end);
#undef CHECK_IDX_RANGE
  return dainput::SingleButtonId{dainput::DEV_none, 0};
}
static inline bool get_btn_state(int dev, int btn)
{
  DEV_BTN_TO_GLOB_BS_IDX(dev, btn, false);
  return btState.get(btn) != 0;
}
static inline bool get_btn_long_press_state(int dev, int btn, uint32_t t)
{
  DEV_BTN_TO_GLOB_BS_IDX(dev, btn, false);
  return btState.get(btn) && t >= clState[btn].t0 + dainput::agData.longPressDur;
}
static inline bool get_btn_rep_click_state(int dev, int btn, int clk)
{
  DEV_BTN_TO_GLOB_BS_IDX(dev, btn, false);
  return btState.get(btn) && clState[btn].clickCnt == clk;
}
static inline int get_btn_press_dur(int dev, int btn, uint32_t t)
{
  DEV_BTN_TO_GLOB_BS_IDX(dev, btn, 0);
  return (t > clState[btn].t0) ? t - clState[btn].t0 : 0;
}

static inline int get_axis_as_idx(int dev, int ax)
{
  DEV_AX_TO_GLOB_AS_IDX(dev, ax, -1);
  return ax;
}
static inline dainput::SingleButtonId get_axis_from_as_idx(uint16_t idx)
{
#define CHECK_IDX_RANGE(ST, END)                \
  if (idx >= AS_OFS_##ST && idx < AS_OFS_##END) \
    return dainput::SingleButtonId{dainput::DEV_##ST, uint16_t(idx - AS_OFS_##ST)};
  G_STATIC_ASSERT(AS_OFS_kbd == AS_OFS_pointing); // CHECK_IDX_RANGE(kbd, pointing);
  CHECK_IDX_RANGE(pointing, gamepad);
  CHECK_IDX_RANGE(gamepad, joy);
  CHECK_IDX_RANGE(joy, end);
#undef CHECK_IDX_RANGE
  return dainput::SingleButtonId{dainput::DEV_none, 0};
}
static inline float get_axis_state(int dev, int ax)
{
  DEV_AX_TO_GLOB_AS_IDX(dev, ax, 0);
  return axState[ax];
}

static inline float calc_axis_non_lin(float val, float nonlin)
{
  return (nonlin && val) ? (val > 0 ? 1.0f : -1.0f) * powf(fabsf(val), 1.f + nonlin) : val;
}
static inline int calc_change(bool new_val, bool old_val) { return new_val == old_val ? 0 : (new_val ? 1 : -1); }

static inline void mask_with_single_button(UsedControlsMask &m, const dainput::SingleButtonId &sb)
{
  int idx = get_btn_bs_idx(sb.devId, sb.btnId);
  if (idx >= 0)
    m.bt.set(idx);

  if (sb.devId == dainput::DEV_gamepad && dainput::dev3_gpad)
  {
    idx = dainput::dev3_gpad->getPairedAxisForButton(sb.btnId);
    if (idx < 0)
      return;
    idx = get_axis_as_idx(sb.devId, idx);
    if (idx >= 0)
      m.ax.set(idx);
  }
}
static inline void mask_with_single_axis(UsedControlsMask &m, const dainput::SingleButtonId &sb)
{
  int idx = get_axis_as_idx(sb.devId, sb.btnId);
  if (idx >= 0)
    m.ax.set(idx);

  if (sb.devId == dainput::DEV_gamepad && dainput::dev3_gpad)
  {
    idx = dainput::dev3_gpad->getPairedButtonForAxis(sb.btnId);
    if (idx < 0)
      return;
    idx = get_btn_bs_idx(sb.devId, idx);
    if (idx >= 0)
      m.bt.set(idx);
  }
}
static bool is_single_button_masked(const UsedControlsMask &m, const dainput::SingleButtonId &sb)
{
  int idx = get_btn_bs_idx(sb.devId, sb.btnId);
  return (idx >= 0) ? m.bt.get(idx) : false;
}

static void mask_with_binding_controls(UsedControlsMask &m, const dainput::DigitalActionBinding &b)
{
  if (b.btnCtrl)
    mask_with_single_button(m, dainput::SingleButtonId{b.devId, b.ctrlId});
  else
    mask_with_single_axis(m, dainput::SingleButtonId{b.devId, b.ctrlId});

  for (int i = 0; i < b.modCnt; i++)
    mask_with_single_button(m, b.mod[i]);
}
static void mask_with_binding_controls(UsedControlsMask &m, const dainput::AnalogAxisActionBinding &b, bool min_b, bool max_b)
{
  mask_with_single_axis(m, dainput::SingleButtonId{b.devId, b.axisId});

  if (min_b)
    mask_with_single_button(m, b.minBtn);
  if (max_b)
    mask_with_single_button(m, b.maxBtn);
  for (int i = 0; i < b.modCnt; i++)
    mask_with_single_button(m, b.mod[i]);
}
static void mask_with_binding_controls(UsedControlsMask &m, const dainput::AnalogStickActionBinding &b, bool xmin_b, bool xmax_b,
  bool ymin_b, bool ymax_b)
{
  mask_with_single_axis(m, dainput::SingleButtonId{b.devId, b.axisXId});
  mask_with_single_axis(m, dainput::SingleButtonId{b.devId, b.axisYId});

  if (xmin_b)
    mask_with_single_button(m, b.minXBtn);
  if (xmax_b)
    mask_with_single_button(m, b.maxXBtn);
  if (ymin_b)
    mask_with_single_button(m, b.minYBtn);
  if (ymax_b)
    mask_with_single_button(m, b.maxYBtn);
  for (int i = 0; i < b.modCnt; i++)
    mask_with_single_button(m, b.mod[i]);
}

static bool is_binding_controls_masked(const UsedControlsMask &m, const dainput::DigitalActionBinding &b)
{
  if (b.btnCtrl)
  {
    int idx = get_btn_bs_idx(b.devId, b.ctrlId);
    if (idx >= 0 && m.bt.get(idx))
      return true;
  }
  else
  {
    int idx = get_axis_as_idx(b.devId, b.ctrlId);
    if (idx >= 0 && m.ax.get(idx))
      return true;
  }

  return false;
}
static bool is_binding_axis_masked(const UsedControlsMask &m, const dainput::AnalogAxisActionBinding &b)
{
  int idx = get_axis_as_idx(b.devId, b.axisId);
  if (idx >= 0 && m.ax.get(idx))
    return true;
  return false;
}
static bool is_binding_axis_masked(const UsedControlsMask &m, const dainput::AnalogStickActionBinding &b)
{
  int idx = get_axis_as_idx(b.devId, b.axisXId);
  if (idx >= 0 && m.ax.get(idx))
    return true;
  idx = get_axis_as_idx(b.devId, b.axisYId);
  if (idx >= 0 && m.ax.get(idx))
    return true;
  return false;
}

static float calc_smoothing(float axis, float smoothed_val, float dt, float smoothing)
{
  float curSmoothing = axis != 0.f ? smoothing : 0.f;
  if (sign(smoothed_val) != sign(axis))
    smoothed_val = 0.f;
  return approach(smoothed_val, axis, dt, curSmoothing);
}

static uint32_t encodeEventTriggered(dainput::action_handle_t a, bool end, int dur_ms)
{
  return (a & 0xFFFFu) | (end ? 0x80000000u : 0u) | ((dur_ms & 0x7FFFu) << 16);
}
static void sendEventTriggered(uint32_t code)
{
  dainput::action_handle_t a = code & 0xFFFFu;

  G_ASSERTF_RETURN(dainput::is_controled_by_cur_thread(), void(),
    "trying to call the sendEventTriggered for '%s' action not from the controlled thread", dainput::get_action_name(a));

  bool end = (code & 0x80000000u) != 0;
  int dur_ms = (code >> 16) & 0x7FFFu;

  if (dainput::notify_ui_action_triggered_cb && (dainput::get_action_flags(a) & dainput::ACTIONF_eventToGui))
    dainput::notify_ui_action_triggered_cb(a, end, dur_ms);

  if (dainput::event_logs && dainput::logscreen)
    dainput::logscreen(String(0, "%s%s  [dur=%d]", dainput::get_action_name(a), end ? ":end" : "", dur_ms));

  if (dainput::notify_action_triggered_cb)
    dainput::notify_action_triggered_cb(a, end, dur_ms);
}

template <typename T>
static bool is_modifier_key_pressed(const T &action_binding)
{
  for (int modIdx = 0; modIdx < action_binding.modCnt; modIdx++)
  {
    if (!get_btn_state(action_binding.mod[modIdx].devId, action_binding.mod[modIdx].btnId))
      return false;
  }

  return true;
}

static void process_actions(uint32_t t)
{
  using namespace dainput;

  if (binding_rec.a != BAD_ACTION_HANDLE)
    return binding_rec.process();

  for (auto &a : agData.ad)
  {
    a.bActivePrev = a.bActive;
    a.bActive = false;
  }
  for (auto &a : agData.aa)
  {
    a.bActivePrev = a.bActive;
    a.bActive = false;
  }
  for (auto &a : agData.as)
  {
    a.bActivePrev = a.bActive;
    a.bActive = false;
  }

  Tab<DigitalActionProgress> daProgress;
  Tab<uint32_t> pendingEvents;
  UsedControlsMask umask, next_umask;
  umask = start_umask;
  next_umask = umask;
  for (int i = dgs_app_active ? actionSetStack.size() - 1 : -1; i >= 0; i--)
  {
    ActionSet &set = actionSets[actionSetStack[i]];
    for (action_handle_t a : set.actions)
      switch (a & TYPEGRP__MASK)
      {
        case TYPEGRP_DIGITAL:
        {
          dainput::DigitalActionData &s = agData.ad[a & ~TYPEGRP__MASK];
          if (s.bActive || !s.bEnabled) // skip second processing of action in another actionset
            continue;
          bool new_state = false, triggered = false;
          bool cur_state_btn = false;
          bool prev_stickyToggle = s.stickyToggle;
          bool vr_action_triggered = false;
          if (dev5_vr && dev5_vr->isInUsableState())
          {
            auto vra = dev5_vr->getDigitalActionState(a);
            if (vra.isActive && vra.state)
              new_state = cur_state_btn = true;
            vr_action_triggered = vra.hasChanged && vra.isActive && vra.state;
          }
          for (unsigned c = 0, col_amask = dainput::colActiveMask; c < agData.bindingsColumnCount; c++, col_amask >>= 1)
          {
            if (!(col_amask & 1))
              continue;
            DigitalActionBinding &b = agData.adb[agData.get_binding_idx(a, c)];
            if (b.devId == dainput::DEV_none || b.devId == dainput::DEV_nullstub || (new_state && !b.stickyToggle) ||
                is_binding_controls_masked(umask, b))
              continue;

            const bool modif_pressed = is_modifier_key_pressed(b);

            // compute explicit control change
            int ctrl_change = 0;
            if (b.btnCtrl)
            {
              int idx = get_btn_bs_idx(b.devId, b.ctrlId);
              if (idx >= 0)
                ctrl_change = calc_change(btState.get(idx), btStatePrev.get(idx));
              if (b.unordCombo)
                for (int m = 0; !ctrl_change && m < b.modCnt; m++)
                {
                  idx = get_btn_bs_idx(b.mod[m].devId, b.mod[m].btnId);
                  if (idx >= 0)
                    ctrl_change = calc_change(btState.get(idx), btStatePrev.get(idx));
                }
            }
            else
            {
              int idx = get_axis_as_idx(b.devId, b.ctrlId);
              if (idx >= 0)
                ctrl_change = calc_change(int(floor(axState[idx] / 0.12f)) > b.axisCtrlThres,
                  int(floor(axStatePrev[idx] / 0.12f)) > b.axisCtrlThres);
            }

            if (!cur_state_btn && modif_pressed)
              cur_state_btn = b.btnCtrl ? get_btn_state(b.devId, b.ctrlId)
                                        : (int(floor(get_axis_state(b.devId, b.ctrlId) / 0.12f)) > b.axisCtrlThres);

            // compute current state
            bool cur_new_state = new_state;
            if (!b.btnCtrl)
              new_state = modif_pressed && (int(floor(get_axis_state(b.devId, b.ctrlId) / 0.12f)) > b.axisCtrlThres);
            else
              switch (b.eventType)
              {
                case DigitalActionBinding::BTN_pressed:
                case DigitalActionBinding::BTN_released:
                case DigitalActionBinding::BTN_released_short:
                case DigitalActionBinding::BTN_released_long: new_state = modif_pressed && get_btn_state(b.devId, b.ctrlId); break;
                case DigitalActionBinding::BTN_pressed_long:
                  new_state = modif_pressed && get_btn_long_press_state(b.devId, b.ctrlId, t);
                  break;
                case DigitalActionBinding::BTN_pressed2:
                  new_state = modif_pressed && get_btn_rep_click_state(b.devId, b.ctrlId, 2);
                  break;
                case DigitalActionBinding::BTN_pressed3:
                  new_state = modif_pressed && get_btn_rep_click_state(b.devId, b.ctrlId, 3);
                  break;
              }

            unsigned prev_longPressBegan = s.longPressBegan;
            if (b.btnCtrl &&
                (b.eventType == DigitalActionBinding::BTN_pressed_long || b.eventType == DigitalActionBinding::BTN_released_long))
            {
              if (modif_pressed && ctrl_change == 1 && !s.longPressBegan)
              {
                s.longPressBegan = true;
                s.iCol = c;
              }
              else if (s.longPressBegan && s.iCol == c && (!modif_pressed || !get_btn_state(b.devId, b.ctrlId)))
                s.longPressBegan = false;

              // prepare data for progress reporting
              if (notify_progress_changed_cb)
              {
                if (!prev_longPressBegan && s.longPressBegan)
                  daProgress.push_back() = DigitalActionProgress{
                    a, DigitalActionProgress::ST_in_progress, int16_t(b.eventType), int(t - get_btn_press_dur(b.devId, b.ctrlId, t))};
                else if (prev_longPressBegan && (!modif_pressed || !get_btn_state(b.devId, b.ctrlId)))
                {
                  if (get_btn_press_dur(b.devId, b.ctrlId, t) < dainput::agData.longPressDur)
                    daProgress.push_back() = DigitalActionProgress{a, DigitalActionProgress::ST_cancelled, int16_t(b.eventType), 0};
                }
              }
            }

            if (b.stickyToggle)
            {
              bool toggle_triggered = false;
              switch (b.eventType)
              {
                case DigitalActionBinding::BTN_pressed:
                case DigitalActionBinding::BTN_pressed2:
                case DigitalActionBinding::BTN_pressed3: toggle_triggered = new_state && ctrl_change == 1; break;
                case DigitalActionBinding::BTN_pressed_long:
                  toggle_triggered = new_state && !s.stickyTogglePressed && s.longPressBegan;
                  break;
                case DigitalActionBinding::BTN_released: toggle_triggered = modif_pressed && ctrl_change == -1; break;
                case DigitalActionBinding::BTN_released_short:
                  toggle_triggered = modif_pressed && ctrl_change == -1 &&
                                     (!b.btnCtrl || get_btn_press_dur(b.devId, b.ctrlId, t) < agData.longPressDur);
                  break;
                case DigitalActionBinding::BTN_released_long:
                  toggle_triggered = modif_pressed && ctrl_change == -1 &&
                                     (!b.btnCtrl || get_btn_press_dur(b.devId, b.ctrlId, t) >= agData.longPressDur);
                  break;
              }
              if (toggle_triggered && new_state)
                s.stickyTogglePressed = true;
              else if (ctrl_change == -1)
                s.stickyTogglePressed = false;

              if (toggle_triggered)
              {
                s.stickyToggle = !s.stickyToggle;
                if (!b.btnCtrl && new_state)
                  s.t0msec = t;
                else if (b.btnCtrl && !s.evReported && get_btn_state(b.devId, b.ctrlId) && s.t0msec < t)
                  s.t0msec = t - get_btn_press_dur(b.devId, b.ctrlId, t);
              }
              new_state = cur_new_state;
              continue;
            }

            if (!s.bState && new_state && ctrl_change == 1)
              s.iCol = c;

            if (!b.btnCtrl && !s.bState && new_state)
              s.t0msec = t;
            else if (b.btnCtrl && !s.evReported && get_btn_state(b.devId, b.ctrlId) && s.t0msec < t)
              s.t0msec = t - get_btn_press_dur(b.devId, b.ctrlId, t);

            // trigger actions
            switch (b.eventType)
            {
              case DigitalActionBinding::BTN_pressed:
              case DigitalActionBinding::BTN_pressed2:
              case DigitalActionBinding::BTN_pressed3:
                if (!s.bState && new_state && ctrl_change == 1)
                  triggered = true;
                break;
              case DigitalActionBinding::BTN_pressed_long:
                if (!s.bState && new_state && prev_longPressBegan)
                  triggered = true;
                break;

              case DigitalActionBinding::BTN_released:
                if (s.bState && !new_state && s.iCol == c && ctrl_change == -1)
                  triggered = true;
                break;
              case DigitalActionBinding::BTN_released_short:
                if (s.bState && !new_state && s.iCol == c && ctrl_change == -1 &&
                    (!b.btnCtrl || get_btn_press_dur(b.devId, b.ctrlId, t) < agData.longPressDur))
                  triggered = true;
                break;
              case DigitalActionBinding::BTN_released_long:
                if (s.bState && !new_state && s.iCol == c && ctrl_change == -1 && prev_longPressBegan &&
                    (!b.btnCtrl || get_btn_press_dur(b.devId, b.ctrlId, t) >= agData.longPressDur))
                  triggered = true;
                break;
            }
            if (new_state || s.bState)
            {
              mask_with_binding_controls(next_umask, b);
              if (s.flags & ACTIONF_maskImmediate)
                mask_with_binding_controls(umask, b);
            }
          }
          if (s.longPressBegan && !cur_state_btn)
          {
            s.longPressBegan = s.bState = false;
            if (notify_progress_changed_cb)
              daProgress.push_back() = DigitalActionProgress{a, DigitalActionProgress::ST_cancelled, 0, 0};
          }
          if (!cur_state_btn && s.bState && s.iCol >= 0 && !triggered)
            switch (agData.adb[agData.get_binding_idx(a, s.iCol)].eventType)
            {
              case DigitalActionBinding::BTN_released:
              case DigitalActionBinding::BTN_released_short:
              case DigitalActionBinding::BTN_released_long:
                s.bState = false;
                s.iCol = -1;
                break;
            }

          if (s.stickyToggle)
          {
            new_state = !new_state;
            triggered = new_state && !s.bState;
          }
          else if (prev_stickyToggle != s.stickyToggle)
            triggered = new_state && !s.bState;

          if (vr_action_triggered)
            triggered = true;
          s.bState = new_state;
          s.bActive = true;

          int dur_ms = t - s.t0msec;
          if (triggered)
          {
            pendingEvents.push_back(encodeEventTriggered(a, false, dur_ms));
            s.evReported = true;
            s.t0msec = t;
            dur_ms = 0;
          }
          if (s.evReported && !new_state)
          {
            pendingEvents.push_back(encodeEventTriggered(a, true, dur_ms));
            s.evReported = false;
          }
        }
        break;

        case TYPEGRP_AXIS:
        {
          dainput::AnalogAxisActionData &s = agData.aa[a & ~TYPEGRP__MASK];
          if (s.bActive || !s.bEnabled) // skip second processing of action in another actionset
            continue;
          bool stateful = s.flags & ACTIONF_stateful;
          uint16_t type = get_action_type(a);
          float new_x = stateful ? s.x : 0, new_v = 0;
          if (dev5_vr && dev5_vr->isInUsableState())
          {
            auto vra = dev5_vr->getAnalogActionState(a);
            if (vra.isActive)
              new_x += vra.state, s.bActive = true;
          }
          for (unsigned c = 0, col_amask = dainput::colActiveMask; c < agData.bindingsColumnCount; c++, col_amask >>= 1)
          {
            if (!(col_amask & 1))
              continue;
            AnalogAxisActionBinding &b = agData.aab[agData.get_binding_idx(a, c)];
            if (b.devId == dainput::DEV_none)
              continue;

            const bool modif_pressed = is_modifier_key_pressed(b);
            if (!modif_pressed)
              continue;

            // compute axis state
            float x = 0;
            if (!is_binding_axis_masked(umask, b))
            {
              x = get_axis_state(b.devId, b.axisId);
              if (b.devId == dainput::DEV_gamepad && dainput::dev3_gpad)
              {
                int j_idx = dainput::dev3_gpad->getJointAxisForAxis(b.axisId);
                if (j_idx >= 0)
                {
                  float y = get_axis_state(b.devId, j_idx);
                  float squareK = 1.f - fabsf(fabsf(x) - fabsf(y)) / (fabsf(x) + fabsf(y) + 0.01f);
                  x *= 1.f + squareK * squareK * squareK * 0.414f;
                }
              }
            }
            if (fabsf(x) < b.deadZoneThres)
              x = 0;
            else if (b.deadZoneThres > 0)
              x /= 1.0f - b.deadZoneThres;
            x = calc_axis_non_lin(x, b.nonLin);
            if (stateful)
            {
              if (b.axisRelativeVal)
                new_v += x * b.relIncScale;
              //== for now we have no marks for inactive axis/device
              else if (fabsf(get_axis_state(b.devId, b.axisId)) > b.deadZoneThres)
                new_x = b.invAxis ? (type == TYPE_TRIGGER ? 1.0f - x : -x) : x;
              x = 0;
            }

            bool max_btn_p = !is_single_button_masked(umask, b.maxBtn) && get_btn_state(b.maxBtn.devId, b.maxBtn.btnId);
            bool min_btn_p = (type == TYPE_STEERWHEEL || stateful) && !is_single_button_masked(umask, b.minBtn) &&
                             get_btn_state(b.minBtn.devId, b.minBtn.btnId);
            if (max_btn_p)
              x = min(x + b.maxVal, b.maxVal);
            if (min_btn_p)
              x = max(x - b.maxVal, -b.maxVal);
            if (stateful)
            {
              bool inc_btn_p = !is_single_button_masked(umask, b.incBtn) && get_btn_state(b.incBtn.devId, b.incBtn.btnId);
              bool dec_btn_p = !is_single_button_masked(umask, b.decBtn) && get_btn_state(b.decBtn.devId, b.decBtn.btnId);
              if (inc_btn_p)
              {
                if (!b.instantIncDecBtn)
                  new_v += b.relIncScale;
                else if (!btStatePrev.get(get_btn_bs_idx(b.incBtn.devId, b.incBtn.btnId)))
                {
                  if (b.quantizeValOnIncDecBtn)
                    x = floorf((new_x + x + b.relIncScale) / b.relIncScale + 0.5f) * b.relIncScale - new_x;
                  else
                    x += b.relIncScale;
                }
              }
              if (dec_btn_p)
              {
                if (!b.instantIncDecBtn)
                  new_v -= b.relIncScale;
                else if (!btStatePrev.get(get_btn_bs_idx(b.decBtn.devId, b.decBtn.btnId)))
                {
                  if (b.quantizeValOnIncDecBtn)
                    x = floorf((new_x + x - b.relIncScale) / b.relIncScale + 0.5f) * b.relIncScale - new_x;
                  else
                    x -= b.relIncScale;
                }
              }
            }

            if (b.invAxis)
              x = type == TYPE_TRIGGER ? 1.0f - x : -x;

            new_x += x;
            s.bActive = true;
            mask_with_binding_controls(next_umask, b, min_btn_p, max_btn_p);
            if (s.flags & ACTIONF_maskImmediate)
              mask_with_binding_controls(umask, b, min_btn_p, max_btn_p);
          }

          if (s.bActive)
          {
            float prev_x = s.x;
            s.x = clamp(new_x, type == TYPE_TRIGGER ? 0.0f : -1.0f, 1.0f);
            s.v = new_v;
            if (notify_ui_action_triggered_cb && (get_action_flags(a) & ACTIONF_eventToGui))
              if (!s.bActivePrev || s.x != prev_x)
                pendingEvents.push_back(encodeEventTriggered(a, false, 0));
          }
        }
        break;

        case TYPEGRP_STICK:
        {
          dainput::AnalogStickActionData &s = agData.as[a & ~TYPEGRP__MASK];
          if (s.bActive || !s.bEnabled) // skip second processing of action in another actionset
            continue;
          uint16_t type = get_action_type(a);
          float new_x = 0, new_y = 0;
          float new_vx = 0, new_vy = 0;
          if (dev5_vr && dev5_vr->isInUsableState())
          {
            auto vra = dev5_vr->getStickActionState(a);
            if (vra.isActive)
              new_x += vra.state.x, new_y += vra.state.y, s.bActive = true;
          }
          for (unsigned c = 0, col_amask = dainput::colActiveMask; c < agData.bindingsColumnCount; c++, col_amask >>= 1)
          {
            if (!(col_amask & 1))
              continue;
            AnalogStickActionBinding &b = agData.asb[agData.get_binding_idx(a, c)];
            if (b.devId == dainput::DEV_none)
              continue;

            const bool modif_pressed = is_modifier_key_pressed(b);
            if (!modif_pressed)
              continue;

            // compute stick state
            float x = 0, y = 0;
            bool x_max_btn_p = false, x_min_btn_p = false, y_max_btn_p = false, y_min_btn_p = false;

            if (!is_binding_axis_masked(umask, b))
            {
              x = get_axis_state(b.devId, b.axisXId);
              y = get_axis_state(b.devId, b.axisYId);
              if (type == TYPE_STICK && (b.devId == dainput::DEV_gamepad || b.devId == dainput::DEV_joy))
              {
                float d2 = x * x + y * y;
                if (d2 > 1)
                {
                  float d = sqrtf(d2);
                  x /= d;
                  y /= d;
                }
              }
            }
            if (type == TYPE_SYSMOUSE && b.devId == DEV_pointing)
              x = y = 0;
            else
            {
              float d2 = x * x + y * y;
              if (d2 < b.deadZoneThres * b.deadZoneThres)
                x = y = 0;
              else if (b.deadZoneThres > 0)
              {
                float s1 = b.deadZoneThres / sqrtf(d2), invs = 1.f - b.deadZoneThres;
                x = (x - x * s1) / invs;
                y = (y - y * s1) / invs;
              }
              x = calc_axis_non_lin(x, b.nonLin);
              y = calc_axis_non_lin(y, b.nonLin);
              if (fabsf(x) > 1e-4 && fabsf(y / x) < b.axisSnapAngK)
                y = 0;
              else if (fabsf(y) > 1e-4 && fabsf(x / y) < b.axisSnapAngK)
                x = 0;

              x_max_btn_p = !is_single_button_masked(umask, b.maxXBtn) && get_btn_state(b.maxXBtn.devId, b.maxXBtn.btnId);
              x_min_btn_p = !is_single_button_masked(umask, b.minXBtn) && get_btn_state(b.minXBtn.devId, b.minXBtn.btnId);
              y_max_btn_p = !is_single_button_masked(umask, b.maxYBtn) && get_btn_state(b.maxYBtn.devId, b.maxYBtn.btnId);
              y_min_btn_p = !is_single_button_masked(umask, b.minYBtn) && get_btn_state(b.minYBtn.devId, b.minYBtn.btnId);
              if (x_max_btn_p)
                x = min(x + b.maxVal, b.maxVal);
              if (x_min_btn_p)
                x = max(x - b.maxVal, -b.maxVal);

              if (y_max_btn_p)
                y = min(y + b.maxVal, b.maxVal);
              if (y_min_btn_p)
                y = max(y - b.maxVal, -b.maxVal);

              if (b.axisXinv)
                x = -x;
              if (b.axisYinv)
                y = -y;
            }
            if (b.devId == DEV_pointing && (b.axisXId | b.axisYId) < 2)
              x *= s.mmScaleX, y *= s.mmScaleY;
            else if (b.devId == DEV_gamepad || b.devId == DEV_joy)
              x *= s.gaScaleX, y *= s.gaScaleY;

            if (type == TYPE_ABSMOUSE && b.devId != DEV_pointing)
            {
              new_vx += x * b.sensScale;
              new_vy += y * b.sensScale;
            }
            else
            {
              new_x += x * b.sensScale;
              new_y += y * b.sensScale;
            }
            s.bActive = true;
            mask_with_binding_controls(next_umask, b, x_min_btn_p, x_max_btn_p, y_min_btn_p, y_max_btn_p);
            if (s.flags & ACTIONF_maskImmediate)
              mask_with_binding_controls(umask, b, x_min_btn_p, x_max_btn_p, y_min_btn_p, y_max_btn_p);
          }

          if (s.bActive)
          {
            float prev_x = s.x, prev_y = s.y;
            if (type == TYPE_STICK)
            {
              s.x = clamp(new_x, -1.0f, 1.0f);
              s.y = clamp(new_y, -1.0f, 1.0f);
            }
            else if (type == TYPE_ABSMOUSE || type == TYPE_STICK_DELTA)
            {
              s.x += new_x;
              s.y += new_y;
              s.vx = new_vx;
              s.vy = new_vy;
            }
            else if (type == TYPE_SYSMOUSE)
            {
              s.x = get_axis_state(DEV_pointing, 2);
              s.y = get_axis_state(DEV_pointing, 3);
              sysMouseVx = new_x;
              sysMouseVy = new_y;
            }
            if (notify_ui_action_triggered_cb && (get_action_flags(a) & ACTIONF_eventToGui))
              if (!s.bActivePrev || s.x != prev_x || s.y != prev_y)
                pendingEvents.push_back(encodeEventTriggered(a, false, 0));
          }
        }
        break;
      }
    if (dainput::breaking_set_handle == actionSetStack[i])
      break;
    umask = next_umask;
  }

  if (!dgs_app_active)
    for (int i = 0; i < agData.ad.size(); i++)
    {
      dainput::DigitalActionData &s = agData.ad[i];
      if (s.evReported)
      {
        pendingEvents.push_back(encodeEventTriggered(action_handle_t(i | TYPEGRP_DIGITAL), true, t - s.t0msec));
        s.evReported = false;
      }
    }

  btStatePrev = btState;
  axStatePrev = axState;
  axState[AS_OFS_pointing + 0] = 0;
  axState[AS_OFS_pointing + 1] = 0;

  for (uint32_t e : pendingEvents)
    sendEventTriggered(e);

  if (notify_progress_changed_cb)
    report_digital_action_progress(make_span(daProgress), pendingEvents);

  if (notify_ui_action_triggered_cb)
  {
    for (auto &a : agData.aa)
      if (a.bActivePrev && !a.bActive && (a.flags & ACTIONF_eventToGui))
        sendEventTriggered(encodeEventTriggered(a.nameId, false, 0));
    for (auto &a : agData.as)
      if (a.bActivePrev && !a.bActive && (a.flags & ACTIONF_eventToGui))
        sendEventTriggered(encodeEventTriggered(a.nameId, false, 0));
  }
}

static void update_btn_state(int glob_btn_id, bool pressed, uint32_t t)
{
  if (pressed)
  {
    if (!btState.get(glob_btn_id))
    {
      if (t < clState[glob_btn_id].t0 + dainput::agData.dblClickDur)
        clState[glob_btn_id].clickCnt++;
      else
        clState[glob_btn_id].clickCnt = 1;
      clState[glob_btn_id].t0 = t;
      if (dainput::event_logs)
        debug("dainput: btn[%d].t0=%d", glob_btn_id, clState[glob_btn_id].t0);
      btState.set(glob_btn_id);
    }
  }
  else
    btState.clr(glob_btn_id);
}

bool dainput::reset_digital_action_sticky_toggle(action_handle_t a)
{
  if ((a & TYPEGRP__MASK) != TYPEGRP_DIGITAL)
    return false;

  dainput::DigitalActionData &s = agData.ad[a & ~TYPEGRP__MASK];
  if (!s.stickyToggle)
    return false;

  s.stickyToggle = 0;
  s.bState = !s.bState;
  if (!s.bActive)
    return false;

  if (s.bState)
  {
    sendEventTriggered(encodeEventTriggered(a, false, 0));
    s.evReported = true;
  }
  if (s.evReported && !s.bState)
  {
    sendEventTriggered(encodeEventTriggered(a, true, 0));
    s.evReported = false;
  }
  return true;
}

static void report_digital_action_progress(dag::Span<dainput::DigitalActionProgress> daProgress, dag::ConstSpan<uint32_t> curEvents)
{
  using namespace dainput;

  bool changed = daProgress.size() > 0;
  for (uint32_t code : curEvents)
    for (dainput::DigitalActionProgress &p : lastReportedDaProgress)
      if (p.action == (code & 0xFFFFu))
      {
        p.state = p.ST_finished;
        changed = true;
        agData.ad[p.action & ~TYPEGRP__MASK].longPressBegan = false;
      }

  for (dainput::DigitalActionProgress &np : daProgress)
  {
    bool exists = false;
    for (dainput::DigitalActionProgress &p : lastReportedDaProgress)
      if (p.action == np.action)
      {
        p = np;
        exists = true;
      }
    if (!exists && np.state == np.ST_in_progress)
      lastReportedDaProgress.push_back(np);
  }
  if (changed && lastReportedDaProgress.size())
    notify_progress_changed_cb(lastReportedDaProgress);

  for (int i = 0; i < lastReportedDaProgress.size(); i++)
    if (lastReportedDaProgress[i].state != dainput::DigitalActionProgress::ST_in_progress)
    {
      erase_items_fast(lastReportedDaProgress, i, 1);
      i--;
    }
}

static void emergency_events_dispatch()
{
  int avail = input_queue.getWriteAvailableSize();
  if (avail > 8 && avail < (2 << 10))
  {
    logwarn("input_queue nearly overflowed, writeAvailableSize=%d, %s flush here", avail,
      dainput::is_controled_by_cur_thread() ? "will" : "CAN'T");
    if (dainput::is_controled_by_cur_thread())
      dainput::process_actions_tick();
  }
}
bool dainput::is_safe_to_sample_input() { return time0_us.load() != 0 && input_queue.getWriteAvailableSize() > (4 << 10); }

void dainput::kbd_btn_event_occurs(int64_t t_usec, int btn_id, bool pressed)
{
  dev_kbd_lastInputOnFrameNo = dagor_frame_no();
  G_ASSERTF(btn_id >= 0 && btn_id < BS_OFS_pointing - BS_OFS_kbd, "btn_id=%d", btn_id);

  last_queued_t_us = t_usec;
  if (!pressed)
    dev_clicks_count[get_const_log2(dainput::DEV_USED_kbd)]++;
  emergency_events_dispatch();
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_btn | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  cwr->writeIntP<2>((BS_OFS_kbd + btn_id) | (pressed ? 0x8000 : 0));
  cwr->endBlock();
  input_queue.endWrite();
}
void dainput::mouse_btn_event_occurs(int64_t t_usec, int btn_id, bool pressed)
{
  if (btn_id == 5 || btn_id == 6)
    return;
#if !(_TARGET_C1 | _TARGET_C2)
  dev_mouse_lastInputOnFrameNo = dagor_frame_no();
#endif
  G_ASSERTF(btn_id >= 0 && btn_id < BS_OFS_gamepad - BS_OFS_pointing, "btn_id=%d", btn_id);

  last_queued_t_us = t_usec;
  if (!pressed)
    dev_clicks_count[get_const_log2(dainput::DEV_USED_mouse)]++;
  emergency_events_dispatch();
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_btn | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  cwr->writeIntP<2>((BS_OFS_pointing + btn_id) | (pressed ? 0x8000 : 0));
  cwr->endBlock();
  input_queue.endWrite();
}
void dainput::touch_event_occurs(int64_t)
{
  // we need touch event only for activate change control type
  // and don't use it for sending actions
  // Touch UI does not bind actions like mouse, keyboard or gamepad
  // Touch sends actions in another place
  dev_touch_lastInputOnFrameNo = dagor_frame_no();
}
void dainput::mouse_wheel_event_occurs(int64_t t_usec, int wheel_inc)
{
  dev_mouse_lastInputOnFrameNo = dagor_frame_no();
  int btn_id = wheel_inc > 0 ? 5 : 6;

  last_queued_t_us = t_usec;
  emergency_events_dispatch();
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_btn | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  cwr->writeIntP<2>((BS_OFS_pointing + btn_id) | 0x8000);
  cwr->endBlock();
  cwr->writeInt64(INPMSG_btn | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  cwr->writeIntP<2>(BS_OFS_pointing + btn_id);
  cwr->endBlock();
  input_queue.endWrite();
}
void dainput::mouse_move_event_occurs(int64_t t_usec, float dx, float dy, int scr_x, int scr_y)
{
  if (dx != 0.f || dy != 0.f)
    dev_mouse_lastInputOnFrameNo = dagor_frame_no();
  last_queued_t_us = t_usec;
  emergency_events_dispatch();
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_mmove | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  cwr->writeReal(dx);
  cwr->writeReal(dy);
  cwr->writeInt(scr_x);
  cwr->writeInt(scr_y);
  cwr->endBlock();
  input_queue.endWrite();
}
static bool check_joy_used(int bs_ofs, int bs_cnt, int as_ofs, int as_cnt)
{
  for (int i = bs_ofs; i < bs_ofs + bs_cnt; i++)
    if (btState.get(i) != btStatePrev.get(i))
      return true;
  for (int i = as_ofs; i < as_ofs + as_cnt; i++)
    if (fabsf(axState[i] - axStatePrev[i]) > 0.25)
      return true;
  return false;
}
static unsigned write_joy_input_event(HumanInput::IGenJoystick *j, int bs_ofs, int as_ofs, IGenSave &cwr, int bs_max, int as_max)
{
  unsigned clicks = 0;
  if (as_max > j->getAxisCount())
    as_max = j->getAxisCount();
  for (int i = 0; i < as_max; i++)
  {
    int id = as_ofs + i;
    if (input_queue_ax_state[id] != j->getAxisPosRaw(i))
    {
      cwr.writeIntP<2>(id | 0x4000);
      cwr.writeIntP<2>(input_queue_ax_state[id] = j->getAxisPosRaw(i));
    }
  }
  if (bs_max > j->getButtonCount())
    bs_max = j->getButtonCount();
  for (int i = 0; i < bs_max; i++)
  {
    int id = bs_ofs + i;
    bool old_state = input_queue_bt_state.get(id);
    bool new_state = j->getButtonState(i);
    if (old_state != new_state)
    {
      new_state ? input_queue_bt_state.set(id) : input_queue_bt_state.clr(id);
      cwr.writeIntP<2>(id | (new_state ? 0x8000 : 0));
      if (!new_state)
        clicks++;
    }
  }
  return clicks;
}
void dainput::gpad_change_event_occurs(int64_t t_usec, HumanInput::IGenJoystick *gamepad)
{
  last_queued_t_us = t_usec;
  emergency_events_dispatch();
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_ax_btn | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  dev_clicks_count[get_const_log2(dainput::DEV_USED_gamepad)] +=
    write_joy_input_event(gamepad, BS_OFS_gamepad, AS_OFS_gamepad, *cwr, BS_OFS_joy - BS_OFS_gamepad, AS_OFS_joy - AS_OFS_gamepad);
  cwr->endBlock();
  input_queue.endWrite();
}
void dainput::joy_change_event_occurs(int64_t t_usec, HumanInput::IGenJoystick *joy)
{
  last_queued_t_us = t_usec;
  emergency_events_dispatch();
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_ax_btn | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  write_joy_input_event(joy, BS_OFS_joy, AS_OFS_joy, *cwr, BS_OFS_end - BS_OFS_joy, AS_OFS_end - AS_OFS_joy);
  cwr->endBlock();
  input_queue.endWrite();
}
void dainput::tick_event_occurs(int64_t t_usec)
{
  if (last_queued_t_us + 2000 >= t_usec)
    return;
  last_queued_t_us = t_usec;
  IGenSave *cwr = input_queue.startWrite();
  cwr->writeInt64(INPMSG_tick | (t_usec << INPMSG_time_shift));
  cwr->beginBlock();
  cwr->endBlock();
  // if (dainput::event_logs)
  //   debug("dainput::tick_event_occurs(%lld) t=%d ms (ticks=%lld)", t_usec, get_rel_time_msec(t_usec), ref_time_ticks());
  input_queue.endWrite();
}

void dainput::reset_input_state()
{
  input_queue.startWrite();
  input_queue_bt_state.reset();
  mem_set_0(input_queue_ax_state);
  last_processed_t_us = 0;
  last_queued_t_us = 0;
  time0_us = 0;
  debug("dainput::reset_input_state()  time0_us=%lld", time0_us.load());
  input_queue.endWrite();

  btState.reset();
  mem_set_0(clState);
  mem_set_0(axState);
  sysMouseVx = sysMouseVy = 0;
  sysMouseResDx = sysMouseResDy = 0;
  clear_and_shrink(lastReportedDaProgress);
  memset(dev_clicks_count, 0, sizeof(dev_clicks_count));
  memset(dev_clicks_count_last, 0, sizeof(dev_clicks_count_last));
}

static int64_t get_processed_dt(int64_t t)
{
  int64_t dt = 0;
  if (last_processed_t_us)
    dt = t - last_processed_t_us;
  last_processed_t_us = t;
  return dt;
}

static int64_t dispatch_input_message(IGenLoad &crd)
{
  int64_t tag = crd.readInt64();
  unsigned t_msec = get_rel_time_msec(tag >> INPMSG_time_shift);

  crd.beginBlock();
  switch (tag & INPMSG_type_mask)
  {
    case INPMSG_btn:
    {
      unsigned btn = crd.readIntP<2>();
      update_btn_state(btn & ~0x8000, (btn & 0x8000) != 0, t_msec);
      break;
    }

    case INPMSG_mmove:
      axState[AS_OFS_pointing + 0] += crd.readReal();
      axState[AS_OFS_pointing + 1] -= crd.readReal();
      axState[AS_OFS_pointing + 2] = crd.readInt();
      axState[AS_OFS_pointing + 3] = crd.readInt();
      break;

    case INPMSG_ax_btn:
    {
      unsigned code = 0;
      for (int wcnt = crd.getBlockRest() / 2; wcnt; wcnt--)
      {
        code = crd.readIntP<2>();
        if (code & 0x4000)
          axState[code & ~0x4000] = short(crd.readIntP<2>()) / 32000.0f, wcnt--;
        else
          update_btn_state(code & ~0x8000, (code & 0x8000) != 0, t_msec);
      }

      if (dev_gamepad_lastInputOnFrameNo != dagor_frame_no())
      {
        bool joy = (code & ~0xC000) >= ((code & 0x4000) ? (int)AS_OFS_joy : (int)BS_OFS_joy);
        if (joy && check_joy_used(BS_OFS_joy, BS_OFS_end - BS_OFS_joy, AS_OFS_joy, AS_OFS_end - AS_OFS_joy))
          dev_joy_lastInputOnFrameNo = dagor_frame_no();
        else if (!joy && check_joy_used(BS_OFS_gamepad, BS_OFS_joy - BS_OFS_gamepad, AS_OFS_gamepad, AS_OFS_joy - AS_OFS_gamepad))
          dev_gamepad_lastInputOnFrameNo = dagor_frame_no();
      }
      break;
    }

    case INPMSG_tick: break;
  }
  crd.endBlock();
  return tag >> INPMSG_time_shift;
}

static void process_one_sample(int64_t t, dainput::actions_processed_t on_input_sample_processed)
{
  using namespace dainput;
  int64_t dt_usec = get_processed_dt(t);
  float dt = dt_usec * 1e-6;

  for (dainput::AnalogAxisActionData &aa : agData.aa)
    if (aa.flags & ACTIONF_stateful)
      if (aa.bActive)
      {
        aa.x += aa.v * dt;
        aa.v = 0;
      }
  for (dainput::AnalogStickActionData &as : agData.as)
    if (as.type == TYPE_ABSMOUSE || as.type == TYPE_STICK_DELTA)
      if (as.bActive)
      {
        if (as.smoothValue > 0.f)
        {
          as.vx = as.sx = calc_smoothing(as.vx, as.sx, dt, as.smoothValue);
          as.vy = as.sy = calc_smoothing(as.vy, as.sy, dt, as.smoothValue);
        }
        as.x += as.vx * dt, as.y += as.vy * dt;
      }

  process_actions(get_rel_time_msec(t));

  if (on_input_sample_processed && dt_usec > 2000)
  {
    on_input_sample_processed(dt);
    for (dainput::AnalogStickActionData &as : agData.as)
      if (as.type == TYPE_ABSMOUSE || as.type == TYPE_STICK_DELTA)
        as.x = as.y = 0;
  }

  if (dainput::dev2_pnt && (sysMouseVx || sysMouseVy))
  {
    if (sysMouseVx * sysMouseResDx < 0)
      sysMouseResDx = 0;
    if (sysMouseVy * sysMouseResDy < 0)
      sysMouseResDy = 0;

    float dx = sysMouseVx * dt + sysMouseResDx;
    float dy = sysMouseVy * dt + sysMouseResDy;
    sysMouseResDx = dx - truncf(dx);
    sysMouseResDy = dy - truncf(dy);
    if (truncf(dx) || truncf(dy))
      dainput::dev2_pnt->setPosition(dainput::dev2_pnt->getRawState().mouse.x + truncf(dx),
        dainput::dev2_pnt->getRawState().mouse.y + truncf(dy));
    sysMouseVx = 0, sysMouseVy = 0;
  }
}

void dainput::process_actions_tick(dainput::actions_processed_t on_input_sample_processed)
{
  if (!time0_us.load())
  {
    time0_us = ref_time_delta_to_usec(rel_ref_time_ticks(ref_time_ticks(), -10000000));
    debug("dainput::process_actions_tick() -> time0_us=%lld (ticks=%lld)", time0_us.load(), ref_time_ticks());
  }

  dainput::tick_event_occurs(ref_time_delta_to_usec(ref_time_ticks()));
  for (dainput::AnalogStickActionData &as : agData.as)
    if (as.type == TYPE_ABSMOUSE || as.type == TYPE_STICK_DELTA)
      as.x = as.y = 0;

  int inp_msg_count = 0;
  if (IGenLoad *crd = input_queue.startRead(inp_msg_count))
  {
    for (; inp_msg_count; inp_msg_count--)
      process_one_sample(dispatch_input_message(*crd), on_input_sample_processed);
    input_queue.endRead();
  }

  if (memcmp(dev_clicks_count_last, dev_clicks_count, sizeof(dev_clicks_count_last)) != 0)
  {
    if (notify_clicks_cb)
    {
#define NOTIFY_CLICKS(DEV)                                                                                       \
  if ((notify_clicks_dev_mask & DEV_USED_##DEV) &&                                                               \
      dev_clicks_count[get_const_log2(DEV_USED_##DEV)] != dev_clicks_count_last[get_const_log2(DEV_USED_##DEV)]) \
  notify_clicks_cb(DEV_USED_##DEV, dev_clicks_count[get_const_log2(DEV_USED_##DEV)])

      NOTIFY_CLICKS(kbd);
      NOTIFY_CLICKS(mouse);
      NOTIFY_CLICKS(gamepad);
#undef NOTIFY_CLICKS
    }
    memcpy(dev_clicks_count_last, dev_clicks_count, sizeof(dev_clicks_count_last));
  }
}

void dainput::send_action_event(action_handle_t a) { sendEventTriggered(encodeEventTriggered(a, false, 0)); }
void dainput::send_action_terminated_event(action_handle_t a, int dur) { sendEventTriggered(encodeEventTriggered(a, true, dur)); }

void BindingsRecorder::resetBindings(dainput::action_handle_t _a)
{
  dainput::process_actions_tick(nullptr);
  a = _a;
  memset(&db, 0, sizeof(db));
  memset(&ab, 0, sizeof(ab));
  memset(&sb, 0, sizeof(sb));
  ab.axisId = sb.axisXId = sb.axisYId = dainput::AXIS_NULL_ID;
  mod = nullptr;
  switch (a & dainput::TYPEGRP__MASK)
  {
    case dainput::TYPEGRP_DIGITAL: mod = db.mod; break;
    case dainput::TYPEGRP_AXIS:
      mod = ab.mod;
      ab.maxVal = 1.0;
      break;
    case dainput::TYPEGRP_STICK:
      mod = sb.mod;
      sb.maxVal = sb.sensScale = 1;
      break;
  }
  btStateInitial = btState;
}

void BindingsRecorder::process()
{
  using namespace dainput;

  SingleButtonId btn_released = {DEV_none, 0};
  SingleButtonId axis_defl = {DEV_none, 0};
  int selected_dev = DEV_none;

  for (int i = 0, inc = 0; i < btStatePrev.SZ; i += inc)
    if (btStatePrev.getIter(i, inc) && !btState.get(i) && !btStateInitial.get(i))
    {
      btn_released = get_btn_from_bs_idx(i);
      break;
    }
  for (int i = 0; i < axState.size(); i++)
    if (fabsf(axStatePrev[i]) >= 0.5 && fabsf(axState[i]) < 0.5)
    {
      axis_defl = get_axis_from_as_idx(i);
      break;
    }

  switch (a & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL:
      if (btn_released.devId)
      {
        // button released
        db.devId = btn_released.devId;
        db.ctrlId = btn_released.btnId;
        db.btnCtrl = 1;
      }
      if (!db.devId && axis_defl.devId && axis_defl.devId != DEV_pointing)
      {
        // trigger released
        db.devId = axis_defl.devId;
        db.ctrlId = axis_defl.btnId;
        db.btnCtrl = 0;
      }
      selected_dev = db.devId;
      break;

    case TYPEGRP_AXIS:
      if (axis_defl.devId)
      {
        // axis deflected
        ab.devId = axis_defl.devId;
        ab.axisId = axis_defl.btnId;
      }
      if (!ab.devId && btn_released.devId && get_action_type(a) == TYPE_TRIGGER)
      {
        // maxBtn released
        ab.devId = DEV_nullstub;
        ab.axisId = 0;
        ab.maxBtn = btn_released;
      }
      selected_dev = (ab.devId == DEV_nullstub) ? ab.maxBtn.devId : ab.devId;
      break;

    case TYPEGRP_STICK:
      if (!sb.devId && axis_defl.devId)
      {
        // record first axis
        sb.devId = axis_defl.devId;
        sb.axisXId = axis_defl.btnId;
      }
      else if (axis_defl.devId && sb.devId == axis_defl.devId && sb.axisXId != axis_defl.btnId)
      {
        // record second axis
        if (sb.axisXId < axis_defl.btnId)
          sb.axisYId = axis_defl.btnId;
        else
        {
          sb.axisYId = sb.axisXId;
          sb.axisXId = axis_defl.btnId;
        }
      }
      selected_dev = sb.devId;
      break;

    case TYPEGRP__MASK: // SingleButtonId recorings
      if (btn_released.devId)
      {
        // button released
        db.devId = btn_released.devId;
        db.ctrlId = btn_released.btnId;
        db.btnCtrl = 1;
      }
      selected_dev = db.devId;
      break;
  }

  if (is_recording_complete())
  {
    // gather active modifer buttons on main control recording complete
    if (mod)
    {
      int modCnt = 0;
      for (int i = 0, inc = 0; modCnt < 3 && i < btState.SZ; i += inc)
        if (btState.getIter(i, inc) && !btStateInitial.get(i))
        {
          mod[modCnt] = get_btn_from_bs_idx(i);
          modCnt++;
        }

      switch (a & TYPEGRP__MASK)
      {
        case TYPEGRP_DIGITAL: db.modCnt = modCnt; break;
        case TYPEGRP_AXIS: ab.modCnt = modCnt; break;
        case TYPEGRP_STICK: sb.modCnt = modCnt; break;
      }
    }
  }

  btStatePrev = btState;
  axStatePrev = axState;
  axState[AS_OFS_pointing + 0] = 0;
  axState[AS_OFS_pointing + 1] = 0;
}

void dainput::start_recording_bindings(action_handle_t action) { binding_rec.resetBindings(action); }
void dainput::start_recording_bindings_for_single_button() { binding_rec.resetBindings(0xFFFEU); }
bool dainput::is_recording_in_progress() { return binding_rec.a != BAD_ACTION_HANDLE; }
bool dainput::is_recording_complete()
{
  if (binding_rec.a == BAD_ACTION_HANDLE)
    return true;
  switch (binding_rec.a & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: return binding_rec.db.devId;
    case TYPEGRP_AXIS: return binding_rec.ab.devId;
    case TYPEGRP_STICK:
      return binding_rec.sb.devId && binding_rec.sb.axisXId != AXIS_NULL_ID && binding_rec.sb.axisYId != AXIS_NULL_ID;
    case TYPEGRP__MASK: // SingleButtonId recorings
      if (binding_rec.a == 0xFFFEU)
        return binding_rec.db.devId;
  }
  return true;
}
bool dainput::finish_recording_bindings(DataBlock &out_binding)
{
  out_binding.clearData();
  if (binding_rec.a == BAD_ACTION_HANDLE)
    return false;
  bool done = dainput::is_recording_complete();
  if (done)
    switch (binding_rec.a & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: save_action_binding_digital(out_binding, binding_rec.db); break;
      case TYPEGRP_AXIS: save_action_binding_axis(out_binding, binding_rec.ab); break;
      case TYPEGRP_STICK: save_action_binding_stick(out_binding, binding_rec.sb); break;
      case TYPEGRP__MASK: // SingleButtonId recorings
        out_binding.setInt("dev", binding_rec.db.devId);
        out_binding.setInt("btn", binding_rec.db.ctrlId);
        break;
      default: done = false; break;
    }
  binding_rec.a = BAD_ACTION_HANDLE;
  return done;
}

void dainput::set_initial_usage_mask(dag::ConstSpan<SingleButtonId> disable_used_buttons, int ord_idx)
{
  if (!disable_used_buttons.size() || interlocked_acquire_load(pending_cnt_start_ubtn[ord_idx]))
  {
    // delay usage mask clear (or subsequent re-init) to next frame
    interlocked_increment(pending_cnt_start_ubtn[ord_idx]);
    add_delayed_action_buffered(make_delayed_action([dub = Tab<SingleButtonId>(disable_used_buttons, tmpmem), ord_idx]() {
      set_initial_usage_mask_immediate(dub, ord_idx);
      interlocked_decrement(pending_cnt_start_ubtn[ord_idx]);
    }));
  }
  else
    set_initial_usage_mask_immediate(disable_used_buttons, ord_idx);
}
void dainput::set_initial_usage_mask_immediate(dag::ConstSpan<SingleButtonId> disable_used_buttons, int ord_idx)
{
  G_ASSERTF_RETURN(ord_idx >= 0 && ord_idx < START_UM_LAYERS, , "set_initial_usage_mask() ord_idx=%d is out of range(0..%d)", ord_idx,
    START_UM_LAYERS - 1);
  if (!start_ubtn_layered[ord_idx].size() && !disable_used_buttons.size())
    return;

  // store layered buttons
  start_ubtn_layered[ord_idx] = disable_used_buttons;

  // rebuilt compound start_umask
  start_umask.bt.reset();
  start_umask.ax.reset();
  for (int i = 0; i < START_UM_LAYERS; i++)
    for (auto &sb : start_ubtn_layered[i])
      if (sb.devId & 0x10)
        mask_with_single_axis(start_umask, SingleButtonId{uint16_t(sb.devId & 0xFu), sb.btnId});
      else
        mask_with_single_button(start_umask, sb);
}

unsigned dainput::get_last_used_device_mask(unsigned past_frames_threshold)
{
#define SET_FLAG(DEV)                                                                                              \
  if (dev_##DEV##_lastInputOnFrameNo && dev_##DEV##_lastInputOnFrameNo + past_frames_threshold > dagor_frame_no()) \
    mask |= DEV_USED_##DEV;

  unsigned mask = 0;
#if !(_TARGET_C1 | _TARGET_C2 | _TARGET_XBOX)
  SET_FLAG(mouse);
  SET_FLAG(kbd);
  SET_FLAG(touch);
#endif
  SET_FLAG(gamepad);
  return mask;

#undef SET_FLAG
}

unsigned dainput::get_overall_button_clicks_count(unsigned dev_used_mask)
{
  unsigned clicks = 0;
#define ADD_CLICKS(DEV)               \
  if (dev_used_mask & DEV_USED_##DEV) \
  clicks += dev_clicks_count[get_const_log2(DEV_USED_##DEV)]
#if !(_TARGET_C1 | _TARGET_C2 | _TARGET_XBOX)
  ADD_CLICKS(mouse);
  ADD_CLICKS(kbd);
#endif
  ADD_CLICKS(gamepad);
#undef ADD_CLICKS
  return clicks;
}

const char *dainput::build_ctrl_name(String &out_name, int dev, int btn_or_axis, bool is_btn)
{
  switch (dev)
  {
    case DEV_kbd:
      if (dev1_kbd && is_btn)
        out_name.printf(0, "kbd.%s", dev1_kbd->getKeyName(btn_or_axis));
      else if (is_btn)
        out_name.printf(0, "kbd.btn%03d", btn_or_axis);
      else
        clear_and_shrink(out_name);
      break;
    case DEV_pointing:
      if (dev2_pnt && is_btn)
        out_name.printf(0, "mouse.%s", dev2_pnt->getBtnName(btn_or_axis));
      else if (is_btn)
        out_name.printf(0, "mouse.btn%d", btn_or_axis);
      else
        out_name.printf(0, "mouse.%c", btn_or_axis == 0 ? 'X' : 'Y');
      break;
    case DEV_gamepad:
      if (dev3_gpad)
        out_name.printf(0, "gpad.%s", is_btn ? dev3_gpad->getButtonName(btn_or_axis) : dev3_gpad->getAxisName(btn_or_axis));
      else
        out_name.printf(0, "gpad.%s%02d", is_btn ? "btn" : "axis", btn_or_axis);
      break;
    case DEV_joy:
      if (dev4_joy)
        out_name.printf(0, "joy.%s", is_btn ? dev4_joy->getButtonName(btn_or_axis) : dev4_joy->getAxisName(btn_or_axis));
      else
        out_name.printf(0, "joy.%s%02d", is_btn ? "btn" : "axis", btn_or_axis);
      break;

    case DEV_none:
    case DEV_nullstub:
    default: clear_and_shrink(out_name); break;
  }

  return out_name;
}

const char *dainput::get_digital_button_type_name(int eventType)
{
  switch (eventType)
  {
    case DigitalActionBinding::BTN_pressed: return "btn/press";
    case DigitalActionBinding::BTN_pressed_long: return "btn/long_press";
    case DigitalActionBinding::BTN_pressed2: return "btn/double_press";
    case DigitalActionBinding::BTN_pressed3: return "btn/tripple_press";
    case DigitalActionBinding::BTN_released: return "btn/release";
    case DigitalActionBinding::BTN_released_short: return "btn/short_release";
    case DigitalActionBinding::BTN_released_long: return "btn/long_release";
  }
  return NULL;
}

static void mask_with_action(UsedControlsMask &umask1, dainput::action_handle_t a, int b_idx)
{
  using namespace dainput;
  switch (a & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: mask_with_binding_controls(umask1, agData.adb[b_idx]); break;
    case TYPEGRP_AXIS: mask_with_binding_controls(umask1, agData.aab[b_idx], true, true); break;
    case TYPEGRP_STICK: mask_with_binding_controls(umask1, agData.asb[b_idx], true, true, true, true); break;
  }
}

static bool load_and_mask_action_binding(UsedControlsMask &umask0, dainput::action_handle_t a, const DataBlock &binding,
  dainput::DigitalActionBinding &adb, dainput::AnalogAxisActionBinding &aab, dainput::AnalogStickActionBinding &asb)
{
  using namespace dainput;
  switch (a & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL:
      load_action_binding_digital(binding, adb);
      mask_with_binding_controls(umask0, adb);
      break;
    case TYPEGRP_AXIS:
      load_action_binding_axis(binding, aab);
      mask_with_binding_controls(umask0, aab, true, true);
      break;
    case TYPEGRP_STICK:
      load_action_binding_stick(binding, asb);
      mask_with_binding_controls(umask0, asb, true, true, true, true);
      break;
    default: return false;
  }
  return true;
}

static bool is_action_masked(UsedControlsMask &umask0, dainput::action_handle_t a, dainput::DigitalActionBinding &adb,
  dainput::AnalogAxisActionBinding &aab, dainput::AnalogStickActionBinding &asb)
{
  using namespace dainput;
  bool masked = false;
  switch (a & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: masked = is_binding_controls_masked(umask0, adb); break;
    case TYPEGRP_AXIS:
      masked = is_binding_axis_masked(umask0, aab);
      masked |= is_single_button_masked(umask0, aab.minBtn);
      masked |= is_single_button_masked(umask0, aab.maxBtn);
      break;
    case TYPEGRP_STICK:
      masked = is_binding_axis_masked(umask0, asb);
      masked |= is_single_button_masked(umask0, asb.minXBtn);
      masked |= is_single_button_masked(umask0, asb.maxXBtn);
      masked |= is_single_button_masked(umask0, asb.minYBtn);
      masked |= is_single_button_masked(umask0, asb.maxYBtn);
      break;
  }
  return masked;
}

static bool is_action_masked(UsedControlsMask &umask0, dainput::action_handle_t a, int b_idx)
{
  using namespace dainput;

  bool masked = false;
  switch (a & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: masked = is_binding_controls_masked(umask0, agData.adb[b_idx]); break;
    case TYPEGRP_AXIS:
      masked = is_binding_axis_masked(umask0, agData.aab[b_idx]);
      masked |= is_single_button_masked(umask0, agData.aab[b_idx].minBtn);
      masked |= is_single_button_masked(umask0, agData.aab[b_idx].maxBtn);
      break;
    case TYPEGRP_STICK:
      masked = is_binding_axis_masked(umask0, agData.asb[b_idx]);
      masked |= is_single_button_masked(umask0, agData.asb[b_idx].minXBtn);
      masked |= is_single_button_masked(umask0, agData.asb[b_idx].maxXBtn);
      masked |= is_single_button_masked(umask0, agData.asb[b_idx].minYBtn);
      masked |= is_single_button_masked(umask0, agData.asb[b_idx].maxYBtn);
      break;
  }
  return masked;
}

bool dainput::check_bindings_conflicts(action_handle_t action, const DataBlock &binding, Tab<action_handle_t> &out_a, Tab<int> &out_c)
{
  dainput::DigitalActionBinding adb;
  dainput::AnalogAxisActionBinding aab;
  dainput::AnalogStickActionBinding asb;
  UsedControlsMask umask0, umask1;

  out_a.clear();
  out_c.clear();
  if (!load_and_mask_action_binding(umask0, action, binding, adb, aab, asb))
    return false;

  for (dainput::ActionSet &set : actionSets)
    if (find_value_idx(set.actions, action) >= 0)
      for (dainput::action_handle_t a : set.actions)
        if (a != action && (!get_action_excl_tag(action) || get_action_excl_tag(action) != get_action_excl_tag(a)))
          for (int c = 0; c < agData.bindingsColumnCount; c++)
          {
            if (is_action_binding_empty(a, c))
              continue;

            int b_idx = agData.get_binding_idx(a, c);
            mask_with_action(umask1, a, b_idx);
            bool conflicts = is_action_masked(umask0, a, b_idx);

            if (!conflicts)
              conflicts = is_action_masked(umask1, action, adb, aab, asb);

            if (conflicts)
            {
              bool already_added = false;
              for (int j = 0; j < out_a.size(); j++)
                if (out_a[j] == a && out_c[j] == c)
                {
                  already_added = true;
                  break;
                }
              if (!already_added)
              {
                out_a.push_back(a);
                out_c.push_back(c);
              }
            }
            umask1.reset();
          }

  return out_a.size() > 0;
}

bool dainput::check_bindings_conflicts_one(action_handle_t a1, int c1, action_handle_t a2, int c2)
{
  if (a1 == a2 || a1 == BAD_ACTION_HANDLE || a2 == BAD_ACTION_HANDLE)
    return false;
  if (is_action_binding_empty(a1, c1) || is_action_binding_empty(a2, c2))
    return false;
  if (get_action_excl_tag(a1) && get_action_excl_tag(a1) == get_action_excl_tag(a2))
    return false;

  UsedControlsMask umask1, umask2;
  int b_idx1 = agData.get_binding_idx(a1, c1), b_idx2 = agData.get_binding_idx(a2, c2);
  mask_with_action(umask1, a1, b_idx1);
  mask_with_action(umask2, a2, b_idx2);
  return is_action_masked(umask2, a1, b_idx1) || is_action_masked(umask1, a2, b_idx2);
}

bool dainput::check_bindings_hides_action(action_handle_t action, const DataBlock &binding, Tab<action_handle_t> &out_a,
  Tab<int> &out_c)
{
  dainput::DigitalActionBinding adb;
  dainput::AnalogAxisActionBinding aab;
  dainput::AnalogStickActionBinding asb;
  UsedControlsMask umask0;

  out_a.clear();
  out_c.clear();
  if (!load_and_mask_action_binding(umask0, action, binding, adb, aab, asb))
    return false;

  for (dainput::ActionSet &set : actionSets)
    if (find_value_idx(set.actions, action) < 0)
      for (dainput::action_handle_t a : set.actions)
        for (int c = 0; c < agData.bindingsColumnCount; c++)
          if (!is_action_binding_empty(a, c) && is_action_masked(umask0, a, agData.get_binding_idx(a, c)))
          {
            bool already_added = false;
            for (int j = 0; j < out_a.size(); j++)
              if (out_a[j] == a && out_c[j] == c)
              {
                already_added = true;
                break;
              }
            if (!already_added)
            {
              out_a.push_back(a);
              out_c.push_back(c);
            }
          }

  return out_a.size() > 0;
}

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
static float stickDz[4] = {0.10f, 0.12f, 0.f, 0.f};
#else
static float stickDz[4] = {0.15f, 0.20f, 0.f, 0.f};
#endif
static inline HumanInput::IGenJoystickClassDrv *joy_cls()
{
  return global_cls_composite_drv_joy ? global_cls_composite_drv_joy : global_cls_drv_joy;
}

void dainput::set_stick_dead_zone(int stick_idx, bool main_dev, float dzone)
{
  if (HumanInput::IGenJoystickClassDrv *drv = joy_cls())
  {
    drv->setStickDeadZoneScale(stick_idx, main_dev, 1.0f);
    float dz0 = drv->getStickDeadZoneAbs(stick_idx, main_dev);
    drv->setStickDeadZoneScale(stick_idx, main_dev, dz0 > 0 ? dzone / dz0 : 0);
    if (stick_idx >= 0 && stick_idx < 2)
      stickDz[(main_dev ? 0 : 2) + stick_idx] = dzone;
  }
}
float dainput::get_stick_dead_zone(int stick_idx, bool main_dev)
{
  if (stick_idx >= 0 && stick_idx < 2)
    return stickDz[(main_dev ? 0 : 2) + stick_idx];
  return 0;
}
float dainput::get_stick_dead_zone_scale(int stick_idx, bool main_dev)
{
  if (HumanInput::IGenJoystickClassDrv *drv = joy_cls())
    return drv->getStickDeadZoneScale(stick_idx, main_dev);
  return 0;
}
float dainput::get_stick_dead_zone_abs(int stick_idx, bool main_dev)
{
  if (HumanInput::IGenJoystickClassDrv *drv = joy_cls())
    return drv->getStickDeadZoneAbs(stick_idx, main_dev);
  return 0;
}
void dainput::reapply_stick_dead_zones()
{
  for (int s = 0; s < 2; s++)
  {
    if (get_stick_dead_zone_abs(s, true) != get_stick_dead_zone(s, true))
      set_stick_dead_zone(s, true, get_stick_dead_zone(s, true));
    if (get_stick_dead_zone_abs(s, false) != get_stick_dead_zone(s, false))
      set_stick_dead_zone(s, false, get_stick_dead_zone(s, false));
  }
}

void dainput::enable_gyroscope(bool enable, bool main_dev)
{
  if (HumanInput::IGenJoystickClassDrv *drv = joy_cls())
    drv->enableGyroscope(enable, main_dev);
}
