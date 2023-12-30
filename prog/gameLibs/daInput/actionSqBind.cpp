#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>
#include "actionData.h"
#include <daInput/config_api.h>
#include <ioSys/dag_dataBlock.h>

#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiJoystick.h>


static const char *format_ctrl_name(String &out_name, int dev, int btn_or_axis, bool is_btn)
{
  using namespace dainput;

  switch (dev)
  {
    case DEV_kbd:
      if (dev1_kbd && is_btn)
        out_name = dev1_kbd->getKeyName(btn_or_axis);
      else if (is_btn)
        out_name.printf(0, "K:%03d", btn_or_axis);
      else
        out_name.printf(0, "K:A%d", btn_or_axis);
      break;
    case DEV_pointing:
      if (dev2_pnt && is_btn)
        out_name.printf(0, "%s", dev2_pnt->getBtnName(btn_or_axis));
      else if (is_btn)
        out_name.printf(0, "M:%d", btn_or_axis);
      else
        out_name.printf(0, "M:%c", btn_or_axis == 0 ? 'X' : 'Y');
      break;
    case DEV_gamepad:
      if (dev3_gpad)
        out_name.printf(0, "J:%s", is_btn ? dev3_gpad->getButtonName(btn_or_axis) : dev3_gpad->getAxisName(btn_or_axis));
      else
        out_name.printf(0, "J:%s%d", is_btn ? "B" : "A", btn_or_axis);
      break;
    case DEV_joy:
      if (dev4_joy)
        out_name.printf(0, "J:%s", is_btn ? dev4_joy->getButtonName(btn_or_axis) : dev4_joy->getAxisName(btn_or_axis));
      else
        out_name.printf(0, "J:%s%d", is_btn ? "B" : "A", btn_or_axis);
      break;

    case DEV_none:
    case DEV_nullstub: clear_and_shrink(out_name); break;
    default:
      G_ASSERTF(0, "Unexpected device id %D", dev);
      clear_and_shrink(out_name);
      break;
  }

  return out_name;
}


static const char *format_btn_name(String &out_name, dainput::SingleButtonId id)
{
  return format_ctrl_name(out_name, id.devId, id.btnId, true);
}


static void build_modifiers_str(String &bindStr, int modCnt, const dainput::SingleButtonId *mod)
{
  clear_and_shrink(bindStr);
  if (modCnt)
  {
    String tmpStr;
    for (int j = 0; j < modCnt; j++)
      bindStr.aprintf(0, "%s+", format_btn_name(tmpStr, mod[j]));
  }
}


static bool is_valid_device(int dev)
{
  using namespace dainput;
  return dev == DEV_kbd || dev == DEV_pointing || dev == DEV_gamepad || dev == DEV_joy;
}

static bool is_action_binding_set(dainput::action_handle_t a, int col)
{
  using namespace dainput;
  int col_e;
  if (col < 0)
    col = 0, col_e = get_actions_binding_columns();
  else
    col_e = col + 1;

  for (; col < col_e; col++)
    switch (get_action_type(a) & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL:
        if (DigitalActionBinding *b = get_digital_action_binding(a, col))
          if (b->devId != DEV_none)
            return true;
        break;

      case TYPEGRP_AXIS:
        if (AnalogAxisActionBinding *b = get_analog_axis_action_binding(a, col))
          if (b->devId != DEV_none)
            return true;
        break;

      case TYPEGRP_STICK:
        if (AnalogStickActionBinding *b = get_analog_stick_action_binding(a, col))
          if (b->devId != DEV_none)
            return true;
        break;
    }
  return false;
}

static void set_main_gamepad_stick_dead_zone(int stick_idx, float dzone) { dainput::set_stick_dead_zone(stick_idx, true, dzone); }
static void set_joystick_stick_dead_zone(int stick_idx, float dzone) { dainput::set_stick_dead_zone(stick_idx, false, dzone); }
static float get_main_gamepad_stick_dead_zone(int stick_idx) { return dainput::get_stick_dead_zone(stick_idx, true); }
static float get_joystick_stick_dead_zone(int stick_idx) { return dainput::get_stick_dead_zone(stick_idx, false); }
static float get_main_gamepad_stick_dead_zone_abs(int stick_idx) { return dainput::get_stick_dead_zone_abs(stick_idx, true); }
static float get_joystick_stick_dead_zone_abs(int stick_idx) { return dainput::get_stick_dead_zone_abs(stick_idx, false); }
static void enable_joystick_gyroscope(bool enable) { dainput::enable_gyroscope(enable, true); }

static SQInteger sq_format_ctrl_name(HSQUIRRELVM vm)
{
  SQInteger devId, btnOrAxisId;
  SQBool isBtn = true;
  sq_getinteger(vm, 2, &devId);
  sq_getinteger(vm, 3, &btnOrAxisId);
  if (sq_gettop(vm) > 3)
    sq_getbool(vm, 4, &isBtn);

  String buf;
  format_ctrl_name(buf, devId, btnOrAxisId, isBtn);
  sq_pushstring(vm, buf.c_str(), buf.length());
  return 1;
}

