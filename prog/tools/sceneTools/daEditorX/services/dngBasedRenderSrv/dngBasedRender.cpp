// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_dynRenderService.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_camera_elem.h>
#include <EditorCore/ec_wndPublic.h>
#include <libTools/renderViewports/cachedViewports.h>
#include <dafxToolsHelper/dafxToolsHelper.h>

#include <render/dag_cur_view.h>
#include <render/fx/dag_demonPostFx.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_startupTex.h>
#include <workCycle/dag_gameScene.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_renderStateId.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_renderStates.h>
#include <math/dag_viewMatrix.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <gui/dag_imgui.h>
#include <gui/dag_stdGuiRender.h>
#include <gui/dag_visualLog.h>
#include <imgui/imgui.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_basePath.h>
#include <debug/dag_debug.h>

#include "main/settings.h"
#include "main/gameProjConfig.h"
#include "main/gameLoad.h"
#include "main/level.h"
#include "main/webui.h"
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "game/gameScripts.h"
#include "game/riDestr.h"
#include "render/animatedSplashScreen.h"
#include "render/renderer.h"
#include "render/renderEvent.h"
#include "render/skies.h"
#include "render/world/dynModelRenderer.h"
#include "camera/sceneCam.h"

#include <daGame/timers.h>
#include <ecs/core/entityManager.h>
#include <ecs/scripts/scripts.h>
#include <ecs/delayedAct/actInThread.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <ecs/weather/skiesSettings.h>
#include <render/hdrRender.h>
#include <render/tdrGpu.h>
#include <rendInst/riexSync.h>
#include <rendInst/rendInstGen.h>
#include <folders/folders.h>
#include <dasModules/dasSystem.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <gameRes/dag_stdGameRes.h>
#include <util/dag_threadPool.h>

#if _TARGET_PC_WIN
#include <windows.h>
#endif

extern CachedRenderViewports *ec_cached_viewports;
extern void ec_init_stat3d();
extern void ec_stat3d_wait_frame_end(bool frame_start);
extern ShaderBlockIdHolder dynamicSceneBlockId;

namespace dng_based_render
{
extern Tab<IRenderingService *> rendSrv;

static SimpleString main_vromfs_fpath_str, main_vromfs_mount_dir_str;
static SimpleString dng_scene_fname, dng_template_fname, dng_game_params_fname;
static IPoint2 pendingRes = {0, 0};
static bool empty_world_created = false;
static int last_dt_realtime_usec = 0;
static UniqueTex nullTarget;
static TexStreamingContext currentTexCtx = {0};
static DataBlock riGenExtraConfig;

static void init_dng_framework();

static void act_scene();
static bool scene_ready_for_render();
static void before_draw_scene(int dt_realtime_usec, float dt_gametime_sec, float zn, float zf, float fov);
static const ManagedTex &get_final_target();
static void draw_scene();
static void after_draw_scene();
}; // namespace dng_based_render

using dng_based_render::rendSrv;

static bool render_enabled = false;
static float wireframeZBias = 0.1;
static bool enable_wireframe = false;

// Starting with draw skipping by default. This way we ensure that before the first drawScene call actScene was called
static bool skip_next_frame = true;

static DataBlock app_ecs_blk;

