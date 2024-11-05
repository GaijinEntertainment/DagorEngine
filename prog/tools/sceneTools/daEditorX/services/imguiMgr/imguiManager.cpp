// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <ecs/input/input.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <imgui/imguiInput.h>
#include <EditorCore/ec_window.h>
#include <windows.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <sepGui/wndPublic.h>

ECS_BROADCAST_EVENT_TYPE(ImGuiStage);
ECS_REGISTER_EVENT(ImGuiStage)


// TODO: ImGui porting: this was used for the VR view in Asset Viewer. It has been vastly simplied, and the most of it
// can be removed. It should be renamed at least.
class ImGuiManager : public IEditorService, public IRenderingService
{
public:
  ImGuiManager()
  {
    if (!IDaEditor3Engine::get().registerService(this))
    {
      logerr("Failed to register ImGui Manager service.");
      return;
    }

    ecs::init_hid_drivers(1);
    ecs::init_input(""); // empty config
  }

  ~ImGuiManager()
  {
    // Do not show the VR controls window at the next application start.
    if (imgui_window_is_visible("VR", "VR controls"))
      imgui_window_set_visible("VR", "VR controls", false);
  }

  virtual const char *getServiceName() const override { return "ImGuiManager"; }
  virtual const char *getServiceFriendlyName() const override { return nullptr; }

  virtual void setServiceVisible(bool vis) override {}
  virtual bool getServiceVisible() const override { return true; }

  virtual void actService(float dt) override {}

  virtual void beforeRenderService() override {}
  virtual void renderService() override {}
  virtual void renderTransService() override {}

  virtual void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IRenderingService);
    return nullptr;
  }
  virtual void renderGeometry(Stage stage) {}

  virtual void renderUI() override
  {
    TIME_PROFILE(ImGuiStage)
    g_entity_mgr->broadcastEventImmediate(ImGuiStage());
  }
};

eastl::unique_ptr<ImGuiManager> imguiManager;

void init_imgui_mgr_service(const DataBlock &app_blk)
{
  if (app_blk.getBool("initImGui", false))
    imguiManager = eastl::make_unique<ImGuiManager>();
}