#include <ctype.h>
#include <stdlib.h>
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
#include <webui/httpserver.h>
#include <webui/helpers.h>


#include <rendInst/rendInstGen.h>

using namespace webui;


void on_rendinst_colors(RequestInfo *params)
{
  if (!params->params)
  {
    const char *inlined =
#include "rendinstColors.html.inl"
      ;
    html_response_raw(params->conn, inlined);
  }
  else if (!strcmp(params->params[0], "attach"))
  {
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "alldata"))
  {
    String s(tmpmem);
    s.reserve(20000);
    s.aprintf(0, "{\"array\":[");

    int i = 0;
    rendinst::get_ri_color_infos([&s, &i](E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name) {
      s.aprintf(1024, "%s{\"name\":\"%s\",\"colorFrom\":\"%02X%02X%02X%02X\",\"colorTo\":\"%02X%02X%02X%02X\"}\n", i > 0 ? "," : "",
        name, colorFrom.r, colorFrom.g, colorFrom.b, colorFrom.a, colorTo.r, colorTo.g, colorTo.b, colorTo.a);
      ++i;
    });

    s.aprintf(0, "]}");
    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "setcolor"))
  {
    const char *name = params->params[2];
    const char *from = params->params[4];
    const char *to = params->params[6];

    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
    sscanf(from, "%02X%02X%02X%02X", &r, &g, &b, &a);
    E3DCOLOR c0(r, g, b, a);
    sscanf(to, "%02X%02X%02X%02X", &r, &g, &b, &a);
    E3DCOLOR c1(r, g, b, a);

    rendinst::update_rigen_color(name, c0, c1);
    html_response_raw(params->conn, "");
  }
  // else if (!strcmp(params->params[0], "detach")) {}
}

webui::HttpPlugin webui::rendinst_colors_http_plugin = {"rendinst_colors", "rendisnt colors editor", NULL, on_rendinst_colors};