class DngBasedRenderScene : public DagorGameScene
{
public:
  int dbgShowType = -1;
  shaders::RenderStateId defaultRenderStateId;
  shaders::RenderStateId alphaWriterRenderStateId;
  shaders::RenderStateId depthMaskRenderStateId;

public:
  DngBasedRenderScene()
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::CULL_NONE);

    defaultRenderStateId = shaders::render_states::create(shaders::RenderState());

    shaders::RenderState rs;
    rs.cull = CULL_NONE;
    rs.colorWr = WRITEMASK_ALPHA;
    rs.zFunc = CMPF_GREATEREQUAL;
    rs.zwrite = 0;
    depthMaskRenderStateId = shaders::render_states::create(rs);

    rs.zFunc = CMPF_ALWAYS;
    alphaWriterRenderStateId = shaders::render_states::create(rs);
  }

  ~DngBasedRenderScene() override { shutdown(); }

  void actScene() override
  {
    if (!IEditorCoreEngine::get())
      return;
    dng_based_render::act_scene();
    ec_camera_elem::freeCameraElem->act();
    ec_camera_elem::maxCameraElem->act();
    ec_camera_elem::fpsCameraElem->act();
    ec_camera_elem::tpsCameraElem->act();
    ec_camera_elem::carCameraElem->act();
    IEditorCoreEngine::get()->actObjects(::dagor_game_act_time * ::dagor_game_time_scale);
    if (dgs_app_active && CCameraElem::getCamera() != CCameraElem::MAX_CAMERA)
    {
      ViewportWindow *vpw = (ViewportWindow *)EDITORCORE->getCurrentViewport();
      if (!vpw)
        CCameraElem::switchCamera(false, false);
    }

    // do not waste resources when app is minimized
#if _TARGET_PC_WIN
    HWND wnd = (HWND)EDITORCORE->getWndManager()->getMainWindow();
    skip_next_frame = IsIconic(wnd);
    if (skip_next_frame)
    {
      int64_t reft = ref_time_ticks_qpc();
      debug("idle in minimized mode");
      while (IsIconic(wnd) && get_time_usec_qpc(reft) < 10000000)
      {
        sleep_msec(10);
        dagor_idle_cycle();
      }
      skip_next_frame = IsIconic(wnd);
    }
#else
    // TODO: tools Linux porting: IsIconic
    skip_next_frame = false;
#endif
  }

  void gatherRendSrv()
  {
    rendSrv.clear();
    for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
    {
      IGenEditorPluginBase *p = EDITORCORE->getPluginBase(i);
      if (p->getVisible())
        if (IRenderingService *iface = p->queryInterface<IRenderingService>())
          rendSrv.push_back(iface);
    }
  }

  void beforeDrawScene(int dt_realtime_usec, float dt_gametime_sec) override
  {
    d3d::set_render_target();
    ::advance_shader_global_time(dt_realtime_usec * 1e-6f);

    if (ec_cached_viewports->getCurRenderVp() != -1)
      return;
    if (skip_next_frame)
      return;

    float zn, zf, fov;
    getViewportRenderParams(zn, zf, fov);

    if (render_enabled)
      dng_based_render::before_draw_scene(dt_realtime_usec, dt_gametime_sec, zn, zf, fov);
    IEditorCoreEngine::get()->beforeRenderObjects();
    d3d::set_render_target();
  }

  void drawScene() override
  {
    if (skip_next_frame)
      return;
    if (!IEditorCoreEngine::get() || !ec_cached_viewports || !render_enabled)
      return;
    if (ec_cached_viewports->getCurRenderVp() != -1)
      return;

    gatherRendSrv();

    for (int i = ec_cached_viewports->viewportsCount() - 1; i >= 0; i--)
    {
      ViewportWindow *vpw = (ViewportWindow *)ec_cached_viewports->getViewportUserData(i);
      if (!vpw || !vpw->isViewportTextureReady())
        continue;

      if (!ec_cached_viewports->startRender(i))
      {
        if (!ec_cached_viewports->getViewport(i))
          logwarn("ec_cached_viewports->startRender(%d) returns false, getViewport(%d)=%p", i, i, ec_cached_viewports->getViewport(i));
        continue;
      }

      renderViewportFrame(vpw);
      ec_cached_viewports->endRender();
    }
    dng_based_render::after_draw_scene();

#if _TARGET_PC_WIN
    HWND hwnd = (HWND)EDITORCORE->getWndManager()->getMainWindow();
    RECT rect;
    GetClientRect(hwnd, &rect);
    const int clientRectWidth = rect.right - rect.left;
    const int clientRectHeight = rect.bottom - rect.top;
#else
    int clientRectWidth = 0;
    int clientRectHeight = 0;
    d3d::get_screen_size(clientRectWidth, clientRectHeight);
#endif

    if (::grs_draw_wire)
      d3d::setwire(0);

    d3d::set_render_target();
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 0, 0);
    renderUI(clientRectWidth, clientRectHeight);

#if _TARGET_PC_WIN // TODO: tools Linux porting: check_and_restore_3d_device
    // When toggling the application's window between minimized and maximized state the client size could be 0.
    // This is here to avoid the assert in d3d::stretch_rect.
    if (clientRectWidth <= 0 || clientRectHeight <= 0)
      hwnd = nullptr;

    if (check_and_restore_3d_device())
      d3d::pcwin::set_present_wnd(hwnd);
#else
    check_and_restore_3d_device();
