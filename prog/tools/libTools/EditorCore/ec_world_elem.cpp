#include "ec_cachedRender.h"
#include "ec_stat3d.h"

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_camera_elem.h>

#include <3d/dag_drv3d_pc.h>
#include <3d/dag_render.h>
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
#include <gui/dag_stdGuiRender.h>

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
  vf.set(v_ldu(&::grs_cur_view.pos.x), f, 0, 0, 1, p.hk, true);
}

void update_visibility_finder()
{
  if (visibility_finder)
    update_visibility_finder(*visibility_finder);
}

class EditorCoreScene : public DagorGameScene
{
public:
  bool clear;

public:
  EditorCoreScene()
  {
    clear = true;

    DataBlock gameParamsBlk;
    PostFxUserSettings globalSettings;
    globalSettings.shouldTakeStateFromBlk = true;
    globalSettings.hdrInstantAdaptation = true;
    postFx = new PostFx(gameParamsBlk, gameParamsBlk, globalSettings);
  }


  ~EditorCoreScene() { delete postFx; }


  void beforeRender()
  {
    if (!IEditorCoreEngine::get())
      return;
    IEditorCoreEngine::get()->beforeRenderObjects();

    update_visibility_finder();
  }

  void render()
  {
    if (!IEditorCoreEngine::get())
      return;

    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(64, 64, 64, 0), 0, 0);

    d3d::settm(TM_WORLD, TMatrix::IDENT);
    update_visibility_finder();
    IEditorCoreEngine::get()->renderObjects();

    ec_camera_elem::freeCameraElem->render();
    ec_camera_elem::maxCameraElem->render();
    ec_camera_elem::fpsCameraElem->render();
    ec_camera_elem::tpsCameraElem->render();
    ec_camera_elem::carCameraElem->render();
  }

  void renderTrans()
  {
    if (IEditorCoreEngine::get())
      IEditorCoreEngine::get()->renderTransObjects();

    ::flush_buffered_debug_lines();
  }

  virtual void drawScene()
  {
    if (!IEditorCoreEngine::get())
      return;
    if (!ec_cached_viewports)
      return;
    if (ec_cached_viewports->getCurRenderVp() != -1)
      return;

    for (int i = ec_cached_viewports->viewportsCount() - 1; i >= 0; i--)
    {
      if (!ec_cached_viewports->startRender(i))
        continue;
      ViewportWindow *vpw = (ViewportWindow *)ec_cached_viewports->getViewportUserData(i);
      renderViewportFrame(vpw);
      ec_cached_viewports->endRender();
      if (i > 0)
      {
        d3d::update_screen();
      }
    }
  }

  void renderViewportFrame(ViewportWindow *vpw)
  {
    Driver3dRenderTarget rt;
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;
    int targetW, targetH;

    d3d::get_render_target(rt);
    d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    d3d::get_target_size(targetW, targetH);

    d3d::setview(0, 0, targetW, targetH, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 1, 0);
    d3d::setview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);

    if (vpw->needStat3d())
    {
      ec_stat3d_wait_frame_end(true);
    }

    // if (clear)
    {
      E3DCOLOR fcolor(64, 6, 6, 0);
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, fcolor, 0, 0);
    }

    beforeRender();
    if (hdr_render_mode != HDR_MODE_NONE)
    {
      Driver3dRenderTarget rt;
      d3d::get_render_target(rt);
      // G_ASSERT(!rt.tex || rt.tex->restype() == RES3D_TEX);

      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, 0, 0, 0);

      render();
      renderTrans();

      d3d_err(d3d::set_render_target(rt));
    }
    else
    {
      if (IEditorCoreEngine::get())
      {
        E3DCOLOR fcolor(64, 64, 64, 0);
        d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, fcolor, 0, 0);
      }

      render();
      renderTrans();
    }

    if (vpw->needStat3d())
    {
      ec_stat3d_wait_frame_end(false);
    }

    d3d::setwire(0);
    vpw->drawStat3d();
    vpw->paint(viewportW, viewportH);
    d3d::setwire(::grs_draw_wire);

    d3d::pcwin32::set_present_wnd(vpw->getHandle());
  }

  void restartPostfx(const DataBlock &game_params, bool allow_isl_creation)
  {
    postFx->restart(&game_params, &game_params, NULL);

    bool isIslEnabled = game_params.getBool("useIsl", false);
  }


  virtual void beforeDrawScene(int realtime_elapsed_usec, float /*gametime_elapsed_sec*/)
  {
    ::advance_shader_global_time(realtime_elapsed_usec * 1e-6f * ::dagor_game_time_scale);
  }
  virtual void actScene()
  {
    if (!IEditorCoreEngine::get())
      return;
    ec_camera_elem::freeCameraElem->act();
    ec_camera_elem::maxCameraElem->act();
    ec_camera_elem::fpsCameraElem->act();
    ec_camera_elem::tpsCameraElem->act();
    ec_camera_elem::carCameraElem->act();
    IEditorCoreEngine::get()->actObjects(::dagor_game_act_time * ::dagor_game_time_scale);

    postFx->update(::dagor_game_act_time * ::dagor_game_time_scale);
  }


private:
  PostFx *postFx;
};


static InitOnDemand<EditorCoreScene> edScene;

static void render_viewport_frame(ViewportWindow *vpw)
{
  if (::edScene)
    ::edScene->renderViewportFrame(vpw);
}

void startup_editor_core_world_elem(bool do_clear_on_render)
{
  ::edScene.demandInit();
  ::edScene->clear = do_clear_on_render;
  ec_camera_elem::freeCameraElem.demandInit();
  ec_camera_elem::maxCameraElem.demandInit();
  ec_camera_elem::fpsCameraElem.demandInit();
  ec_camera_elem::tpsCameraElem.demandInit();
  ec_camera_elem::carCameraElem.demandInit();
  ViewportWindow::render_viewport_frame = &render_viewport_frame;
  ec_init_stat3d();

  dagor_select_game_scene(::edScene);
}
