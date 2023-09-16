#include "sqEventDispatch.h"

#include <quirrel/sqEventBus/sqEventBus.h>
#include <sqrat.h>


namespace webbrowser
{

void dispatch_sq_event(EventType event_type, const char *url, const char *title, bool can_go_back, const char *error_desc)
{
  HSQUIRRELVM vm = sqeventbus::get_vm();
  if (!vm)
    return;

  Sqrat::Table params(vm);

  params.SetValue("eventType", event_type);
  params.SetValue("canGoBack", can_go_back);
  if (url && ::strlen(url) > 0)
    params.SetValue("url", url);
  if (title && ::strlen(title) > 0)
    params.SetValue("title", title);
  if (error_desc && ::strlen(error_desc) > 0)
    params.SetValue("errorDesc", error_desc);

  sqeventbus::send_event("browser_event", params);
}


void dispatch_js_call_sq_event(const char *name, const char *p1, const char *p2)
{
  HSQUIRRELVM vm = sqeventbus::get_vm();
  if (!vm)
    return;

  Sqrat::Table params(vm);

  params.SetValue("name", (name && *name ? name : ""));
  params.SetValue("p1", (p1 && *p1 ? p1 : ""));
  params.SetValue("p2", (p2 && *p2 ? p2 : ""));

  sqeventbus::send_event("browser_window_method_call", params);
}

} // namespace webbrowser