#endif
  }

  static void getViewportRenderParams(float &zn, float &zf, float &fov)
  {
    zn = 0.1f, zf = 1000.f, fov = 90.f;
    if (auto *vpw = (ViewportWindow *)ec_cached_viewports->getViewportUserData(0))
      vpw->getZnearZfar(zn, zf), fov = RadToDeg(vpw->getFov());
  }

  BaseTexture *getRenderBuffer() { return dng_based_render::get_final_target().getTex2D(); }
  D3DRESID getRenderBufferId() { return dng_based_render::get_final_target().getTexId(); }

  void renderViewportFrame(ViewportWindow *vpw, bool cached_render = false)
  {
    G_ASSERT(ec_cached_viewports->getCurRenderVp() >= 0);
    auto empty_render = []() {
      int target_w, target_h;
      d3d::get_target_size(target_w, target_h);
      d3d::setview(0, 0, target_w, target_h, 0, 1);
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(10, 10, 64, 0), 0, 0);
    };

    if (!render_enabled)
      return empty_render();

    Driver3dRenderTarget rt;
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;

    d3d::get_render_target(rt);
    d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    viewportW = (viewportW + 3) & ~3;
    viewportH = (viewportH + 3) & ~3;

    if (!cached_render || !getRenderBuffer() || viewportW != targetW || viewportH != targetH)
    {
      cached_render = false;

      updateBackBufSize(viewportW, viewportH);
      if (!getRenderBuffer())
        return empty_render();

      // G_ASSERT(!rt.tex);
      G_ASSERT(rt.isBackBufferColor());

      if (vpw->needStat3d() && dgs_app_active)
        ec_stat3d_wait_frame_end(true);

      enable_wireframe = vpw->wireframeOverlayEnabled();

      if (dng_based_render::scene_ready_for_render())
        beforeRender();
      d3d::set_render_target(getRenderBuffer(), 0);
      d3d::setview(0, 0, viewportW, viewportH, 0, 1);
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(64, 64, 64, 0), 0, 0);

      dng_based_render::draw_scene();
    }

    Texture *finalRt = getRenderBuffer();

    if (vpw->needStat3d() && dgs_app_active && !cached_render)
      ec_stat3d_wait_frame_end(false);

    if (::grs_draw_wire)
      d3d::setwire(0);

    RectInt rdst;
    rdst.left = viewportX;
    rdst.top = viewportY;
    rdst.right = viewportX + viewportW;
    rdst.bottom = viewportY + viewportH;
    d3d::set_srgb_backbuffer_write(srgb_backbuf_wr);
    d3d_err(d3d::stretch_rect(finalRt, rt.getColor(0).tex, NULL, &rdst));
    d3d::set_srgb_backbuffer_write(false);

    d3d_err(d3d::set_render_target(rt));
    d3d::setview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);

    { // War Thunder fills the alpha channel. We need to make it opaque, otherwise it will be wrong when ImGui renders it.
      TIME_D3D_PROFILE(setAlphaToOpaque);

      shaders::overrides::reset();
      d3d::set_program(d3d::get_debug_program());
      d3d::set_vs_const(0, &TMatrix4_vec4::IDENT, 4);

      struct Vertex
      {
        Point3 p;
        E3DCOLOR c;
      };
      static Vertex v[4];
      v[0].p.set(-1, -1, 0);
      v[1].p.set(+1, -1, 0);
      v[2].p.set(-1, +1, 0);
      v[3].p.set(+1, +1, 0);

      shaders::render_states::set(alphaWriterRenderStateId);
      v[0].c = v[1].c = v[2].c = v[3].c = 0xFFFFFFFF;
      d3d::draw_up(PRIM_TRISTRIP, 2, v, sizeof(v[0]));

      d3d::set_program(BAD_PROGRAM);
      shaders::overrides::reset();
      shaders::render_states::set(defaultRenderStateId);
    }

    ec_cached_viewports->endRender();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    vpw->drawStat3d();
    vpw->paint(viewportW, viewportH);
    vpw->copyTextureToViewportTexture(*rt.getColor(0).tex, viewportW, viewportH);

    d3d::setwire(::grs_draw_wire);

    if (auto *hlp = EDITORCORE->queryEditorInterface<IRenderHelperService>())
      hlp->dumpPendingProfilerDumpOnFrameEnd();
    dagor_frame_no_increment();
  }

  void renderScreenShot()
  {
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;
    Driver3dRenderTarget rt;

    d3d::get_render_target(rt);
    d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    updateBackBufSize(viewportW, viewportH);

    float zn, zf, fov;
    getViewportRenderParams(zn, zf, fov);
    dng_based_render::before_draw_scene(1, 1e-6f, zn, zf, fov);
    beforeRender();
    d3d::set_render_target(getRenderBuffer(), 0);
    d3d::setview(0, 0, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(64, 64, 64, 0), 0, 0);
    dng_based_render::draw_scene();

    Texture *finalRt = getRenderBuffer();

    RectInt r;
    r.left = viewportX;
    r.top = viewportY;
    r.right = viewportX + viewportW;
    r.bottom = viewportY + viewportH;
    d3d::set_srgb_backbuffer_write(srgb_backbuf_wr);
    d3d::stretch_rect(finalRt, rt.getColor(0).tex, NULL, &r);
    d3d::set_srgb_backbuffer_write(false);

    // fill alfa mask using depthBuf
    {
      d3d::set_render_target(rt.getColor(0).tex, 0);
      d3d::setview(0, 0, viewportW, viewportH, viewportMinZ, viewportMaxZ);

      shaders::overrides::reset();
      d3d::set_program(d3d::get_debug_program());
      d3d::set_vs_const(0, &TMatrix4_vec4::IDENT, 4);

      struct Vertex
      {
        Point3 p;
        E3DCOLOR c;
      };
      static Vertex v[4];
      v[0].p.set(-1, -1, 0);
      v[1].p.set(+1, -1, 0);
      v[2].p.set(-1, +1, 0);
      v[3].p.set(+1, +1, 0);

      shaders::render_states::set(alphaWriterRenderStateId);
      v[0].c = v[1].c = v[2].c = v[3].c = 0xFFFFFFFF;
      d3d::draw_up(PRIM_TRISTRIP, 2, v, sizeof(v[0]));

      shaders::render_states::set(depthMaskRenderStateId);
      v[0].c = v[1].c = v[2].c = v[3].c = 0x00FFFFFF;
      d3d::draw_up(PRIM_TRISTRIP, 2, v, sizeof(v[0]));

      d3d::set_program(BAD_PROGRAM);
      shaders::overrides::reset();
      shaders::render_states::set(defaultRenderStateId);
    }

    d3d_err(d3d::set_render_target(rt));
    d3d::setview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
  }
  void beforeRender()
  {
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    IEditorCoreEngine::get()->beforeRenderObjects();
    TIME_D3D_PROFILE_NAME(render_vsm, "before_render");
    for (auto *srv : rendSrv)
      srv->renderGeometry(IRenderingService::STG_BEFORE_RENDER);
  }

  void renderUI(int client_rect_width, int client_rect_height)
  {
    TIME_D3D_PROFILE(imgui);

    // Maybe just add a updateImgui call to IEditorCoreEngine?
    IMainWindowImguiRenderingService *renderingService =
      IEditorCoreEngine::get()->queryEditorInterface<IMainWindowImguiRenderingService>();
    G_ASSERT_RETURN(renderingService, );

    renderingService->beforeUpdateImgui();
    imgui_update(client_rect_width, client_rect_height);
    renderingService->updateImgui();

    for (auto *srv : rendSrv)
      srv->renderUI();

    imgui_perform_registered(/*with_menu_bar = */ false);
    imgui_endframe();
    imgui_render();
  }

  void initDngBasedRender() { dng_based_render::init_dng_framework(); }

  void shutdown()
  {
    destroy_world_renderer();
    shaders::overrides::destroy(wireframeState);
    imgui_shutdown();
  }

  void updateBackBufSize(int w, int h)
  {
    if (!w || !h)
    {
      targetW = targetH = 0;
      return;
    }

    if (getRenderBuffer() && w == targetW && h == targetH)
      return;

    dng_based_render::pendingRes.set(targetW = w, targetH = h);
  }

