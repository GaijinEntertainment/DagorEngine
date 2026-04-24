// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/webui.h"

#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>

#include <webui/editVarPlugin.h>
#include <webui/helpers.h>
#include <webui/httpserver.h>
#include <webui/dargMcpPlugin.h>
#include <webui/shaderEditors.h>
#include <webui/sqDebuggerPlugin.h>
#include <webui/websocket/webSocketStream.h>

#include "render/renderer.h"
#include "main/scriptDebug.h"
#include "net/dedicated.h"
#include "ui/overlay.h"

void init_webui()
{
  const DataBlock &debug_block = *dgs_get_settings()->getBlockByNameEx("debug");
  if (debug_block.getBool("http_server", DAGOR_DBGLEVEL > 0))
  {
    static webui::HttpPlugin test_http_plugins[] = {webui::profiler_http_plugin, webui::shader_http_plugin,
      webui::color_pipette_http_plugin, webui::colorpicker_http_plugin, webui::curveeditor_http_plugin, webui::cookie_http_plugin,
      webui::editvar_http_plugin, webui::ecsviewer_http_plugin, webui::colorgradient_http_plugin, webui::rendinst_colors_http_plugin,
      NULL};

    int plugins = 0;
    webui::plugin_lists[plugins++] = webui::dagor_http_plugins;
    webui::plugin_lists[plugins++] = test_http_plugins;
    webui::plugin_lists[plugins++] = webui::webview_http_plugins;
    webui::plugin_lists[plugins++] = webui::webview_files_http_plugins;

#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
    webui::plugin_lists[plugins++] = get_renderer_http_plugins();
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT

#if DAGOR_DBGLEVEL > 0
    webui::plugin_lists[plugins++] = webui::darg_mcp_http_plugins;
    webui::set_darg_mcp_scene_provider(overlay_ui::gui_scene);
#endif

    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (dedicated::is_dedicated())
      cfg.bindPort = debug_block.getInt("dedicatedWebUIPort", 23457);
    else
      cfg.bindPort = debug_block.getInt("WebUIPort", 23456);
    webui::startup(&cfg);

    if (debug_block.getBool("enableWebSocketStream", false))
    {
      int webSocketPort;
      if (dedicated::is_dedicated())
        webSocketPort = debug_block.getInt("dedicatedWebSocketPort", 9114);
      else
        webSocketPort = debug_block.getInt("WebSocketPort", 9112);

      webui::init_dmviewer_plugin();
      webui::init_ecsviewer_plugin();
      websocket::start(webSocketPort);
    }
  }

  de3_webui_init();
}

void stop_webui()
{
  de3_webui_term();
  webui::shutdown();
  websocket::stop();
}

void update_webui()
{
  TIME_PROFILE(webui__update);
  webui::update();
  websocket::update();
}

void select_entity_webui(const ecs::EntityId &eid)
{
  BsonStream bson;
  bson.add("selectEid", (uint32_t)eid);
  websocket::send_binary(bson);
}
