// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "actionData.h"
#include <drv/hid/dag_hiVrInput.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_delayedAction.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_atomic.h>

Tab<dainput::action_set_handle_t> dainput::actionSetStack;
dainput::action_set_handle_t dainput::breaking_set_handle = dainput::BAD_ACTION_SET_HANDLE;
dainput::ActionGlobData dainput::agData;
Tab<dainput::ActionSet> dainput::actionSets;
Tab<dainput::action_handle_t> dainput::actionsNativeOrdered;
FastStrMap dainput::actionNameIdx;
Tab<const char *> dainput::actionNameBackMap[3];
FastNameMapEx dainput::actionSetNameIdx;
FastNameMap dainput::tagNames;
DataBlock dainput::customPropsScheme;
int dainput::configVer = 0;
unsigned dainput::colActiveMask = 0xFFFFFFFFu;
bool dainput::actionset_logs = false, dainput::event_logs = false;
void (*dainput::logscreen)(const char *s) = nullptr;

HumanInput::IGenKeyboard *dainput::dev1_kbd = nullptr;
HumanInput::IGenPointing *dainput::dev2_pnt = nullptr;
HumanInput::IGenJoystick *dainput::dev3_gpad = nullptr;
HumanInput::IGenJoystick *dainput::dev4_joy = nullptr;
HumanInput::VrInput *dainput::dev5_vr = nullptr;

int64_t dainput::controlTID = -1;
bool dainput::is_controled_by_cur_thread() { return dainput::controlTID == get_current_thread_id(); }
void dainput::set_control_thread_id(int64_t tid) { dainput::controlTID = tid; }

void (*dainput::notify_ui_action_triggered_cb)(action_handle_t action, bool action_terminated, int dur_ms) = nullptr;
dainput::action_triggered_t dainput::notify_action_triggered_cb = nullptr;
dainput::action_progress_changed_t dainput::notify_progress_changed_cb = nullptr;
unsigned dainput::notify_clicks_dev_mask = 0;
void (*dainput::notify_clicks_cb)(unsigned dev_mask, unsigned total_clicks) = nullptr;

void dainput::set_event_receiver(action_triggered_t on_action_triggered) { notify_action_triggered_cb = on_action_triggered; }
void dainput::set_digital_event_progress_monitor(action_progress_changed_t cb) { notify_progress_changed_cb = cb; }

void dainput::set_long_press_time(int dur_ms) { agData.longPressDur = dur_ms; }
int dainput::get_long_press_time() { return agData.longPressDur; }
void dainput::set_double_click_time(int dur_ms) { agData.dblClickDur = dur_ms; }
int dainput::get_double_click_time() { return agData.dblClickDur; }

int dainput::get_actions_config_version() { return configVer; }
int dainput::get_actions_count() { return actionsNativeOrdered.size(); }
dainput::action_handle_t dainput::get_action_handle_by_ord(int ord_idx) { return actionsNativeOrdered[ord_idx]; }

int dainput::get_action_sets_count() { return actionSetNameIdx.nameCount(); }
dainput::action_set_handle_t dainput::get_action_set_handle_by_ord(int ord_idx) { return action_set_handle_t(ord_idx); }