private:
  UniqueTexHolder wireframeTex;
  shaders::OverrideStateId wireframeState;
  PostFxRenderer wireframeRenderer;
  int targetW = 0, targetH = 0;
  bool srgb_backbuf_wr = false;
};

class DngBasedRenderService : public IDynRenderService
{
public:
  DngBasedRenderScene *dynScene;
  Tab<const char *> dbgShowTypeNm;
  Tab<int> dbgShowTypeVal;
  int dbgShowType;
  UniqueTex nullTex;

  DngBasedRenderService() : dynScene(NULL)
  {
    dbgShowTypeNm.push_back("- RESULT -");
    dbgShowTypeVal.push_back(-1);
    dbgShowType = 0;
  }

  void setup(const char *app_dir, const DataBlock &appblk) override
  {
    dynScene = new DngBasedRenderScene;

    String base_game_dir(0, "%s/%s/", app_dir, appblk.getBlockByNameEx("game")->getStr("game_folder", ""));
    simplify_fname(base_game_dir);
    dd_add_base_path(base_game_dir);
    dng_based_render::main_vromfs_fpath_str = appblk.getBlockByNameEx("game")->getStr("main_vromfs", "");
    dng_based_render::main_vromfs_mount_dir_str = appblk.getBlockByNameEx("game")->getStr("main_vromfs_mnt", "");
    if (const char *scn = appblk.getBlockByNameEx("game")->getStr("scene", nullptr))
      dng_based_render::dng_scene_fname = String::mk_str_cat(app_dir, scn);
    if (const char *templ = appblk.getBlockByNameEx("game")->getStr("entities", nullptr))
      dng_based_render::dng_template_fname = String::mk_str_cat(app_dir, templ);
    if (const char *params = appblk.getBlockByNameEx("game")->getStr("params", nullptr))
      dng_based_render::dng_game_params_fname = String::mk_str_cat(app_dir, params);
    app_ecs_blk = *appblk.getBlockByNameEx("ecs");

    ec_camera_elem::freeCameraElem.demandInit();
    ec_camera_elem::maxCameraElem.demandInit();
    ec_camera_elem::fpsCameraElem.demandInit();
    ec_camera_elem::tpsCameraElem.demandInit();
    ec_camera_elem::carCameraElem.demandInit();
    ViewportWindow::render_viewport_frame = &render_viewport_frame;
  }

