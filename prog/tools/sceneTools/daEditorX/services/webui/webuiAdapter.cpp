#include <de3_interface.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/websocket/webSocketStream.h>
#include <startup/dag_globalSettings.h>

#include <EASTL/unique_ptr.h>

class WebUIAdapter : public IEditorService
{
public:
  WebUIAdapter()
  {
    constexpr int webSocketPort = 9113;
    if (!IDaEditor3Engine::get().registerService(this))
    {
      logerr("Failed to register ECS Manager service.");
    }
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setBool("enableWebSocketStream", true);
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setInt("WebSocketPort", webSocketPort);

    static webui::HttpPlugin http_plugins[] = {webui::profiler_http_plugin, webui::shader_http_plugin,
      webui::color_pipette_http_plugin, webui::colorpicker_http_plugin, webui::cookie_http_plugin, webui::ecsviewer_http_plugin, NULL};

    webui::plugin_lists[0] = webui::dagor_http_plugins;
    webui::plugin_lists[1] = webui::webview_http_plugins;
    webui::plugin_lists[2] = http_plugins;
    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.bindPort = 23457;
    webui::startup(&cfg);
    webui::init_ecsviewer_plugin();
    websocket::start(webSocketPort);
  }

  void update() { webui::update(); }

  ~WebUIAdapter()
  {
    webui::shutdown();
    websocket::stop();
  }

  virtual const char *getServiceName() const override { return serviceName; };
  virtual const char *getServiceFriendlyName() const override { return friendlyServiceName; };

  virtual void setServiceVisible(bool vis) override { isVisible = true; };
  virtual bool getServiceVisible() const override { return isVisible; };

  virtual void actService(float dt) override
  {
    webui::update();
    websocket::update();
  }

  virtual void beforeRenderService() override{};
  virtual void renderService() override{};
  virtual void renderTransService() override{};

  virtual void *queryInterfacePtr(unsigned huid) override { return nullptr; }

private:
  static constexpr const char *serviceName = "webui";
  static constexpr const char *friendlyServiceName = nullptr; //"(srv) WebUI Service"; // hidden service

  bool isVisible = false;
};

eastl::unique_ptr<WebUIAdapter> webuiAdapter;

void init_webui_service() { webuiAdapter = eastl::make_unique<WebUIAdapter>(); }