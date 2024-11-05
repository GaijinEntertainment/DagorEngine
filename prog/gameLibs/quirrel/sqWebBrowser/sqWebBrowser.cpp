// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webBrowserHelper/webBrowser.h>
#include <sqModules/sqModules.h>

/**@module browser
Opens and control embeded web browser. Requires native cef binary and its integration
*/

namespace bindquirrel
{

bool can_use_embedded_browser() { return webbrowser::get_helper()->canEnable(); }

void go_back() { webbrowser::get_helper()->goBack(); }

void reload() { webbrowser::get_helper()->reload(); }

void go(const char *url) { webbrowser::get_helper()->go(url); }

const char *get_current_url() { return webbrowser::get_helper()->getUrl(); }

const char *get_current_title() { return webbrowser::get_helper()->getTitle(); }

void add_window_method(const char *name, unsigned params_cnt)
{
  webbrowser::get_helper()->getBrowser()->addWindowMethod(name, params_cnt);
}

void bind_webbrowser(SqModules *sq_modules_mgr)
{
  Sqrat::Table tbl(sq_modules_mgr->getVM());
  tbl //
    .Func("can_use_embeded_browser", can_use_embedded_browser)
    .Func("browser_go_back", go_back)
    .Func("browser_reload_page", reload)
    .Func("browser_go", go)
    .Func("browser_get_current_url", get_current_url)
    .Func("browser_get_current_title", get_current_title)
    .Func("browser_add_window_method", add_window_method)
    /**/;

  using namespace webbrowser;

///@skipline
#define CONST(x) tbl.SetValue(#x, x)
  CONST(BROWSER_EVENT_INITIALIZED);
  CONST(BROWSER_EVENT_DOCUMENT_READY);
  CONST(BROWSER_EVENT_FAIL_LOADING_FRAME);
  CONST(BROWSER_EVENT_CANT_DOWNLOAD);

  CONST(BROWSER_EVENT_FINISH_LOADING_FRAME);
  CONST(BROWSER_EVENT_BEGIN_LOADING_FRAME);
  CONST(BROWSER_EVENT_NEED_RESEND_FRAME);
  CONST(BROWSER_EVENT_BROWSER_CRASHED);
#undef CONST

  sq_modules_mgr->addNativeModule("browser", tbl);
}

} // namespace bindquirrel