  void init() override
  {
    if (auto *hlp = EDITORCORE->queryEditorInterface<IRenderHelperService>())
      hlp->init();
    ec_init_stat3d();
    dynScene->initDngBasedRender();
  }
  void term() override
  {
    dagor_select_game_scene(nullptr);
    del_it(dynScene);
    if (auto *hlp = EDITORCORE->queryEditorInterface<IRenderHelperService>())
      hlp->term();
  }

  void enableRender(bool enable) override { render_enabled = enable; }

  void selectAsGameScene() override { dagor_select_game_scene(dynScene); }

  void setEnvironmentSnapshot(const char *blk_fn, bool render_cubetex_from_snapshot) override {}
  bool hasEnvironmentSnapshot() override { return false; }

  void restartPostfx(const DataBlock &game_params) override {}
  void onLightingSettingsChanged() override {}
  void reloadCharacterMicroDetails(const DataBlock &, const DataBlock &) override {}

  void beforeD3DReset(bool full_reset) override
  {
    if (auto *wr = get_world_renderer())
      wr->beforeDeviceReset(full_reset);
  }
  void afterD3DReset(bool full_reset) override
  {
    hdrrender::update_globals();
    if (auto *wr = get_world_renderer())
      wr->afterDeviceReset(full_reset);
  }

