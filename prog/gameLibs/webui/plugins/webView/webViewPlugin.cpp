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
#include "web/index.html.inl"
    };
    webui::html_response_raw(params->conn, inlinedData, sizeof(inlinedData) - 1);
  }
}

void on_file_font_1(webui::RequestInfo *params)
{
  static uint8_t inlinedData[] = {
#include "web/fontawesome-webfont.woff2.inl"
  };
  webui::html_response_raw(params->conn, "font/woff2", (const char *)inlinedData, sizeof(inlinedData) - 1);
}

void on_file_font_2(webui::RequestInfo *params)
{
  static uint8_t inlinedData[] = {
#include "web/fontawesome-webfont.woff.inl"
  };
  webui::html_response_raw(params->conn, "application/font-woff", (const char *)inlinedData, sizeof(inlinedData) - 1);
}

void on_file_font_3(webui::RequestInfo *params)
{
  static uint8_t inlinedData[] = {
#include "web/fontawesome-webfont.ttf.inl"
  };
  webui::html_response_raw(params->conn, "application/x-font-ttf", (const char *)inlinedData, sizeof(inlinedData) - 1);
}

void on_file_font_4(webui::RequestInfo *params)
{
  static uint8_t inlinedData[] = {
#include "web/fontawesome-webfont.eot.inl"
  };
  webui::html_response_raw(params->conn, "application/vnd.ms-fontobject", (const char *)inlinedData, sizeof(inlinedData) - 1);
}

void on_file_svg_1(webui::RequestInfo *params)
{
  static uint8_t inlinedData[] = {
#include "web/fontawesome-webfont.svg.inl"
  };
  webui::html_response_raw(params->conn, "image/svg+xml", (const char *)inlinedData, sizeof(inlinedData) - 1);
}

void on_file_svg_2(webui::RequestInfo *params)
{
  static uint8_t inlinedData[] = {
#include "web/jsoneditor-icons.svg.inl"
  };
  webui::html_response_raw(params->conn, "image/svg+xml", (const char *)inlinedData, sizeof(inlinedData) - 1);
}

void on_file_css_1(webui::RequestInfo *params)
{
  static char inlinedData[] = {
#include "web/assets/styles.css.inl"
  };
  webui::html_response_raw(params->conn, "text/css", inlinedData, sizeof(inlinedData) - 1);
}

void on_file_css_2(webui::RequestInfo *params)
{
  static char inlinedData[] = {
#include "web/styles.bundle.css.inl"
  };
  webui::html_response_raw(params->conn, "text/css", inlinedData, sizeof(inlinedData) - 1);
}

void on_file_js_1(webui::RequestInfo *params)
{
  static char inlinedData[] = {
#include "web/inline.bundle.js.inl"
  };
  webui::html_response_raw(params->conn, "text/javascript", inlinedData, sizeof(inlinedData) - 1);
}

void on_file_js_2(webui::RequestInfo *params)
{
  static char inlinedData[] = {
#include "web/polyfills.bundle.js.inl"
  };
  webui::html_response_raw(params->conn, "text/javascript", inlinedData, sizeof(inlinedData) - 1);
}

void on_file_js_3(webui::RequestInfo *params)
{
  static char inlinedData[] = {
#include "web/vendor.bundle.js.inl"
  };
  webui::html_response_raw(params->conn, "text/javascript", inlinedData, sizeof(inlinedData) - 1);
}

void on_file_js_4(webui::RequestInfo *params)
{
  static char inlinedData[] = {
#include "web/main.bundle.js.inl"
  };
  webui::html_response_raw(params->conn, "text/javascript", inlinedData, sizeof(inlinedData) - 1);
}

webui::HttpPlugin webui::webview_http_plugins[] = {{"webview", "Advanced WebView (DM Viewer, ECS Viewer)", NULL, on_webview_index},
  {"webview/settings.json", NULL, NULL, on_webview_settings}, {"webview/assets/styles.css", NULL, NULL, on_file_css_1},
  {"webview/styles.bundle.css", NULL, NULL, on_file_css_2}, {"webview/inline.bundle.js", NULL, NULL, on_file_js_1},
  {"webview/polyfills.bundle.js", NULL, NULL, on_file_js_2}, {"webview/vendor.bundle.js", NULL, NULL, on_file_js_3},
  {"webview/main.bundle.js", NULL, NULL, on_file_js_4}, {"webview/fontawesome-webfont.woff2", NULL, NULL, on_file_font_1},
  {"webview/fontawesome-webfont.woff", NULL, NULL, on_file_font_2}, {"webview/fontawesome-webfont.ttf", NULL, NULL, on_file_font_3},
  {"webview/fontawesome-webfont.eot", NULL, NULL, on_file_font_4}, {"webview/fontawesome-webfont.svg", NULL, NULL, on_file_svg_1},
  {"webview/jsoneditor-icons.svg", NULL, NULL, on_file_svg_2}, {NULL}};