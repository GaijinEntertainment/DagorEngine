// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sqEventDispatch.h"

#include <quirrel/sqEventBus/sqEventBus.h>
#include <sqrat.h>
#include <json/value.h>
#include <util/dag_delayedAction.h>


namespace webbrowser
{

void dispatch_sq_event(EventType event_type, const char *url, const char *title, bool can_go_back, const char *error_desc)
{
  Json::Value params(Json::objectValue);

  params["eventType"] = Json::Value(event_type);
  params["canGoBack"] = Json::Value(can_go_back);
  if (url && ::strlen(url) > 0)
    params["url"] = Json::Value(url);
  if (title && ::strlen(title) > 0)
    params["title"] = Json::Value(title);
  if (error_desc && ::strlen(error_desc) > 0)
    params["errorDesc"] = Json::Value(error_desc);

  // eventbus works with multiple VMs, so wait until it is safe to perform operation on all of them
  run_action_on_main_thread([params]() { sqeventbus::send_event("browser_event", params); });
}


void dispatch_js_call_sq_event(const char *name, const char *p1, const char *p2)
{
  Json::Value params(Json::objectValue);

  params["name"] = Json::Value(name && *name ? name : "");
  params["p1"] = Json::Value(p1 && *p1 ? p1 : "");
  params["p2"] = Json::Value(p2 && *p2 ? p2 : "");

  // eventbus works with multiple VMs, so wait until it is safe to perform operation on all of them
  run_action_on_main_thread([params]() { sqeventbus::send_event("browser_window_method_call", params); });
}

} // namespace webbrowser
