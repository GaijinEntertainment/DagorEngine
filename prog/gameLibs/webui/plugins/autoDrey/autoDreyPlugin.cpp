#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_framemem.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_clipboard.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_base64.h>
#include <util/dag_watchdog.h>
#include <util/dag_simpleString.h>
#include <perfMon/dag_cpuFreq.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>

#include <quirrel/sqDrey/sqDrey.h>
#include <quirrel/sqModules/sqModules.h>

#include <squirrel.h>
#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>


using namespace webui;


#if _TARGET_PC_WIN

static bool is_web_interface_active = false;

void on_auto_drey(RequestInfo *params)
{
  if (!params->params)
  {
    const char *inlined =
#include "autoDrey.html.inl"
      ;
    html_response_raw(params->conn, inlined);
  }
  else if (!strcmp(params->params[0], "attach"))
  {
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "get_output_marker"))
  {
    is_web_interface_active = true;
    String s(0, "%d", sqdrey::get_drey_call_count());
    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "get_output_text"))
  {
    html_response_raw(params->conn, sqdrey::get_drey_result().str());
  }
  else if (!strcmp(params->params[0], "detach"))
  {
    html_response_raw(params->conn, "");
    is_web_interface_active = false;
  }
  else
  {
    html_response_raw(params->conn, "");
  }
}


#else


void auto_drey_init(const char *) {}

void auto_drey_before_reload_scripts(HSQUIRRELVM, const char *, const char *) {}

void auto_drey_after_reload_scripts(HSQUIRRELVM, const char *, const char *) {}

void on_auto_drey(RequestInfo *params) { html_response_raw(params->conn, "This plugin is available only for Windows.\n"); }


#endif


webui::HttpPlugin webui::auto_drey_http_plugin = {"auto_drey", "Drey (static analyzer for Squirrel 3)", NULL, on_auto_drey};
