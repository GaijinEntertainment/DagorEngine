// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>
#include <colorPipette/colorPipette.h>
#include <string.h>
#include <util/dag_string.h>

namespace webui
{
static const char colorpicker_js[] = {
#include "colorPicker.js.inl"
};
static const char colorgradient_js[] = {
#include "colorGradient.js.inl"
};
static const char cookie_js[] = {
#include "cookie.js.inl"
};

static void on_colorpicker(RequestInfo *params) { return html_response_raw(params->conn, colorpicker_js, sizeof(colorpicker_js) - 1); }
static void on_colorgradient(RequestInfo *params)
{
  return html_response_raw(params->conn, colorgradient_js, sizeof(colorgradient_js) - 1);
}
static void on_cookie(RequestInfo *params) { return html_response_raw(params->conn, cookie_js, sizeof(cookie_js) - 1); }


static String pipette_result;

static void pipette_callback(void * /*user_data*/, bool picked, int r, int g, int b)
{
  if (picked)
    pipette_result.printf(64, "%d,%d,%d", r, g, b);
  else
    pipette_result.setStr("stop");
}

static void on_color_pipette(RequestInfo *params)
{
  if (!strcmp(params->params[0], "start"))
  {
    pipette_result.setStr("");
    colorpipette::start(pipette_callback, nullptr);
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "get_result"))
  {
    html_response_raw(params->conn, pipette_result.c_str());
  }
  else if (!strcmp(params->params[0], "stop"))
  {
    colorpipette::terminate();
    html_response_raw(params->conn, "");
  }
  else
    html_response_raw(params->conn, "Invalid command");
}

HttpPlugin color_pipette_http_plugin = {"color_pipette", NULL, NULL, on_color_pipette};
HttpPlugin colorpicker_http_plugin = {"colorPicker.js", NULL, NULL, on_colorpicker};
HttpPlugin colorgradient_http_plugin = {"colorGradient.js", NULL, NULL, on_colorgradient};
HttpPlugin cookie_http_plugin = {"cookie.js", NULL, NULL, on_cookie};

} // namespace webui