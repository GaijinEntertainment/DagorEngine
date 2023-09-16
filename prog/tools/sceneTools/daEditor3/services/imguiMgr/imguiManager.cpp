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

ECS_BROADCAST_EVENT_TYPE(ImGuiStage);
ECS_REGISTER_EVENT(ImGuiStage)


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
    imgui_switch_state(); // from disable to enable

    ecs::init_hid_drivers(1);
    ecs::init_input(""); // empty config

    imgui_update(); // invoke init on demand
    imgui_endframe();
    imgui_draw_mouse_cursor(false);
    ImGui::GetIO().MouseDrawCursor = false;
  }

  ~ImGuiManager() { imgui_shutdown(); }

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
    // here we try to manage with not matched vieport space and application space.
    // convert (0, 0) pixel of viewport to application space, add title bar height and 3 pixel (don't know why exactly 3)
    int x = 0, y = 0;
    auto viewPort = IEditorCoreEngine::get()->getCurrentViewport();
    viewPort->screenToClient(x, y);
    TITLEBARINFO tbi;
    GetTitleBarInfo((HWND)viewPort->getEcWindow()->getHandle(), &tbi);
    int titleBarHeight = ::GetSystemMetrics(SM_CYCAPTION);
    const int MOUSE_CURSOR_OFFSET = 3;
    imgui_set_viewport_offset(x + MOUSE_CURSOR_OFFSET, y + titleBarHeight + MOUSE_CURSOR_OFFSET);

    // update imgui and render it
    imgui_update();
    {
      TIME_PROFILE(ImGuiStage)
      g_entity_mgr->broadcastEventImmediate(ImGuiStage());
    }
    imgui_endframe();
    {
      TIME_PROFILE(imgui_render)
      imgui_render();
    }
  }
};

eastl::unique_ptr<ImGuiManager> imguiManager;

void init_imgui_mgr_service(const DataBlock &app_blk)
{
  if (app_blk.getBool("initImGui", false))
    imguiManager = eastl::make_unique<ImGuiManager>();
}