  TexStreamingContext getMainViewStreamingContext() override { return dng_based_render::currentTexCtx; }
  void renderViewportFrame(ViewportWindow *vpw) override
  {
    d3d::GpuAutoLock gpuLock;
    if (dynScene && vpw)
      dynScene->renderViewportFrame(vpw, true);
    else if (dynScene)
    {
      dagor_work_cycle_flush_pending_frame();
      dagor_draw_scene_and_gui(true, false);
      d3d::update_screen();
    }
    else
    {
      int targetW, targetH;
      d3d::get_target_size(targetW, targetH);
      d3d::setview(0, 0, targetW, targetH, 0, 1);
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(10, 10, 64, 0), 0, 0);
    }
  }
  void renderScreenshot() override
  {
    if (dynScene)
      dynScene->renderScreenShot();
  }

  dag::ConstSpan<int> getSupportedRenderTypes() const override { return {}; }
  int getRenderType() const override { return RTYPE_DNG_BASED; }
  bool setRenderType(int t) override { return t == RTYPE_DNG_BASED; }

  dag::ConstSpan<const char *> getDebugShowTypeNames() const override { return dbgShowTypeNm; }
  int getDebugShowType() const override { return dbgShowType; }
  bool setDebugShowType(int t) override
  {
    if (t < 0 || t >= dbgShowTypeNm.size())
      return false;
    dbgShowType = t;
    dynScene->dbgShowType = dbgShowTypeVal[t];
    return true;
  }

  const char *getRenderOptName(int ropt) override { return nullptr; }
  bool getRenderOptSupported(int ropt) override { return false; }
  void setRenderOptEnabled(int ropt, bool enable) override {}
  bool getRenderOptEnabled(int ropt) override { return false; }

  dag::ConstSpan<const char *> getShadowQualityNames() const override { return {}; }
  int getShadowQuality() const override { return 0; }
  void setShadowQuality(int q) override {}

  bool hasExposure() const override { return false; }
  void setExposure(float exposure) override {}
  float getExposure() override { return 1.0f; }

  void getPostFxSettings(DemonPostFxSettings &set) override { memset(&set, 0, sizeof(set)); }
  void setPostFxSettings(DemonPostFxSettings &set) override {}

  BaseTexture *getRenderBuffer() override { return dynScene ? dynScene->getRenderBuffer() : nullptr; }
  D3DRESID getRenderBufferId() override { return dynScene ? dynScene->getRenderBufferId() : BAD_TEXTUREID; }

  BaseTexture *getDepthBuffer() override { return nullptr; }
  D3DRESID getDepthBufferId() override { return BAD_TEXTUREID; }

  const ManagedTex &getDownsampledFarDepth() override { return nullTex; }
  void toggleVrMode() override {}

  void renderOneDynModelInstance(DynamicRenderableSceneInstance *sceneInstance, Stage stage, int *optional_inst_seed,
    bool raw_render) override
  {
    if (!raw_render)
    {
      carray<Point4, 5> params;
      mem_set_0(params);
      if (optional_inst_seed)
        memcpy(&params[4].x, optional_inst_seed, sizeof(*optional_inst_seed));

      dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
      int sm0 = ShaderMesh::STG_opaque, sm1 = ShaderMesh::STG_imm_decal;
      if (stage == Stage::STG_RENDER_DYNAMIC_TRANS)
        sm0 = sm1 = ShaderMesh::STG_trans;
      else if (stage == Stage::STG_RENDER_DYNAMIC_DISTORTION)
        sm0 = sm1 = ShaderMesh::STG_distortion;

      // TODO: use NULL for fallback instead
      auto additionalData = animchar_additional_data::prepare_fixed_space<AAD_RAW_INITIAL_TM__HASHVAL>(
        make_span_const(params.data(), optional_inst_seed ? params.size() : 1));

      state.process_animchar(sm0, sm1, sceneInstance, additionalData, true, nullptr, nullptr, 0, 0, false,
        dynmodel_renderer::RenderPriority::DEFAULT, nullptr, dng_based_render::currentTexCtx);

      state.prepareForRender();
      if (const auto *buffer = state.requestBuffer(dynmodel_renderer::BufferType::OTHER))
      {
        state.setVars(buffer->buffer.getBufId());
        SCENE_LAYER_GUARD(dynamicSceneBlockId);
        state.render(buffer->curOffset);
        return;
      }
    }

    if (stage == Stage::STG_RENDER_DYNAMIC_OPAQUE || stage == Stage::STG_RENDER_SHADOWS || stage == Stage::STG_RENDER_TO_CLIPMAP)
      sceneInstance->render();
    else if (stage == Stage::STG_RENDER_DYNAMIC_TRANS)
      sceneInstance->renderTrans();
    else if (stage == Stage::STG_RENDER_DYNAMIC_DISTORTION)
      sceneInstance->renderDistortion();
  }

  static void render_viewport_frame(ViewportWindow *vpw);
};

static DngBasedRenderService srv;
void *get_dng_based_render_service() { return &srv; }

void DngBasedRenderService::render_viewport_frame(ViewportWindow *vpw) { srv.renderViewportFrame(vpw); }


// internals of DNG to be used
extern void app_start(bool register_dagor_scene);
extern void init_device_reset();

namespace game_scene
{
extern double total_time;
extern bool parallel_logic_mode;
extern eastl::tuple<float, float, float> updateTime(); // [rtDt, dt, curTime]
} // namespace game_scene


// implementation of DNG-based update/render for tools
static void dng_update(float dt, float real_dt, float cur_time)
{
  using namespace game_scene;
  TIME_PROFILE(dng_update);
  total_time += dt;

  dump_periodic_gpu_info();

  g_entity_mgr->tick();
  riexsync::update();

  if (is_level_loaded() || dng_based_render::empty_world_created)
  {
    g_entity_mgr->update(ecs::UpdateStageInfoAct(dt, cur_time));
    ridestr::update(dt, calc_active_camera_globtm());
  }
  g_entity_mgr->broadcastEventImmediate(UpdateStageGameLogic(dt, cur_time));
  if (get_world_renderer() && !sceneload::is_load_in_progress())
  {
    TIME_PROFILE(get_world_renderer__update);
    get_world_renderer()->update(dt, real_dt, get_cam_itm());
  }
  g_entity_mgr->broadcastEventImmediate(EventOnGameUpdateAfterRenderer(dt, cur_time));

  sqeventbus::process_events(gamescripts::get_vm());
}