dainput::action_handle_t dainput::get_action_handle(const char *action_name, uint16_t required_type_grp)
{
  int act_nid = actionNameIdx.getStrId(action_name);
  if (act_nid < 0)
    return BAD_ACTION_HANDLE;
  if (required_type_grp == 0xFFFF || (act_nid & TYPEGRP__MASK) == required_type_grp)
    return action_handle_t(act_nid);
  logerr("get_action_handle(%s).type_grp=%X != required_type_grp=%X", action_name, (act_nid & TYPEGRP__MASK), required_type_grp);
  return BAD_ACTION_HANDLE;
}
uint16_t dainput::get_action_type(dainput::action_handle_t action)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: return agData.ad[action & ~TYPEGRP__MASK].type;
      case TYPEGRP_AXIS: return agData.aa[action & ~TYPEGRP__MASK].type;
      case TYPEGRP_STICK: return agData.as[action & ~TYPEGRP__MASK].type;
      default: return 0xFFFF;
    }
  return 0xFFFF;
}
const char *dainput::get_action_name(dainput::action_handle_t action)
{
  return action != BAD_ACTION_HANDLE ? get_action_name_fast(action) : NULL;
}
bool dainput::is_action_active(dainput::action_handle_t action)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: return agData.ad[action & ~TYPEGRP__MASK].bActive;
      case TYPEGRP_AXIS: return agData.aa[action & ~TYPEGRP__MASK].bActive;
      case TYPEGRP_STICK: return agData.as[action & ~TYPEGRP__MASK].bActive;
      default: return false;
    }
  return false;
}
bool dainput::is_action_enabled(dainput::action_handle_t action)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: return agData.ad[action & ~TYPEGRP__MASK].bEnabled;
      case TYPEGRP_AXIS: return agData.aa[action & ~TYPEGRP__MASK].bEnabled;
      case TYPEGRP_STICK: return agData.as[action & ~TYPEGRP__MASK].bEnabled;
      default: return false;
    }
  return false;
}
void dainput::set_action_enabled(dainput::action_handle_t action, bool is_enabled)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: agData.ad[action & ~TYPEGRP__MASK].bEnabled = is_enabled; break;
      case TYPEGRP_AXIS: agData.aa[action & ~TYPEGRP__MASK].bEnabled = is_enabled; break;
      case TYPEGRP_STICK: agData.as[action & ~TYPEGRP__MASK].bEnabled = is_enabled; break;
    }
}
void dainput::set_action_mask_immediate(dainput::action_handle_t action, bool is_enabled)
{
  const auto set_flag = [&](uint16_t &flags, bool is_enabled) {
    if (is_enabled)
      flags |= ACTIONF_maskImmediate;
    else
      flags &= ~ACTIONF_maskImmediate;
  };

  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: set_flag(agData.ad[action & ~TYPEGRP__MASK].flags, is_enabled); break;
      case TYPEGRP_AXIS: set_flag(agData.aa[action & ~TYPEGRP__MASK].flags, is_enabled); break;
      case TYPEGRP_STICK: set_flag(agData.as[action & ~TYPEGRP__MASK].flags, is_enabled); break;
    }
}
bool dainput::is_action_mask_immediate(dainput::action_handle_t action)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: return agData.ad[action & ~TYPEGRP__MASK].flags & ACTIONF_maskImmediate; break;
      case TYPEGRP_AXIS: return agData.aa[action & ~TYPEGRP__MASK].flags & ACTIONF_maskImmediate; break;
      case TYPEGRP_STICK: return agData.as[action & ~TYPEGRP__MASK].flags & ACTIONF_maskImmediate; break;
      default: return false;
    }
  return false;
}
bool dainput::is_action_internal(action_handle_t action)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: return agData.ad[action & ~TYPEGRP__MASK].flags & ACTIONF_internal;
      case TYPEGRP_AXIS: return agData.aa[action & ~TYPEGRP__MASK].flags & ACTIONF_internal;
      case TYPEGRP_STICK: return agData.as[action & ~TYPEGRP__MASK].flags & ACTIONF_internal;
      default: return false;
    }
  return false;
}
const char *dainput::get_tag_str(unsigned tag) { return tag ? tagNames.getName(tag - 1) : ""; }
unsigned dainput::get_group_tag_for_action(dainput::action_handle_t action)
{
  if (action != BAD_ACTION_HANDLE)
    switch (action & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: return agData.ad[action & ~TYPEGRP__MASK].groupTag;
      case TYPEGRP_AXIS: return agData.aa[action & ~TYPEGRP__MASK].groupTag;
      case TYPEGRP_STICK: return agData.as[action & ~TYPEGRP__MASK].groupTag;
      default: return 0;
    }
  return 0;
}
const dainput::DigitalAction &dainput::get_digital_action_state(dainput::action_handle_t action)
{
  if ((action & TYPEGRP__MASK) == TYPEGRP_DIGITAL)
    return agData.ad[action & ~TYPEGRP__MASK];
  static DigitalAction stub = {false, false};
  return stub;
}
const dainput::AnalogAxisAction &dainput::get_analog_axis_action_state(dainput::action_handle_t action)
{
  if ((action & TYPEGRP__MASK) == TYPEGRP_AXIS)
    return agData.aa[action & ~TYPEGRP__MASK];
  static AnalogAxisAction stub = {0, false};
  return stub;
}
const dainput::AnalogStickAction &dainput::get_analog_stick_action_state(dainput::action_handle_t action)
{
  if ((action & TYPEGRP__MASK) == TYPEGRP_STICK)
    return agData.as[action & ~TYPEGRP__MASK];
  static AnalogStickAction stub = {0, 0, false};
  return stub;
}

bool dainput::set_analog_axis_action_state(action_handle_t action, float val)
{
  if ((action & TYPEGRP__MASK) == TYPEGRP_AXIS && (agData.aa[action & ~TYPEGRP__MASK].flags & ACTIONF_stateful))
  {
    agData.aa[action & ~TYPEGRP__MASK].x = val;
    agData.aa[action & ~TYPEGRP__MASK].bActive = true;
    return true;
  }
  return false;
}

