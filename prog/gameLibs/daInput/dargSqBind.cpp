#include <daRg/dag_events.h>
#include <daRg/dag_guiScene.h>
#include <sqrat.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/algorithm.h>

#include "actionData.h"


static const size_t MAX_SCENES = 2;

static darg::IGuiScene *darg_scenes[MAX_SCENES] = {nullptr, nullptr};


static bool ui_action_triggered_cb(dainput::action_handle_t action, bool action_terminated, int dur_ms)
{
  if (dainput::event_logs && dainput::logscreen)
  {
    G_STATIC_ASSERT(MAX_SCENES == 2); // for log message format below
    dainput::logscreen(String(0, "%s%s  [daRg scene=%p | %p, dur=%d]", dainput::get_action_name(action),
      action_terminated ? ":end" : "", darg_scenes[0], darg_scenes[1], dur_ms));
  }

  if (action_terminated && (action & dainput::TYPEGRP__MASK) == dainput::TYPEGRP_DIGITAL)
  {
    if (dur_ms > dainput::agData.ad[action & ~dainput::TYPEGRP__MASK].uiHoldToToggleDur)
      action_terminated = false; // send termination of long action as another action (hold/release -> clicks)
  }

  for (darg::IGuiScene *scene : darg_scenes)
  {
    if (!scene)
      continue;

    bool processed = false;
    Sqrat::Table data(scene->getScriptVM());
    if (!action_terminated)
    {
      if ((action & dainput::TYPEGRP__MASK) == dainput::TYPEGRP_AXIS)
      {
        const auto &as = dainput::get_analog_axis_action_state(action);
        data.SetValue("active", as.bActive);
        data.SetValue("x", as.x);
      }
      else if ((action & dainput::TYPEGRP__MASK) == dainput::TYPEGRP_STICK)
      {
        const auto &as = dainput::get_analog_stick_action_state(action);
        data.SetValue("active", as.bActive);
        data.SetValue("x", as.x);
        data.SetValue("y", as.y);
      }
      processed = scene->getEvents().sendEvent(dainput::get_action_name(action), data);
    }
    else
    {
      data.SetValue("dur", dur_ms);
      data.SetValue("appActive", dgs_app_active);
      processed = scene->getEvents().sendEvent(String(0, "%s:end", dainput::get_action_name(action)), data);
    }

    if (processed)
      break;
  }

  return false;
}


static void report_clicks_cb(unsigned dev_mask, unsigned total_clicks)
{
  if (dainput::event_logs && dainput::logscreen)
  {
    G_STATIC_ASSERT(MAX_SCENES == 2); // for log message format below
    dainput::logscreen(String(0, "dev=%d clicks=%d [daRg scene=%p | %p]", dev_mask, total_clicks, darg_scenes[0], darg_scenes[1]));
  }

  for (darg::IGuiScene *scene : darg_scenes)
  {
    if (!scene)
      continue;

    Sqrat::Table data(scene->getScriptVM());
    data.SetValue("dev_mask", dev_mask);
    data.SetValue("clicks", total_clicks);
    bool processed = scene->getEvents().sendEvent("dainput::click", data);
    if (processed)
      break;
  }
}


void dainput::register_darg_scene(int idx, darg::IGuiScene *scn)
{
  G_ASSERTF_RETURN(idx >= 0 && idx < MAX_SCENES, , "Invalid darg scene index %d of max %d", idx, MAX_SCENES);
  G_ASSERT(!darg_scenes[idx]);
  darg_scenes[idx] = scn;
  dainput::notify_ui_action_triggered_cb = &ui_action_triggered_cb;
}


void dainput::unregister_darg_scene(int idx)
{
  G_ASSERTF_RETURN(idx >= 0 && idx < MAX_SCENES, , "Invalid darg scene index %d of max %d", idx, MAX_SCENES);
  darg_scenes[idx] = nullptr;

  darg::IGuiScene **scenesEnd = darg_scenes + MAX_SCENES;
  bool haveScene = eastl::find_if(darg_scenes, scenesEnd, [](darg::IGuiScene *scn) { return scn != nullptr; }) != scenesEnd;
  if (!haveScene)
    dainput::notify_ui_action_triggered_cb = nullptr;
}


void dainput::enable_darg_events_for_button_clicks(bool enable, unsigned dev_used_mask)
{
  if (enable)
    dainput::notify_clicks_dev_mask |= dev_used_mask;
  else
    dainput::notify_clicks_dev_mask &= ~dev_used_mask;
  dainput::notify_clicks_cb = dainput::notify_clicks_dev_mask ? &report_clicks_cb : nullptr;
}
