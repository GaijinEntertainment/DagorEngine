// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/webui.h"
#include <de3_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

class WebUIAdapter : public IEditorService
{
public:
  WebUIAdapter()
  {
    constexpr int webUIPort = 23457;
    constexpr int webSocketPort = 9113;
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setBool("enableWebSocketStream", true);
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setInt("WebUIPort", webUIPort);
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("debug")->setInt("WebSocketPort", webSocketPort);

    init_webui();
  }

  ~WebUIAdapter() override { stop_webui(); }

  const char *getServiceName() const override { return serviceName; };
  const char *getServiceFriendlyName() const override { return friendlyServiceName; };

  void setServiceVisible(bool vis) override { isVisible = true; };
  bool getServiceVisible() const override { return isVisible; };

  void actService(float dt) override { update_webui(); }

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
