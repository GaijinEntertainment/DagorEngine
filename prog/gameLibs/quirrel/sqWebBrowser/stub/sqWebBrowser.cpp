#include <sqModules/sqModules.h>

namespace bindquirrel
{

static const int BROWSER_DUMMY_EVENT_VALUE = 0xFF;
static bool can_use_embedded_browser() { return false; }
static const char *browser_get_current_url() { return NULL; }
static void browser_void_stub() {} // VOID
static void add_window_method_stub(const char *, unsigned) {}


void bind_webbrowser(SqModules *sq_modules_mgr)
{
  Sqrat::Table tbl(sq_modules_mgr->getVM());
  tbl.Func("can_use_embeded_browser", can_use_embedded_browser)
    .Func("browser_go_back", browser_void_stub)
    .Func("browser_reload_page", browser_void_stub)
    .Func("browser_go", browser_void_stub)
    .Func("browser_get_current_url", browser_get_current_url)
    .Func("browser_get_current_title", browser_void_stub)
    .Func("browser_add_window_method", add_window_method_stub);

#define _SET_VALUE(x) tbl.SetValue(#x, BROWSER_DUMMY_EVENT_VALUE)
  _SET_VALUE(BROWSER_EVENT_INITIALIZED);
  _SET_VALUE(BROWSER_EVENT_DOCUMENT_READY);
  _SET_VALUE(BROWSER_EVENT_FAIL_LOADING_FRAME);
  _SET_VALUE(BROWSER_EVENT_CANT_DOWNLOAD);

  _SET_VALUE(BROWSER_EVENT_FINISH_LOADING_FRAME);
  _SET_VALUE(BROWSER_EVENT_BEGIN_LOADING_FRAME);
  _SET_VALUE(BROWSER_EVENT_NEED_RESEND_FRAME);
  _SET_VALUE(BROWSER_EVENT_BROWSER_CRASHED);
#undef _SET_VALUE

  sq_modules_mgr->addNativeModule("browser", tbl);
}

} // namespace bindquirrel