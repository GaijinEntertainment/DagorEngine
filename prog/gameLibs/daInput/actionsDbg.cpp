#include "actionData.h"
#include <util/dag_string.h>
#include <gui/dag_visualLog.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>

void dainput::dump_action_sets()
{
  debug("dump_action_sets: %d sets, (%d,%d,%d actions)", actionSets.size(), agData.ad.size(), agData.aa.size(), agData.as.size());
  for (int i = 0; i < actionSets.size(); i++)
  {
    debug("actionSet[%d] \"%s\" {", i, actionSetNameIdx.getName(i));
    for (action_handle_t a : actionSets[i].actions)
      debug("  %s { type=%X name=%X grp=\"%s\" }", get_action_name_fast(a), get_action_type(a), a, get_group_tag_str_for_action(a));
    debug("}");
  }
}
static void build_modifiers_str(String &bindStr, int modCnt, const dainput::SingleButtonId *mod)
{
  if (modCnt)
  {
    String tmpStr;
    bindStr.printf(0, "mod=[");
    for (int j = 0; j < modCnt; j++)
      bindStr.aprintf(0, "%s%s", build_btn_name(tmpStr, mod[j]), j + 1 < modCnt ? "+" : "] ");
  }
  else
    bindStr = "";
}
void dainput::dump_action_bindings()
{
  String bindStr, tmpStr;
  debug("dump_action_bindings: %d actions, %d columns", get_actions_count(), get_actions_binding_columns());
  for (int i = 0; i < get_actions_count(); i++)
  {
    action_handle_t a = get_action_handle_by_ord(i);
    debug("  %04X type=%4X  \"%s\"", a, get_action_type(a), get_action_name(a));
    for (int k = 0; k < get_actions_binding_columns(); k++)
    {
      bindStr = "";
      switch (get_action_type(a) & TYPEGRP__MASK)
      {
        case TYPEGRP_DIGITAL:
          if (DigitalActionBinding *b = get_digital_action_binding(a, k))
          {
            build_modifiers_str(bindStr, b->modCnt, b->mod);
            bindStr.aprintf(0, "%s  type:%s", build_ctrl_name(tmpStr, b->devId, b->ctrlId, b->btnCtrl),
              get_digital_button_type_name(b->eventType));
          }
          break;

        case TYPEGRP_AXIS:
          if (AnalogAxisActionBinding *b = get_analog_axis_action_binding(a, k))
          {
            bool stateful = agData.aa[a & ~TYPEGRP__MASK].flags & ACTIONF_stateful;
            build_modifiers_str(bindStr, b->modCnt, b->mod);
            bindStr.aprintf(0, "[%s%s%s%s%s] ", build_ctrl_name(tmpStr, b->devId, b->axisId, false), b->invAxis ? "(inverse)" : "",
              b->axisRelativeVal ? "(rel)" : "", b->instantIncDecBtn ? "(inst)" : "", b->quantizeValOnIncDecBtn ? "(quant)" : "");
            bindStr.aprintf(0, "B+:%s", build_btn_name(tmpStr, b->maxBtn));
            if (get_action_type(a) == TYPE_STEERWHEEL || stateful)
              bindStr.aprintf(0, " B-:%s", build_btn_name(tmpStr, b->minBtn));
            if (stateful)
            {
              bindStr.aprintf(0, " B++:%s", build_btn_name(tmpStr, b->incBtn));
              bindStr.aprintf(0, " B--:%s", build_btn_name(tmpStr, b->decBtn));
            }
          }
          break;

        case TYPEGRP_STICK:
          if (AnalogStickActionBinding *b = get_analog_stick_action_binding(a, k))
          {
            build_modifiers_str(bindStr, b->modCnt, b->mod);
            bindStr.aprintf(0, "[%s%s/", build_ctrl_name(tmpStr, b->devId, b->axisXId, false), b->axisXinv ? "(inverse)" : "");
            bindStr.aprintf(0, "%s%s] ", build_ctrl_name(tmpStr, b->devId, b->axisYId, false), b->axisYinv ? "(inverse)" : "");
            if (b->minXBtn.devId)
              bindStr.aprintf(0, "X-:%s ", build_btn_name(tmpStr, b->minXBtn));
            if (b->maxXBtn.devId)
              bindStr.aprintf(0, "X+:%s ", build_btn_name(tmpStr, b->maxXBtn));
            if (b->minYBtn.devId)
              bindStr.aprintf(0, "Y-:%s ", build_btn_name(tmpStr, b->minYBtn));
            if (b->maxYBtn.devId)
              bindStr.aprintf(0, "Y+:%s ", build_btn_name(tmpStr, b->maxYBtn));
          }
          break;
      }
      if (!bindStr.empty())
        debug("    [%d] %s", k, bindStr);
    }
  }
}
void dainput::dump_action_sets_stack()
{
  debug("action set stack (%d, top down):", actionSetStack.size());
  for (int i = actionSetStack.size() - 1; i >= 0; i--)
    debug("  [%4X] %s (prio=%d)", actionSetStack[i], actionSetNameIdx.getName(actionSetStack[i]),
      actionSets[actionSetStack[i]].ordPriority);
}

static bool visuallog_upd_proc(int event, struct visuallog::LogItem *item)
{
  if (event == visuallog::EVT_REFRESH)
  {
    if (!item->param)
    {
      item->param = (void *)(intptr_t)get_time_msec();
      return true;
    }
    int t0 = (intptr_t)item->param;
    return get_time_msec() < t0 + 5000;
  }
  return true;
}
static void visuallog_debug(const char *s)
{
  visuallog::logmsg(s, &visuallog_upd_proc, NULL, E3DCOLOR(255, 255, 0, 96), 1000);
  debug("daInp: %s", s);
}
void dainput::enable_debug_traces(bool enable_actionset_logs, bool enable_event_logs)
{
  actionset_logs = enable_actionset_logs;
  event_logs = enable_event_logs;
  logscreen = (actionset_logs || event_logs) ? &visuallog_debug : nullptr;
}
