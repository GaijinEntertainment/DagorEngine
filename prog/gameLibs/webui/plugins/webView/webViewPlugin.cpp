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
#include <webui/websocket/webSocketStream.h>

void on_webview_settings(webui::RequestInfo *params)
{
  String resp;
  resp.printf(0, "{ \"port\": %d }", websocket::get_port());
  webui::html_response_raw(params->conn, "application/json", resp.str(), resp.length());
}

void on_webview_index(webui::RequestInfo *params)
{
  if (!params->params)
  {
    static char inlinedData[] = {
#include "vue/index.html.inl"
    };
    webui::html_response_raw(params->conn, inlinedData, sizeof(inlinedData) - 1);
  }
}

webui::HttpPlugin webui::webview_http_plugins[] = {{"webview", "Advanced WebView (DM Viewer, ECS Viewer)", NULL, on_webview_index},
  {"webview/settings.json", NULL, NULL, on_webview_settings}, {NULL}};