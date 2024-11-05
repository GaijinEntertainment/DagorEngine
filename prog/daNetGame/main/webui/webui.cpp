// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/webui.h"

#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>

#include <webui/editVarPlugin.h>
#include <webui/helpers.h>
#include <webui/httpserver.h>
#include <webui/shaderEditors.h>
#include <webui/sqDebuggerPlugin.h>
#include <webui/websocket/webSocketStream.h>

#include "render/renderer.h"
#include "main/scriptDebug.h"
#include "net/dedicated.h"

void init_webui()
{
  const DataBlock &debug_block = *dgs_get_settings()->getBlockByNameEx("debug");
  if (debug_block.getBool("http_server", DAGOR_DBGLEVEL > 0))
  {
    static webui::HttpPlugin test_http_plugins[] = {
      webui::profiler_http_plugin,
      webui::shader_http_plugin,
      webui::color_pipette_http_plugin,
      webui::colorpicker_http_plugin,
      webui::curveeditor_http_plugin,
      webui::cookie_http_plugin,
      webui::editvar_http_plugin,
      webui::ecsviewer_http_plugin,
      webui::colorgradient_http_plugin,
      webui::rendinst_colors_http_plugin,

#if DAGOR_DBGLEVEL > 0
      {"sqdebug_game", "squirrel debugger (in-game scripts)", NULL, webui::on_sqdebug<SQ_DEBUGGER_GAME_SCRIPTS>},
      {"sqdebug_darg", "squirrel debugger (UI)", NULL, webui::on_sqdebug<SQ_DEBUGGER_DARG_SCRIPTS>},
      {"sqdebug_overlay_ui", "squirrel debugger (Overlay UI scripts)", NULL, webui::on_sqdebug<SQ_DEBUGGER_OVERLAY_UI_SCRIPTS>},
#endif

      NULL
    };
    webui::plugin_lists[0] = webui::dagor_http_plugins;
    webui::plugin_lists[1] = test_http_plugins;
    webui::plugin_lists[2] = webui::webview_http_plugins;
    webui::plugin_lists[3] = webui::webview_files_http_plugins;
#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
    webui::plugin_lists[4] = get_renderer_http_plugins();
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT
    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (dedicated::is_dedicated())
      cfg.bindPort = debug_block.getInt("dedicatedWebUIPort", 23456);
    else
      cfg.bindPort = debug_block.getInt("WebUIPort", 23456);
#if _TARGET_XBOX
    cfg.bindPort = 4600;
#endif
    webui::startup(&cfg);

    if (debug_block.getBool("enableWebSocketStream", false))
    {
      int webSocketPort = debug_block.getInt("WebSocketPort", 9112);
      if (dedicated::is_dedicated())
        webSocketPort = debug_block.getInt("dedicatedWebSocketPort", webSocketPort);

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