static void dng_create_world()
{
  debug("DNG: creating world renderer (due to missing 'level' entity)");
  auto *wr = create_world_renderer();
  dng_based_render::empty_world_created = true;

  wr->beforeLoadLevel(DataBlock::emptyBlock);
  wr->preloadLevelTextures(); // this will at least wait for character_micro_details arr-tex to be loaded
  wr->onLevelLoaded(DataBlock::emptyBlock);
  g_entity_mgr->broadcastEventImmediate(EventDoFinishLocationDataLoad());
  wr->onSceneLoaded(nullptr);

  load_daskies(DataBlock::emptyBlock, 6.f);
  g_entity_mgr->broadcastEventImmediate(EventSkiesLoaded{});
}

static void dng_based_render::act_scene()
{
  using namespace game_scene;
  if (is_animated_splash_screen_in_thread())
  {
    debug("[splash] auto stop, is_load_in_progress()=%d", sceneload::is_load_in_progress());
    stop_animated_splash_screen_in_thread();
  }
  if (sceneload::is_scene_switch_in_progress())
    return;
  if (!get_world_renderer() && !is_level_loaded())
    dng_create_world();
  else if (empty_world_created && is_level_loaded())
    empty_world_created = false;

  gamescripts::update_deferred();

  uint64_t startTime = profile_ref_ticks();
  if (gamescripts::reload_changed())
    visuallog::logmsg(String(0, "Some das scripts were reloaded in %dms", profile_time_usec(startTime) / 1000).c_str());

  gamescripts::collect_das_garbage();

  float rtDt, dt, curTime;
  eastl::tie(rtDt, dt, curTime) = updateTime();

  {
    TIME_PROFILE(game_timers_act);
    update_ecs_sq_timers(dt, rtDt);
    game::g_timers_mgr->act(dt);
  }
  dng_update(dt, rtDt, curTime);

#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
  if (ShaderGraphRecompiler *compiler = ShaderGraphRecompiler::getInstance())
    compiler->update(rtDt);
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT

  {
    TIME_PROFILE(visuallog_act);
    visuallog::act(rtDt);
  }
  acesfx::wait_fx_managers_update_job_done();
}

static void dng_based_render::before_draw_scene(int dt_realtime_usec, float dt_gametime_sec, float znear, float zfar, float fov)
{
  if (auto wr = get_world_renderer(); wr && pendingRes.x)
  {
    debug("update final target resolution: %dx%d", pendingRes.x, pendingRes.y);
    wr->overrideResolution(pendingRes);
    pendingRes.set(0, 0);
  }

  last_dt_realtime_usec = dt_realtime_usec;

  // this has to happen before setting constrained mt mode, because it calls tick between ECS entity deletion and creation
  update_delayed_weather_selection();

  G_ASSERT(!g_entity_mgr->isConstrainedMTMode());
  g_entity_mgr->setConstrainedMTMode(true);

  before_render_daskies();

  float rtDt = ::dagor_game_act_time;
  float scaledDt = rtDt; // * time_speed;

  if (auto wr = get_world_renderer(); wr && (is_level_loaded() || empty_world_created))
  {
    int w = 4, h = 4;
    d3d::get_render_target_size(w, h, wr->getFinalTargetTex().getTex2D());
    Driver3dPerspective curPersp = calc_camera_perspective(fov, EFM_HOR_PLUS, znear, zfar, w, h);
    TMatrix itm = ::grs_cur_view.itm;
    itm.setcol(0, normalize(itm.getcol(0)));
    itm.setcol(1, normalize(itm.getcol(1)));
    itm.setcol(2, normalize(itm.getcol(2)));
    dng_based_render::currentTexCtx = TexStreamingContext(curPersp, w);
    wr->beforeRender(scaledDt, rtDt, dt_realtime_usec * 1e-6, dt_gametime_sec, itm, DPoint3(::grs_cur_view.pos), curPersp);

    acesfx::wait_fx_managers_update_and_allow_accum_cmds();
    FxRenderTargetOverride fx_q = FX_RT_OVERRIDE_DEFAULT;
    if (dafx_helper_globals::particles_resolution_preview == 0)
      fx_q = FX_RT_OVERRIDE_LOWRES;
    else if (dafx_helper_globals::particles_resolution_preview == 2)
      fx_q = FX_RT_OVERRIDE_HIGHRES;
    if (acesfx::get_rt_override() != fx_q)
      acesfx::set_rt_override(fx_q);
    // set actual dafx context to be used in tools code
    dafx_helper_globals::ctx = acesfx::get_dafx_context();
    dafx_helper_globals::cull_id = acesfx::get_cull_id();
    dafx_helper_globals::cull_fom_id = acesfx::get_cull_fom_id();
  }
}