#if DAGOR_DBGLEVEL > 0
static const char *sq_dump_action_sets()
{
  using namespace dainput;
  static String s;
  s.printf(0, "action sets: %d total:\n", actionSets.size());
  for (int i = 0; i < actionSets.size(); i++)
    s.aprintf(0, "  [%4X] %s (prio=%d)\n", i, actionSetNameIdx.getName(i), actionSets[i].ordPriority);
  return s.str();
}
static const char *sq_dump_action_sets_stack()
{
  using namespace dainput;
  static String s;
  s.printf(0, "action set stack (%d of %d, top down):\n", actionSetStack.size(), actionSets.size());
  for (int i = actionSetStack.size() - 1; i >= 0; i--)
    s.aprintf(0, "  [%4X] %s (prio=%d)\n", actionSetStack[i], actionSetNameIdx.getName(actionSetStack[i]),
      actionSets[actionSetStack[i]].ordPriority);
  return s.str();
}
static const char *sq_dump_action_set(const char *nm)
{
  using namespace dainput;
  static String s;
  action_handle_t h = get_action_set_handle(nm);
  s.printf(0, "action set %s handle=%d prio=%d\n", nm, h, actionSets[h].ordPriority);
  if (h != dainput::BAD_ACTION_SET_HANDLE)
    for (action_handle_t a : dainput::get_action_set_actions(h))
      s.aprintf(0, "  %s { type=%X name=%X grp=\"%s\" }\n", get_action_name_fast(a), get_action_type(a), a,
        get_group_tag_str_for_action(a));
  return s.str();
}
#endif

static SQInteger sq_get_action_bindings_text(HSQUIRRELVM vm)
{
  using namespace dainput;

  const char *actionName = nullptr;
  sq_getstring(vm, 2, &actionName);

  action_handle_t a = get_action_handle(actionName);
  if (a == BAD_ACTION_HANDLE)
    return sq_throwerror(vm, String(32, "Invalid action name %s", actionName));

  int nCols = get_actions_binding_columns();

  sq_newarray(vm, nCols);

  String bindStr, tmpStr;

  for (int iCol = 0; iCol < nCols; ++iCol)
  {
    clear_and_shrink(bindStr);
    clear_and_shrink(tmpStr);
    switch (get_action_type(a) & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL:
        if (DigitalActionBinding *b = get_digital_action_binding(a, iCol))
        {
          build_modifiers_str(bindStr, b->modCnt, b->mod);
          bindStr.aprintf(0, "%s", format_ctrl_name(tmpStr, b->devId, b->ctrlId, b->btnCtrl));
        }
        break;

      case TYPEGRP_AXIS:
        if (AnalogAxisActionBinding *b = get_analog_axis_action_binding(a, iCol))
        {
          bool stateful = agData.aa[a & ~TYPEGRP__MASK].flags & ACTIONF_stateful;
          build_modifiers_str(bindStr, b->modCnt, b->mod);
          if (is_valid_device(b->devId))
            bindStr.aprintf(0, "[%s%s%s%s%s] ", format_ctrl_name(tmpStr, b->devId, b->axisId, false), b->invAxis ? "(inverse)" : "",
              b->axisRelativeVal ? "(rel)" : "", b->instantIncDecBtn ? "(inst)" : "", b->quantizeValOnIncDecBtn ? "(quant)" : "");
          if (get_action_type(a) == TYPE_STEERWHEEL || stateful)
          {
            if (is_valid_device(b->minBtn.devId) || is_valid_device(b->maxBtn.devId))
            {
              bindStr.aprintf(0, "B-:%s", format_btn_name(tmpStr, b->minBtn));
              bindStr.aprintf(0, " B+:%s ", format_btn_name(tmpStr, b->maxBtn));
            }
            if (stateful && (is_valid_device(b->incBtn.devId) || is_valid_device(b->decBtn.devId)))
            {
              bindStr.aprintf(0, " B++:%s", build_btn_name(tmpStr, b->incBtn));
              bindStr.aprintf(0, " B--:%s", build_btn_name(tmpStr, b->decBtn));
            }
          }
          else
          {
            if (is_valid_device(b->maxBtn.devId))
              bindStr.aprintf(0, "%s", format_btn_name(tmpStr, b->maxBtn));
          }
        }
        break;

      case TYPEGRP_STICK:
        if (AnalogStickActionBinding *b = get_analog_stick_action_binding(a, iCol))
        {
          build_modifiers_str(bindStr, b->modCnt, b->mod);
          if (is_valid_device(b->devId))
          {
            bindStr.aprintf(0, "[%s%s/", format_ctrl_name(tmpStr, b->devId, b->axisXId, false), b->axisXinv ? "(inverse)" : "");
            bindStr.aprintf(0, "%s%s] ", format_ctrl_name(tmpStr, b->devId, b->axisYId, false), b->axisYinv ? "(inverse)" : "");
          }
          if (b->minXBtn.devId)
            bindStr.aprintf(0, "X-:%s ", format_btn_name(tmpStr, b->minXBtn));
          if (b->maxXBtn.devId)
            bindStr.aprintf(0, "X+:%s ", format_btn_name(tmpStr, b->maxXBtn));
          if (b->minYBtn.devId)
            bindStr.aprintf(0, "Y-:%s ", format_btn_name(tmpStr, b->minYBtn));
          if (b->maxYBtn.devId)
            bindStr.aprintf(0, "Y+:%s ", format_btn_name(tmpStr, b->maxYBtn));
        }
        break;
    }

    if (!bindStr.empty())
    {
      sq_pushinteger(vm, iCol);
      sq_pushstring(vm, bindStr, bindStr.length());
      G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
    }
  }

  return 1;
}


