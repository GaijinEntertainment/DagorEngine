// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/websocket/webSocketStream.h>
#include <startup/dag_globalSettings.h>

class WebUIAdapter : public IEditorService
{
public:
  WebUIAdapter()
  {
    constexpr int webSocketPort = 9113;
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setBool("enableWebSocketStream", true);
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setInt("WebSocketPort", webSocketPort);

    static webui::HttpPlugin http_plugins[] = {webui::profiler_http_plugin, webui::shader_http_plugin,
      webui::color_pipette_http_plugin, webui::colorpicker_http_plugin, webui::cookie_http_plugin, webui::ecsviewer_http_plugin, NULL};

    webui::plugin_lists[0] = webui::dagor_http_plugins;
    webui::plugin_lists[1] = webui::webview_http_plugins;
    webui::plugin_lists[2] = webui::webview_files_http_plugins;
    webui::plugin_lists[3] = http_plugins;
    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.bindPort = 23457;
    webui::startup(&cfg);
    webui::init_ecsviewer_plugin();
    websocket::start(webSocketPort);
  }

  ~WebUIAdapter() override
  {
    webui::shutdown();
    websocket::stop();
  }

  const char *getServiceName() const override { return serviceName; };
  const char *getServiceFriendlyName() const override { return friendlyServiceName; };

  void setServiceVisible(bool vis) override { isVisible = true; };
  bool getServiceVisible() const override { return isVisible; };

  void actService(float dt) override
  {
    webui::update();
    websocket::update();
  }

  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void *queryInterfacePtr(unsigned huid) override { return nullptr; }

private:
  static constexpr const char *serviceName = "webui";
  static constexpr const char *friendlyServiceName = nullptr; //"(srv) WebUI Service"; // hidden service

  bool isVisible = false;
};

void init_webui_service() { IDaEditor3Engine::get().registerService(new (inimem) WebUIAdapter); }
