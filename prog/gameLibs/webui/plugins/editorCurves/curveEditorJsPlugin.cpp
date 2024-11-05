// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>

namespace webui
{

static const char curveeditor_js[] = {
#include "curveEditor.js.inl"
};

static void on_curveeditor(RequestInfo *params) { return html_response_raw(params->conn, curveeditor_js, sizeof(curveeditor_js) - 1); }

HttpPlugin curveeditor_http_plugin = {"curveEditor.js", NULL, NULL, on_curveeditor};

} // namespace webui