static const ManagedTex &dng_based_render::get_final_target()
{
  if (auto wr = get_world_renderer())
    return wr->getFinalTargetTex();
  return nullTarget;
}

static bool dng_based_render::scene_ready_for_render() { return get_world_renderer() && (is_level_loaded() || empty_world_created); }
static void dng_based_render::draw_scene()
{
  if (auto wr = get_world_renderer(); wr && (is_level_loaded() || empty_world_created))
  {
    wr->updateFinalTargetFrame();
    d3d::set_render_target(wr->getFinalTargetTex().getTex2D(), 0);
    bool prevAutoBlockFlag = ShaderGlobal::enableAutoBlockChange(false);
    wr->draw(0, last_dt_realtime_usec * 1e-6f);
    ShaderGlobal::enableAutoBlockChange(prevAutoBlockFlag);
  }
  d3d::set_render_target();

  g_entity_mgr->setConstrainedMTMode(false);
  G_ASSERT(!g_entity_mgr->isConstrainedMTMode());
}

static void dng_based_render::after_draw_scene()
{
  if (g_entity_mgr->isConstrainedMTMode())
    g_entity_mgr->setConstrainedMTMode(false);
}

const char *gameproj::main_vromfs_fpath() { return dng_based_render::main_vromfs_fpath_str; }
const char *gameproj::main_vromfs_mount_dir() { return dng_based_render::main_vromfs_mount_dir_str; }

static void dng_based_render::init_dng_framework()
{
  threadpool::shutdown();
  StdGuiRender::close_render();

  g_entity_mgr.demandInit(); // init it early to be able to use broadcast events for init
  folders::initialize(gameproj::game_codename());
  initial_load_settings();
  if (auto *config = const_cast<DataBlock *>(dgs_get_settings()))
  {
    config->setStr("scene", dng_scene_fname);
    config->setStr("entitiesPath", dng_template_fname);
    if (app_ecs_blk.paramCount() || app_ecs_blk.blockCount())
      config->addNewBlock(&app_ecs_blk, "ecs");
    config->setBool("disableMenu", true);
    config->setBool("skipSplashScreenAnimationInThread", true);
    config->addStr("esTag", "inside_tools");
    config->removeBlock("shadersWarmup");
  }

  hdrrender::init_globals(*::dgs_get_settings()->getBlockByNameEx("video"));
  das::daScriptEnvironment::ensure();
  init_webui();
  init_device_reset();

  ::register_fast_phys_gameres_factory();
  ::register_a2d_gameres_factory();
  ::register_animchar_gameres_factory();
  ::register_character_gameres_factory();
  ::register_effect_gameres_factory();
  ::register_any_vromfs_tex_create_factory("avif|png|jpg|tga|ddsx");
  ::register_lshader_gameres_factory();
  rendinst::register_land_gameres_factory();

  const char *rendinstDmgBlkFn = get_rendinst_dmg_blk_fn();
  if (dd_file_exists(rendinstDmgBlkFn))
    riGenExtraConfig.load(rendinstDmgBlkFn);
  else
    riGenExtraConfig.reset();
  rendinst::registerRIGenExtraConfig(&riGenExtraConfig);

  app_start(false);

  ::dagor_reset_spent_work_time();
  while (!dng_based_render::scene_ready_for_render())
  {
    dagor_work_cycle();
    dng_based_render::act_scene();
    sleep_msec(10);
  }

  if (auto *params = const_cast<DataBlock *>(dgs_get_game_params()); params && !dng_game_params_fname.empty())
  {
    DataBlock gameParamsCopy(dng_game_params_fname);
    merge_data_block(*params, gameParamsCopy);
  }
}

namespace webui
{
void init_dmviewer_plugin() {}
} // namespace webui

// Compatibility with DNG. Right now it doesn't impact anything here, so let's not complicate things.
bool is_initial_loading_complete() { return true; }