static SQInteger sq_get_action_set_actions(HSQUIRRELVM vm)
{
  Sqrat::Var<SQInteger> varHandle(vm, 2);
  dag::ConstSpan<dainput::action_handle_t> actions = dainput::get_action_set_actions(varHandle.value);
  Sqrat::Array arr(vm, actions.size());

  for (int i = 0; i < actions.size(); i++)
    arr.SetValue<SQInteger>(i, actions[i]);

  sq_pushobject(vm, arr);
  return 1;
}


static SQInteger sq_setup_action_set(HSQUIRRELVM vm)
{
  Sqrat::Var<const char *> varName(vm, 2);
  Sqrat::Var<Sqrat::Array> varActions(vm, 3);
  Tab<dainput::action_handle_t> tabActions;

  tabActions.resize(varActions.value.Length());
  for (int i = 0; i < tabActions.size(); i++)
    tabActions[i] = varActions.value.GetValue<SQInteger>(i);

  sq_pushinteger(vm, dainput::setup_action_set(varName.value, tabActions));
  return 1;
}


static SQInteger sq_check_bindings_conflicts(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SQInteger, DataBlock *>(vm, 2))
    return SQ_ERROR;

  Sqrat::Var<SQInteger> varHandle(vm, 2);
  Sqrat::Var<const DataBlock &> varBinding(vm, 3);
  Tab<dainput::action_handle_t> out_a;
  Tab<int> out_c;

  bool res = dainput::check_bindings_conflicts(varHandle.value, varBinding.value, out_a, out_c);
  if (!res)
    return 0;

  Sqrat::Array arr(vm, out_a.size());
  for (int i = 0; i < out_a.size(); i++)
  {
    Sqrat::Table t(vm);
    t.SetValue<SQInteger>("action", out_a[i]);
    t.SetValue<SQInteger>("column", out_c[i]);
    arr.SetValue(i, t);
  }
  sq_pushobject(vm, arr);
  return 1;
}


static SQInteger sq_check_bindings_hides_action(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SQInteger, DataBlock *>(vm, 2))
    return SQ_ERROR;

  Sqrat::Var<SQInteger> varHandle(vm, 2);
  Sqrat::Var<const DataBlock &> varBinding(vm, 3);
  Tab<dainput::action_handle_t> out_a;
  Tab<int> out_c;

  bool res = dainput::check_bindings_hides_action(varHandle.value, varBinding.value, out_a, out_c);
  if (!res)
    return 0;

  Sqrat::Array arr(vm, out_a.size());
  for (int i = 0; i < out_a.size(); i++)
  {
    Sqrat::Table t(vm);
    t.SetValue<SQInteger>("action", out_a[i]);
    t.SetValue<SQInteger>("column", out_c[i]);
    arr.SetValue(i, t);
  }
  sq_pushobject(vm, arr);
  return 1;
}

static void sq_load_actions_binding(const DataBlock &blk, int column) { return dainput::load_actions_binding(blk, column); }
static bool sq_is_action_stateful(dainput::action_handle_t action)
{
  using namespace dainput;
  if ((action & TYPEGRP__MASK) == TYPEGRP_AXIS)
    return agData.aa[action & ~TYPEGRP__MASK].flags & ACTIONF_stateful;
  return false;
}

static Sqrat::Function digital_event_progress_monitor_sq;
static void on_digital_event_progress_changed_sq(dag::ConstSpan<dainput::DigitalActionProgress> actions)
{
  if (digital_event_progress_monitor_sq.IsNull())
    return;
  HSQUIRRELVM vm = digital_event_progress_monitor_sq.GetVM();
  Sqrat::Array arr(vm, actions.size());
  for (int i = 0; i < actions.size(); i++)
  {
    Sqrat::Table t(vm);
    t.SetValue<SQInteger>("action", actions[i].action);
    t.SetValue<SQInteger>("state", actions[i].state);
    t.SetValue<SQInteger>("evType", actions[i].evType);
    t.SetValue<SQInteger>("startT0msec", actions[i].startT0msec);
    arr.SetValue(i, t);
  }
  digital_event_progress_monitor_sq(arr);
}
static void sq_install_digital_event_progress_monitor(Sqrat::Function func)
{
  digital_event_progress_monitor_sq = func;
  dainput::set_digital_event_progress_monitor(func.IsNull() ? nullptr : &on_digital_event_progress_changed_sq);
}

#define SQ_IMPLEMENT_PROP_TYPED(STRUCT, MEMBER, MTYPE, TYPESUFFIX) \
  static SQInteger STRUCT##_get_##MEMBER(HSQUIRRELVM vm)           \
  {                                                                \
    if (!Sqrat::check_signature<dainput::STRUCT *>(vm, 1))         \
      return SQ_ERROR;                                             \
    Sqrat::Var<dainput::STRUCT *> a(vm, 1);                        \
    sq_push##TYPESUFFIX(vm, a.value->MEMBER);                      \
    return 1;                                                      \
  }                                                                \
  static SQInteger STRUCT##_set_##MEMBER(HSQUIRRELVM vm)           \
  {                                                                \
    if (!Sqrat::check_signature<dainput::STRUCT *>(vm, 1))         \
      return SQ_ERROR;                                             \
    Sqrat::Var<dainput::STRUCT *> a(vm, 1);                        \
    MTYPE val;                                                     \
    sq_get##TYPESUFFIX(vm, 2, &val);                               \
    a.value->MEMBER = val;                                         \
    return 0;                                                      \
  }

