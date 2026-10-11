// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daInput/input_api.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_oaHashNameMap.h>

namespace dainput
{
enum
{
  ACTIONF_internal = 0x1,      //< internal actions should be hidden from controls bingings UI
  ACTIONF_maskImmediate = 0x2, //< when event occurs, UsedControlsMask is updated immediately (for current layer)
  ACTIONF_eventToGui = 0x4,    //< send events for action to GUI scene callback
  ACTIONF_stateful = 0x8,      //< axis action stateful trigger
};

struct DigitalActionData : public DigitalAction
{
  uint16_t nameId, type;
  int16_t iCol;
  uint16_t evReported : 1, stickyToggle : 1, stickyTogglePressed : 1, groupTag : 8, longPressBegan : 1, _resv : 4;
  uint16_t flags, exclTag;
  uint32_t t0msec;
  uint32_t uiHoldToToggleDur;
  void init(int nid, int t)
  {
    bState = false, bActive = false, bEnabled = true, nameId = nid, type = t;
    iCol = -1;
    flags = exclTag = evReported = 0;
    groupTag = 0;
    stickyToggle = stickyTogglePressed = 0;
    longPressBegan = 0;
    _resv = 0;
    t0msec = 0;
    uiHoldToToggleDur = 0;
  }
};

struct AnalogAxisActionData : public AnalogAxisAction
{
  uint16_t nameId, type;
  uint16_t flags, exclTag, groupTag;
  float v;
  void init(int nid, int t)
  {
    x = 0, bActive = false, bEnabled = true, nameId = nid, type = t;
    flags = exclTag = groupTag = 0;
    v = 0;
  }
};

struct AnalogStickActionData : public AnalogStickAction
{
  uint16_t nameId, type;
  uint16_t flags, exclTag, groupTag;
  float vx, vy;
  float sx, sy;
  float mmScaleX, mmScaleY, gaScaleX, gaScaleY;
  float smoothValue;
  void init(int nid, int t)
  {
    x = y = 0, bActive = false, bEnabled = true, nameId = nid, type = t;
    gpadX = gpadY = 0;
    flags = exclTag = groupTag = 0;
    vx = vy = 0;
    sx = sy = 0;
    mmScaleX = mmScaleY = gaScaleX = gaScaleY = 1.0;
    smoothValue = 0.f;
  }
};

struct ActionGlobData
{
  Tab<DigitalActionData> ad;
  Tab<DigitalActionBinding> adb;

  Tab<AnalogAxisActionData> aa;
  Tab<AnalogAxisActionBinding> aab;

  Tab<AnalogStickActionData> as;
  Tab<AnalogStickActionBinding> asb;

  int bindingsColumnCount = 0; // adb.size() == ad.size()*bindingsColumnCount, etc.
  int longPressDur = 300;
  int dblClickDur = 220;

  // 'useBinding' index map, one per type group and indexed as ad/aa/as
  carray<Tab<uint16_t>, 3> useBindingOf;

