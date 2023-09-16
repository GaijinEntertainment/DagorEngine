#include <webBrowserHelper/webBrowser.h>


namespace webbrowser
{

void dispatch_sq_event(EventType e, const char *url, const char *title = nullptr, bool can_go_back = false, const char *err = nullptr);

void dispatch_js_call_sq_event(const char *name, const char *p1, const char *p2);

} // namespace webbrowser