#define SQ_IMPLEMENT_PROP_SB_ARR(STRUCT, MEMBER)                          \
  static SQInteger STRUCT##_get_##MEMBER(HSQUIRRELVM vm)                  \
  {                                                                       \
    if (!Sqrat::check_signature<dainput::STRUCT *>(vm, 1))                \
      return SQ_ERROR;                                                    \
    Sqrat::Var<dainput::STRUCT *> a(vm, 1);                               \
    if (!a.value->MEMBER##Cnt)                                            \
      return 0;                                                           \
    Sqrat::Array arr(vm, a.value->MEMBER##Cnt);                           \
    for (int i = 0; i < a.value->MEMBER##Cnt; i++)                        \
      arr.SetValue<dainput::SingleButtonId>(i, a.value->MEMBER[i]);       \
    sq_pushobject(vm, arr);                                               \
    return 1;                                                             \
  }                                                                       \
  static SQInteger STRUCT##_set_##MEMBER(HSQUIRRELVM vm)                  \
  {                                                                       \
    if (!Sqrat::check_signature<dainput::STRUCT *>(vm, 1))                \
      return SQ_ERROR;                                                    \
    Sqrat::Var<dainput::STRUCT *> a(vm, 1);                               \
    Sqrat::Var<Sqrat::Array> sb(vm, 2);                                   \
    G_ASSERT(sb.value.Length() <= 3);                                     \
    a.value->MEMBER##Cnt = sb.value.Length();                             \
    for (int i = 0; i < sb.value.Length(); i++)                           \
      a.value->MEMBER[i] = sb.value.GetValue<dainput::SingleButtonId>(i); \
    return 0;                                                             \
  }

#define SQ_IMPLEMENT_PROP_SB(STRUCT, MEMBER)                                             \
  static SQInteger STRUCT##_get_##MEMBER(HSQUIRRELVM vm)                                 \
  {                                                                                      \
    if (!Sqrat::check_signature<dainput::STRUCT *>(vm, 1))                               \
      return SQ_ERROR;                                                                   \
    Sqrat::Var<dainput::STRUCT *> a(vm, 1);                                              \
    Sqrat::ClassType<dainput::SingleButtonId>::PushNativeInstance(vm, &a.value->MEMBER); \
    return 1;                                                                            \
  }                                                                                      \
  static SQInteger STRUCT##_set_##MEMBER(HSQUIRRELVM vm)                                 \
  {                                                                                      \
    if (!Sqrat::check_signature<dainput::STRUCT *, dainput::SingleButtonId *>(vm, 1))    \
      return SQ_ERROR;                                                                   \
    Sqrat::Var<dainput::STRUCT *> a(vm, 1);                                              \
    Sqrat::Var<dainput::SingleButtonId *> sb(vm, 2);                                     \
    a.value->MEMBER = *sb.value;                                                         \
    return 0;                                                                            \
  }

#define SQ_IMPLEMENT_PROP_INT(STRUCT, MEMBER)   SQ_IMPLEMENT_PROP_TYPED(STRUCT, MEMBER, SQInteger, integer)
#define SQ_IMPLEMENT_PROP_BOOL(STRUCT, MEMBER)  SQ_IMPLEMENT_PROP_TYPED(STRUCT, MEMBER, SQBool, bool)
#define SQ_IMPLEMENT_PROP_FLOAT(STRUCT, MEMBER) SQ_IMPLEMENT_PROP_TYPED(STRUCT, MEMBER, SQFloat, float)

#define SQ_BIND_PROP(STRUCT, MEMBER) .SquirrelProp(#MEMBER, STRUCT##_get_##MEMBER, STRUCT##_set_##MEMBER)

#define SQ_BIND_MEMBER_VAR(STRUCT, MEMBER) .Var(#MEMBER, &STRUCT::MEMBER)

/// @module dainput2
///@class SingleButtonId
/// @property devId integer
SQ_IMPLEMENT_PROP_INT(SingleButtonId, devId);
/// @property btnId integer
SQ_IMPLEMENT_PROP_INT(SingleButtonId, btnId);

///@class DigitalActionBinding
/// @property devId integer
SQ_IMPLEMENT_PROP_INT(DigitalActionBinding, devId)
/// @property ctrlId integer
SQ_IMPLEMENT_PROP_INT(DigitalActionBinding, ctrlId)
/// @property eventType integer
SQ_IMPLEMENT_PROP_INT(DigitalActionBinding, eventType)
/// @property axisCtrlThres integer
SQ_IMPLEMENT_PROP_INT(DigitalActionBinding, axisCtrlThres)
/// @property btnCtrl bool
SQ_IMPLEMENT_PROP_BOOL(DigitalActionBinding, btnCtrl)
/// @property stickyToggle bool
SQ_IMPLEMENT_PROP_BOOL(DigitalActionBinding, stickyToggle)
/// @property unordCombo bool
SQ_IMPLEMENT_PROP_BOOL(DigitalActionBinding, unordCombo)
/// @property modCnt integer
SQ_IMPLEMENT_PROP_INT(DigitalActionBinding, modCnt)
/// @property mod array
SQ_IMPLEMENT_PROP_SB_ARR(DigitalActionBinding, mod)

///@class AnalogAxisActionBinding
/// @property devId integer
SQ_IMPLEMENT_PROP_INT(AnalogAxisActionBinding, devId)
/// @property axisId integer
SQ_IMPLEMENT_PROP_INT(AnalogAxisActionBinding, axisId)
/// @property invAxis bool
SQ_IMPLEMENT_PROP_BOOL(AnalogAxisActionBinding, invAxis)
/// @property axisRelativeVal bool
SQ_IMPLEMENT_PROP_BOOL(AnalogAxisActionBinding, axisRelativeVal)
/// @property instantIncDecBtn bool
SQ_IMPLEMENT_PROP_BOOL(AnalogAxisActionBinding, instantIncDecBtn)
/// @property quantizeValOnIncDecBtn bool
SQ_IMPLEMENT_PROP_BOOL(AnalogAxisActionBinding, quantizeValOnIncDecBtn)
/// @property modCnt integer
SQ_IMPLEMENT_PROP_INT(AnalogAxisActionBinding, modCnt)
/// @property mod array
SQ_IMPLEMENT_PROP_SB_ARR(AnalogAxisActionBinding, mod)
/// @property minBtn
SQ_IMPLEMENT_PROP_SB(AnalogAxisActionBinding, minBtn)
/// @property maxBtn
SQ_IMPLEMENT_PROP_SB(AnalogAxisActionBinding, maxBtn)
/// @property incBtn
SQ_IMPLEMENT_PROP_SB(AnalogAxisActionBinding, incBtn)
/// @property decBtn
SQ_IMPLEMENT_PROP_SB(AnalogAxisActionBinding, decBtn)
/// @property deadZoneThres float
SQ_IMPLEMENT_PROP_FLOAT(AnalogAxisActionBinding, deadZoneThres)
/// @property nonLin float
SQ_IMPLEMENT_PROP_FLOAT(AnalogAxisActionBinding, nonLin)
/// @property maxVal float
SQ_IMPLEMENT_PROP_FLOAT(AnalogAxisActionBinding, maxVal)
/// @property relIncScale float
SQ_IMPLEMENT_PROP_FLOAT(AnalogAxisActionBinding, relIncScale)

///@class AnalogStickActionBinding
/// @property devId integer
SQ_IMPLEMENT_PROP_INT(AnalogStickActionBinding, devId)
/// @property axisXId integer
SQ_IMPLEMENT_PROP_INT(AnalogStickActionBinding, axisXId)
/// @property axisYId integer
SQ_IMPLEMENT_PROP_INT(AnalogStickActionBinding, axisYId)
/// @property axisXinv bool
SQ_IMPLEMENT_PROP_BOOL(AnalogStickActionBinding, axisXinv)
/// @property axisYinv bool
SQ_IMPLEMENT_PROP_BOOL(AnalogStickActionBinding, axisYinv)
/// @property modCnt integer
SQ_IMPLEMENT_PROP_INT(AnalogStickActionBinding, modCnt)
/// @property mod array
SQ_IMPLEMENT_PROP_SB_ARR(AnalogStickActionBinding, mod)
/// @property minXBtn
SQ_IMPLEMENT_PROP_SB(AnalogStickActionBinding, minXBtn)
/// @property maxXBtn
SQ_IMPLEMENT_PROP_SB(AnalogStickActionBinding, maxXBtn)
/// @property minYBtn
SQ_IMPLEMENT_PROP_SB(AnalogStickActionBinding, minYBtn)
/// @property maxYBtn
SQ_IMPLEMENT_PROP_SB(AnalogStickActionBinding, maxYBtn)
/// @property deadZoneThres float
SQ_IMPLEMENT_PROP_FLOAT(AnalogStickActionBinding, deadZoneThres)
/// @property axisSnapAngK float
SQ_IMPLEMENT_PROP_FLOAT(AnalogStickActionBinding, axisSnapAngK)
/// @property nonLin float
SQ_IMPLEMENT_PROP_FLOAT(AnalogStickActionBinding, nonLin)
/// @property maxVal float
SQ_IMPLEMENT_PROP_FLOAT(AnalogStickActionBinding, maxVal)
/// @property sensScale float
SQ_IMPLEMENT_PROP_FLOAT(AnalogStickActionBinding, sensScale)


void dainput::bind_sq_api(SqModules *moduleMgr)
{
  HSQUIRRELVM vm = moduleMgr->getVM();

  Sqrat::Table dainp(vm);
#define DEF_CONSTANT_EX(name, nsp) dainp.SetValue(#name, nsp::name)
#define DEF_CONSTANT(name)         DEF_CONSTANT_EX(name, dainput)

  // clang-format off
  /// @const DEV_none
  DEF_CONSTANT(DEV_none);
  /// @const DEV_kbd
  DEF_CONSTANT(DEV_kbd);
  /// @const DEV_pointing
  DEF_CONSTANT(DEV_pointing);
  /// @const DEV_gamepad
  DEF_CONSTANT(DEV_gamepad);
  /// @const DEV_joy
  DEF_CONSTANT(DEV_joy);
  /// @const DEV_nullstub
  DEF_CONSTANT(DEV_nullstub);

  /// @const DEV_USED_mouse
  DEF_CONSTANT(DEV_USED_mouse);
  /// @const DEV_USED_kbd
  DEF_CONSTANT(DEV_USED_kbd);
  /// @const DEV_USED_gamepad
  DEF_CONSTANT(DEV_USED_gamepad);
  /// @const DEV_USED_touch
  DEF_CONSTANT(DEV_USED_touch);

  /// @const TYPEGRP__MASK
  DEF_CONSTANT(TYPEGRP__MASK);
  /// @const TYPEGRP_DIGITAL
  DEF_CONSTANT(TYPEGRP_DIGITAL);
  /// @const TYPE_BUTTON
  DEF_CONSTANT(TYPE_BUTTON);
  /// @const TYPEGRP_AXIS
  DEF_CONSTANT(TYPEGRP_AXIS);
  /// @const TYPE_TRIGGER
  DEF_CONSTANT(TYPE_TRIGGER);
  /// @const TYPE_STEERWHEEL
  DEF_CONSTANT(TYPE_STEERWHEEL);
  /// @const TYPEGRP_STICK
  DEF_CONSTANT(TYPEGRP_STICK);
  /// @const TYPE_SYSMOUSE
  DEF_CONSTANT(TYPE_SYSMOUSE);
  /// @const TYPE_ABSMOUSE
  DEF_CONSTANT(TYPE_ABSMOUSE);
  /// @const TYPE_STICK
  DEF_CONSTANT(TYPE_STICK);
  /// @const TYPE_STICK_DELTA
  DEF_CONSTANT(TYPE_STICK_DELTA);

  /// @const BAD_ACTION_HANDLE
  DEF_CONSTANT(BAD_ACTION_HANDLE);
  /// @const BAD_ACTION_SET_HANDLE
  DEF_CONSTANT(BAD_ACTION_SET_HANDLE);

  /// @const AXIS_NULL_ID
  DEF_CONSTANT(AXIS_NULL_ID);
  /// @const BTN_NULL_ID
  DEF_CONSTANT(BTN_NULL_ID);

  /// @const BTN_pressed
  DEF_CONSTANT_EX(BTN_pressed, DigitalActionBinding);
  /// @const BTN_pressed_long
  DEF_CONSTANT_EX(BTN_pressed_long, DigitalActionBinding);
  /// @const BTN_pressed2
  DEF_CONSTANT_EX(BTN_pressed2, DigitalActionBinding);
  /// @const BTN_pressed3
  DEF_CONSTANT_EX(BTN_pressed3, DigitalActionBinding);
  /// @const BTN_released
  DEF_CONSTANT_EX(BTN_released, DigitalActionBinding);
  /// @const BTN_released_short
  DEF_CONSTANT_EX(BTN_released_short, DigitalActionBinding);
  /// @const BTN_released_long
  DEF_CONSTANT_EX(BTN_released_long, DigitalActionBinding);

  /// @const ST_in_progress
  DEF_CONSTANT_EX(ST_in_progress, DigitalActionProgress);
  /// @const ST_finished
  DEF_CONSTANT_EX(ST_finished, DigitalActionProgress);
  /// @const ST_cancelled
  DEF_CONSTANT_EX(ST_cancelled, DigitalActionProgress);

  /// @const GAMEPAD_VENDOR_UNKNOWN
  DEF_CONSTANT_EX(GAMEPAD_VENDOR_UNKNOWN, HumanInput);
  /// @const GAMEPAD_VENDOR_MICROSOFT
  DEF_CONSTANT_EX(GAMEPAD_VENDOR_MICROSOFT, HumanInput);
  /// @const GAMEPAD_VENDOR_SONY
  DEF_CONSTANT_EX(GAMEPAD_VENDOR_SONY, HumanInput);
  /// @const GAMEPAD_VENDOR_NINTENDO
  DEF_CONSTANT_EX(GAMEPAD_VENDOR_NINTENDO, HumanInput);

  dainp
    .Func("set_long_press_time", set_long_press_time)
    .Func("get_long_press_time", get_long_press_time)
    .Func("set_double_click_time", set_double_click_time)
    .Func("get_double_click_time", get_double_click_time)

    .Func("get_actions_config_version", get_actions_config_version)
    .Func("reset_actions_binding", reset_actions_binding)
    .Func("append_actions_binding", append_actions_binding)
    .Func("clear_actions_binding", clear_actions_binding)
    .Func("load_actions_binding", sq_load_actions_binding)
    .Func("save_actions_binding", save_actions_binding)
    .Func("get_actions_binding_columns", get_actions_binding_columns)
    .Func("get_tag_str", get_tag_str)
    .Func("set_actions_binding_column_active", set_actions_binding_column_active)
    .Func("get_actions_binding_column_active", get_actions_binding_column_active)

    .Func("get_digital_action_state", get_digital_action_state)
    .Func("get_analog_axis_action_state", get_analog_axis_action_state)
    .Func("get_analog_stick_action_state", get_analog_stick_action_state)

    .Func("set_analog_axis_action_state", set_analog_axis_action_state)

    .Func("get_digital_action_binding", get_digital_action_binding)
    .Func("get_analog_axis_action_binding", get_analog_axis_action_binding)
    .Func("get_analog_stick_action_binding", get_analog_stick_action_binding)
    .Func("is_action_binding_set", is_action_binding_set)

    .Func("get_analog_stick_action_smooth_value", get_analog_stick_action_smooth_value)
    .Func("set_analog_stick_action_smooth_value", set_analog_stick_action_smooth_value)

    .Func("get_action_handle", get_action_handle)
    .Func("get_action_type", get_action_type)
    .Func("get_action_name", get_action_name)
    .Func("is_action_active", is_action_active)
    .Func("is_action_internal", is_action_internal)
    .Func("is_action_stateful", sq_is_action_stateful)
    .Func("get_group_tag_for_action", get_group_tag_for_action)
    .Func("get_group_tag_str_for_action", get_group_tag_str_for_action)

    .Func("get_action_set_handle", get_action_set_handle)
    .Func("get_action_set_name", get_action_set_name)
    .SquirrelFunc("get_action_set_actions", sq_get_action_set_actions, 2, ".i")

    .SquirrelFunc("setup_action_set", sq_setup_action_set, 3, ".sa")
    .Func("clear_action_set_actions", clear_action_set_actions)

    .Func("reset_action_set_stack", reset_action_set_stack)
    .Func("activate_action_set", activate_action_set)
    .Func("get_action_set_stack_depth", get_action_set_stack_depth)
    .Func("get_action_set_stack_item", get_action_set_stack_item)
    .Func("get_current_action_set", get_current_action_set)
    .Func("set_breaking_action_set", set_breaking_action_set)

    .Func("get_action_binding", get_action_binding)
    .Func("set_action_binding", set_action_binding)
    .Func("reset_action_binding", reset_action_binding)

    .Func("get_actions_count", get_actions_count)
    .Func("get_action_handle_by_ord", get_action_handle_by_ord)
    .Func("get_action_sets_count", get_action_sets_count)
    .Func("get_action_set_handle_by_ord", get_action_set_handle_by_ord)

    .Func("start_recording_bindings", start_recording_bindings)
    .Func("start_recording_bindings_for_single_button", start_recording_bindings_for_single_button)
    .Func("is_recording_in_progress", is_recording_in_progress)
    .Func("is_recording_complete", is_recording_complete)
    .Func("finish_recording_bindings", finish_recording_bindings)

    .Func("reset_digital_action_sticky_toggle", reset_digital_action_sticky_toggle)
    .Func("get_last_used_device_mask", get_last_used_device_mask)

    .Func("get_overall_button_clicks_count", get_overall_button_clicks_count)
    .Func("enable_darg_events_for_button_clicks", enable_darg_events_for_button_clicks)

    .Func("send_action_event", send_action_event)
    .Func("send_action_terminated_event", send_action_terminated_event)

    .Func("set_digital_event_progress_monitor", sq_install_digital_event_progress_monitor)
    .Func("enable_debug_traces", enable_debug_traces)

    .SquirrelFunc("check_bindings_conflicts", sq_check_bindings_conflicts, 3, ".ix")
    .SquirrelFunc("check_bindings_hides_action", sq_check_bindings_hides_action, 3, ".ix")
    .Func("check_bindings_conflicts_one", check_bindings_conflicts_one)

    .SquirrelFunc("get_action_bindings_text", sq_get_action_bindings_text, 2, ".s")
    .SquirrelFunc("format_ctrl_name", sq_format_ctrl_name, -3, ".iib")

    .Func("set_main_gamepad_stick_dead_zone", set_main_gamepad_stick_dead_zone)
    .Func("set_joystick_stick_dead_zone", set_joystick_stick_dead_zone)
    .Func("get_main_gamepad_stick_dead_zone", get_main_gamepad_stick_dead_zone)
    .Func("get_joystick_stick_dead_zone", get_joystick_stick_dead_zone)
    .Func("get_main_gamepad_stick_dead_zone_abs", get_main_gamepad_stick_dead_zone_abs)
    .Func("get_joystick_stick_dead_zone_abs", get_joystick_stick_dead_zone_abs)
    .Func("enable_joystick_gyroscope", enable_joystick_gyroscope)

    .Func("set_default_preset_prefix", set_default_preset_prefix)
    .Func("get_default_preset_prefix", get_default_preset_prefix)
    .Func("load_user_config", load_user_config)
    .Func("save_user_config", save_user_config)
    .Func("get_user_config_base_preset", get_user_config_base_preset)
    .Func("is_user_config_customized", is_user_config_customized)
    .Func("reset_user_config_to_preset", reset_user_config_to_preset)
    .Func("reset_user_config_to_currest_preset", reset_user_config_to_currest_preset)
    .Func("get_user_props", get_user_props)
    .Func("is_user_props_customized", is_user_props_customized)

#if DAGOR_DBGLEVEL > 0
    .Func("dump_action_sets", sq_dump_action_sets)
    .Func("dump_action_sets_stack", sq_dump_action_sets_stack)
    .Func("dump_action_set", sq_dump_action_set)
#endif
  ;

  Sqrat::Class<SingleButtonId> sqSingleButtonId(vm, "SingleButtonId");
  sqSingleButtonId
    SQ_BIND_PROP(SingleButtonId, devId)
    SQ_BIND_PROP(SingleButtonId, btnId)
  ;

  Sqrat::Class<DigitalActionBinding>(vm, "DigitalActionBinding")
    SQ_BIND_PROP(DigitalActionBinding, devId)
    SQ_BIND_PROP(DigitalActionBinding, ctrlId)
    SQ_BIND_PROP(DigitalActionBinding, eventType)
    SQ_BIND_PROP(DigitalActionBinding, axisCtrlThres)
    SQ_BIND_PROP(DigitalActionBinding, btnCtrl)
    SQ_BIND_PROP(DigitalActionBinding, stickyToggle)
    SQ_BIND_PROP(DigitalActionBinding, unordCombo)
    SQ_BIND_PROP(DigitalActionBinding, modCnt)
    SQ_BIND_PROP(DigitalActionBinding, mod)
  ;

  Sqrat::Class<AnalogAxisActionBinding>(vm, "AnalogAxisActionBinding")
    SQ_BIND_PROP(AnalogAxisActionBinding, devId)
    SQ_BIND_PROP(AnalogAxisActionBinding, axisId)
    SQ_BIND_PROP(AnalogAxisActionBinding, invAxis)
    SQ_BIND_PROP(AnalogAxisActionBinding, axisRelativeVal)
    SQ_BIND_PROP(AnalogAxisActionBinding, instantIncDecBtn)
    SQ_BIND_PROP(AnalogAxisActionBinding, quantizeValOnIncDecBtn)

    SQ_BIND_PROP(AnalogAxisActionBinding, modCnt)
    SQ_BIND_PROP(AnalogAxisActionBinding, mod)
    SQ_BIND_PROP(AnalogAxisActionBinding, minBtn)
    SQ_BIND_PROP(AnalogAxisActionBinding, maxBtn)
    SQ_BIND_PROP(AnalogAxisActionBinding, incBtn)
    SQ_BIND_PROP(AnalogAxisActionBinding, decBtn)
    SQ_BIND_PROP(AnalogAxisActionBinding, deadZoneThres)
    SQ_BIND_PROP(AnalogAxisActionBinding, nonLin)
    SQ_BIND_PROP(AnalogAxisActionBinding, maxVal)
    SQ_BIND_PROP(AnalogAxisActionBinding, relIncScale)
  ;

  Sqrat::Class<AnalogStickActionBinding>(vm, "AnalogStickActionBinding")
    SQ_BIND_PROP(AnalogStickActionBinding, devId)
    SQ_BIND_PROP(AnalogStickActionBinding, axisXId)
    SQ_BIND_PROP(AnalogStickActionBinding, axisYId)
    SQ_BIND_PROP(AnalogStickActionBinding, axisXinv)
    SQ_BIND_PROP(AnalogStickActionBinding, axisYinv)
    SQ_BIND_PROP(AnalogStickActionBinding, modCnt)
    SQ_BIND_PROP(AnalogStickActionBinding, mod)
    SQ_BIND_PROP(AnalogStickActionBinding, minXBtn)
    SQ_BIND_PROP(AnalogStickActionBinding, maxXBtn)
    SQ_BIND_PROP(AnalogStickActionBinding, minYBtn)
    SQ_BIND_PROP(AnalogStickActionBinding, maxYBtn)
    SQ_BIND_PROP(AnalogStickActionBinding, deadZoneThres)
    SQ_BIND_PROP(AnalogStickActionBinding, axisSnapAngK)
    SQ_BIND_PROP(AnalogStickActionBinding, nonLin)
    SQ_BIND_PROP(AnalogStickActionBinding, maxVal)
    SQ_BIND_PROP(AnalogStickActionBinding, sensScale)
  ;

  Sqrat::Class<DigitalAction>(vm, "DigitalAction")
    SQ_BIND_MEMBER_VAR(DigitalAction, bState)
    SQ_BIND_MEMBER_VAR(DigitalAction, bActive)
    SQ_BIND_MEMBER_VAR(DigitalAction, bActivePrev)
  ;

  Sqrat::Class<AnalogAxisAction>(vm, "AnalogAxisAction")
    SQ_BIND_MEMBER_VAR(AnalogAxisAction, x)
    SQ_BIND_MEMBER_VAR(AnalogAxisAction, bActive)
    SQ_BIND_MEMBER_VAR(AnalogAxisAction, bActivePrev)
  ;

  Sqrat::Class<AnalogStickAction>(vm, "AnalogStickAction")
    SQ_BIND_MEMBER_VAR(AnalogStickAction, x)
    SQ_BIND_MEMBER_VAR(AnalogStickAction, y)
    SQ_BIND_MEMBER_VAR(AnalogStickAction, bActive)
    SQ_BIND_MEMBER_VAR(AnalogStickAction, bActivePrev)
  ;
  // clang-format on

  dainp.Bind("SingleButtonId", sqSingleButtonId);

  moduleMgr->addNativeModule("dainput2", dainp);
}