dainput::action_set_handle_t dainput::get_action_set_handle(const char *action_set_name)
{
  int set_nid = actionSetNameIdx.getNameId(action_set_name);
  if (set_nid < 0)
    return BAD_ACTION_SET_HANDLE;
  return action_set_handle_t(set_nid);
}
const char *dainput::get_action_set_name(dainput::action_set_handle_t set)
{
  return is_action_set_handle_valid(set) ? actionSetNameIdx.getName(set) : NULL;
}
dag::ConstSpan<dainput::action_handle_t> dainput::get_action_set_actions(dainput::action_set_handle_t set)
{
  return is_action_set_handle_valid(set) ? make_span_const(actionSets[set].actions) : dag::ConstSpan<action_handle_t>();
}
dainput::action_set_handle_t dainput::setup_action_set(const char *action_set_name, dag::ConstSpan<dainput::action_handle_t> actions)
{
  for (dainput::action_handle_t a : actions)
    if (!is_action_handle_valid(a))
    {
      logerr("bad action=%X in %s", a, __FUNCTION__);
      return BAD_ACTION_SET_HANDLE;
    }

  int set = actionSetNameIdx.addNameId(action_set_name);
  if (set == actionSets.size())
    actionSets.push_back();
  if (is_action_set_handle_valid(set))
    actionSets[set].actions = actions;
  return action_set_handle_t(set);
}
void dainput::clear_action_set_actions(dainput::action_set_handle_t set)
{
  if (is_action_set_handle_valid(set))
    clear_and_shrink(actionSets[set].actions);
}

int dainput::get_action_set_stack_depth() { return actionSetStack.size(); }
dainput::action_set_handle_t dainput::get_action_set_stack_item(int top_down_index)
{
  if (top_down_index >= 0 && top_down_index < actionSetStack.size())
    return actionSetStack[actionSetStack.size() - 1 - top_down_index];
  return BAD_ACTION_SET_HANDLE;
}
dainput::action_set_handle_t dainput::get_current_action_set()
{
  return actionSetStack.size() ? actionSetStack.back() : BAD_ACTION_SET_HANDLE;
}
dainput::action_set_handle_t dainput::set_breaking_action_set(dainput::action_set_handle_t set)
{
  action_set_handle_t h = breaking_set_handle;
  breaking_set_handle = set;
  return h;
}


void dainput::reset_action_set_stack() { actionSetStack.clear(); }
void dainput::activate_action_set(action_set_handle_t set, bool activate)
{
  if (set < actionSets.size() &&
      ((!activate && find_value_idx(actionSetStack, set) >= 0) || (activate && interlocked_acquire_load(actionSets[set].pendingCnt))))
  {
    // delay action set deactivation (and re-activation of pending set) to next frame
    interlocked_increment(actionSets[set].pendingCnt);
    add_delayed_action_buffered(make_delayed_action([set, activate]() {
      activate_action_set_immediate(set, activate);
      if (set < actionSets.size())
        interlocked_decrement(actionSets[set].pendingCnt);
    }));
  }
  else
    activate_action_set_immediate(set, activate);
}
void dainput::activate_action_set_immediate(action_set_handle_t set, bool activate)
{
  if (activate && actionSetStack.size() && actionSetStack.back() == set)
    return;
  if (set >= actionSets.size())
    return;

  int idx = find_value_idx(actionSetStack, set);
  if (idx >= 0)
    erase_items(actionSetStack, idx, 1);
  if (activate)
  {
    if (!actionSetStack.size())
      actionSetStack.push_back(set);
    else
      for (int i = actionSetStack.size() - 1; i >= -1; i--)
        if (i == -1 || actionSets[actionSetStack[i]].ordPriority >= actionSets[set].ordPriority)
        {
          insert_item_at(actionSetStack, i + 1, set);
          break;
        }
  }
  if (dainput::actionset_logs && dainput::logscreen)
    dainput::logscreen(String(0, "dainput::activate_action_set(%s, %s), stack_len=%d (@%d, prio=%d)", get_action_set_name(set),
      activate ? "TRUE" : "false", actionSetStack.size(), find_value_idx(actionSetStack, set), actionSets[set].ordPriority));
}


void dainput::reset_devices()
{
  dainput::dev1_kbd = nullptr;
  dainput::dev2_pnt = nullptr;
  dainput::dev3_gpad = nullptr;
  dainput::dev4_joy = nullptr;
  dainput::reset_input_state();
}
void dainput::register_dev1_kbd(HumanInput::IGenKeyboard *kbd) { dainput::dev1_kbd = kbd; }
void dainput::register_dev2_pnt(HumanInput::IGenPointing *pnt) { dainput::dev2_pnt = pnt; }
void dainput::register_dev3_gpad(HumanInput::IGenJoystick *gamepad) { dainput::dev3_gpad = gamepad; }
void dainput::register_dev4_joy(HumanInput::IGenJoystick *joy) { dainput::dev4_joy = joy; }
void dainput::register_dev5_vr(HumanInput::VrInput *vr) { dainput::dev5_vr = vr; }

void dainput::set_actions_binding_column_active(unsigned column_idx, bool active)
{
  G_ASSERTF(column_idx < 32, "col=%u", column_idx);
  if (active)
    dainput::colActiveMask |= 1u << column_idx;
  else
    dainput::colActiveMask &= ~(1u << column_idx);
}
bool dainput::get_actions_binding_column_active(unsigned column_idx)
{
  G_ASSERTF(column_idx < 32, "col=%u", column_idx);
  return (dainput::colActiveMask & (1u << column_idx)) ? true : false;
}
