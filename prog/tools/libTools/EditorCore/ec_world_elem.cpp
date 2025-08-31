// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_cachedRender.h"
#include "ec_stat3d.h"

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_camera_elem.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_wndPublic.h>

#include <coolConsole/coolConsole.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_platform_pc.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_info.h>
#include <render/dag_cur_view.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <libTools/renderViewports/renderViewport.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <fx/dag_hdrRender.h>
#include <render/fx/dag_postfx.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_renderScene.h>
#include <perfMon/dag_graphStat.h>
#include <debug/dag_debug3d.h>
#include <gui/dag_imgui.h>
#include <gui/dag_stdGuiRender.h>
#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/messageQueue.h>
#include <propPanel/propPanel.h>

#include <imgui/imgui.h>

void update_visibility_finder(VisibilityFinder &vf) // legacy
{
  mat44f viewMatrix4;
  d3d::gettm(TM_VIEW, viewMatrix4);
  mat44f projTm4;
  d3d::gettm(TM_PROJ, projTm4);
  Driver3dPerspective p(1.3f, 2.3f, 1.f, 10000.f, 0.f, 0.f);

  mat44f pv;
  v_mat44_mul(pv, projTm4, viewMatrix4); // Avoid floating point errors from v_mat44_orthonormalize33.
  Frustum f;
  f.construct(pv);
  d3d::getpersp(p);
  vf.set(v_ldu(&::grs_cur_view.pos.x), f, 0, 0, 1, p.hk, current_occlusion);
}


class EditorStartupScene : public DagorGameScene
{
public:
  void actScene() override { PropPanel::process_message_queue(); }

  void drawScene() override
  {
    if (!ImGui::GetCurrentContext())
      return;

    IWndManager *wndManager = EDITORCORE->getWndManager();
    G_ASSERT_RETURN(wndManager, );

    wndManager->updateImguiMouseCursor();

    unsigned clientWidth = 0;
    unsigned clientHeight = 0;
    wndManager->getWindowClientSize(wndManager->getMainWindow(), clientWidth, clientHeight);

    // They can be zero when toggling the application's window between minimized and maximized state.
    if (clientWidth == 0 || clientHeight == 0)
      return;

    imgui_update(clientWidth, clientHeight);
    PropPanel::after_new_frame();

    if (EDITORCORE->getConsole().isVisible())
    {
      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);

      ImGui::Begin("Console###startupSceneConsole", nullptr,
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
      EDITORCORE->getConsole().updateImgui();
      ImGui::End();
    }

    PropPanel::render_dialogs();

    PropPanel::before_end_frame();
    imgui_endframe();

    {
      d3d::GpuAutoLock gpuLock;

      d3d::set_render_target();

      const ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
      d3d::clearview(CLEAR_TARGET, e3dcolor(Color4(color.x, color.y, color.z, color.w)), 0, 0);

      imgui_render();

#if _TARGET_PC_WIN
      d3d::pcwin::set_present_wnd(wndManager->getMainWindow());
#endif
    }
  }

  void sceneDeselected(DagorGameScene * /*new_scene*/) override { delete this; }
};

void startup_editor_core_force_startup_scene_draw()
{
  d3d::GpuAutoLock gpuLock;

  dagor_work_cycle_flush_pending_frame();
  dagor_draw_scene_and_gui(true, true);
  d3d::update_screen();
}

void startup_editor_core_select_startup_scene()
{
  dagor_select_game_scene(new EditorStartupScene());
  dagor_reset_spent_work_time();
  startup_editor_core_force_startup_scene_draw();
}