  int get_binding_idx(action_handle_t a, int column) { return (a & ~TYPEGRP__MASK) * bindingsColumnCount + column; }
};

struct ActionSet
{
  SmallTab<action_handle_t, MidmemAlloc> actions;
  SmallTab<action_set_handle_t, MidmemAlloc> exclusiveWith;
  int ordPriority = 0;
  int pendingCnt = 0;
};

extern ActionGlobData agData;
extern Tab<ActionSet> actionSets;
extern unsigned actionSetsGen;
extern Tab<action_handle_t> actionsNativeOrdered;
extern FastStrMap actionNameIdx;
extern Tab<const char *> actionNameBackMap[3];
extern FastNameMapEx actionSetNameIdx;
extern FastNameMap tagNames;
extern int exclusiveSetLinks; // exclusive_with links of the config, so a build with none pays nothing per tick
extern DataBlock customPropsScheme;
extern unsigned colActiveMask;
extern unsigned devReportMask;

extern Tab<action_set_handle_t> actionSetStack;
void reset_exclusive_sets_reports();  // a new config starts with nothing reported
void check_exclusive_sets_on_stack(); // the dev build judges the stack at the tick
extern action_set_handle_t breaking_set_handle;

extern HumanInput::IGenKeyboard *dev1_kbd;
extern HumanInput::IGenPointing *dev2_pnt;
extern HumanInput::IGenJoystick *dev3_gpad;
extern HumanInput::IGenJoystick *dev4_joy;
extern HumanInput::VrInput *dev5_vr;
extern int configVer;

extern int64_t controlTID;
inline bool on_control_thread() { return controlTID < 0 || is_controled_by_cur_thread(); } // none before init_actions()

extern void (*notify_ui_action_triggered_cb)(action_handle_t action, bool action_terminated, int dur_ms);
extern action_triggered_t notify_action_triggered_cb;
extern action_progress_changed_t notify_progress_changed_cb;
extern bool actionset_logs, event_logs;
extern void (*logscreen)(const char *s);
extern unsigned notify_clicks_dev_mask;
extern void (*notify_clicks_cb)(unsigned dev_mask, unsigned total_clicks);

inline bool is_action_handle_valid(action_handle_t h)
{
  int idx = (h & ~TYPEGRP__MASK);
  switch (h & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: return idx < agData.ad.size();
    case TYPEGRP_AXIS: return idx < agData.aa.size();
    case TYPEGRP_STICK: return idx < agData.as.size();
  }
  return false;
}
inline uint16_t get_action_flags(action_handle_t h)
{
  int idx = (h & ~TYPEGRP__MASK);
  switch (h & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: return agData.ad[idx].flags;
    case TYPEGRP_AXIS: return agData.aa[idx].flags;
    case TYPEGRP_STICK: return agData.as[idx].flags;
  }
  return 0;
}
inline uint16_t get_action_excl_tag(action_handle_t h)
{
  int idx = (h & ~TYPEGRP__MASK);
  switch (h & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: return agData.ad[idx].exclTag;
    case TYPEGRP_AXIS: return agData.aa[idx].exclTag;
    case TYPEGRP_STICK: return agData.as[idx].exclTag;
  }
  return 0;
}
// the index within the type group of <h> of the action whose binding it copies, or BAD_ACTION_HANDLE when it declares none
inline uint16_t get_use_binding_idx(action_handle_t h)
{
  if (!is_action_handle_valid(h)) // a handle of no type group, BAD_ACTION_HANDLE among them, would index past the tables
    return BAD_ACTION_HANDLE;
  const Tab<uint16_t> &at = agData.useBindingOf[h >> 14];
  unsigned idx = h & ~TYPEGRP__MASK;
  return idx < at.size() ? at[idx] : uint16_t(BAD_ACTION_HANDLE);
}
inline bool is_use_binding_action(action_handle_t h) { return get_use_binding_idx(h) != BAD_ACTION_HANDLE; }
// the action whose binding <h> copies: another action when <h> declares 'useBinding', else itself
inline action_handle_t get_use_binding_action(action_handle_t h)
{
  uint16_t idx = get_use_binding_idx(h);
  return idx != BAD_ACTION_HANDLE ? action_handle_t((h & TYPEGRP__MASK) | idx) : h;
}
// true when both fire with one binding: the same action twice, one declaring 'useBinding' of the other, or both of a third one
inline bool uses_same_binding(action_handle_t a, action_handle_t b) { return get_use_binding_action(a) == get_use_binding_action(b); }
inline bool is_action_binding_empty(action_handle_t h, int c)
{
  int b_idx = agData.get_binding_idx(h, c);
  switch (h & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: return agData.adb[b_idx].devId == dainput::DEV_none;
    case TYPEGRP_AXIS: return agData.aab[b_idx].devId == dainput::DEV_none;
    case TYPEGRP_STICK: return agData.asb[b_idx].devId == dainput::DEV_none;
  }
  return false;
}

inline bool is_action_set_handle_valid(action_set_handle_t h) { return h < actionSets.size(); }
inline const char *get_action_name_fast(action_handle_t h) { return actionNameBackMap[h >> 14][h & ~TYPEGRP__MASK]; }

void save_action_binding_digital(DataBlock &blk, const dainput::DigitalActionBinding &b);
void save_action_binding_axis(DataBlock &blk, const dainput::AnalogAxisActionBinding &b);
void save_action_binding_stick(DataBlock &blk, const dainput::AnalogStickActionBinding &b);

void load_action_binding_digital(const DataBlock &blk, dainput::DigitalActionBinding &b);
void load_action_binding_axis(const DataBlock &blk, dainput::AnalogAxisActionBinding &b);
void load_action_binding_stick(const DataBlock &blk, dainput::AnalogStickActionBinding &b);

void reset_input_state();
void activate_action_set_immediate(action_set_handle_t set, bool activate);
void set_initial_usage_mask_immediate(dag::ConstSpan<SingleButtonId> disable_used_buttons, int ord_idx);
} // namespace dainput
