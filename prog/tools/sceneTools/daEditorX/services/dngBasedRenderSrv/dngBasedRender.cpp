// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_dynRenderService.h>
#include <de3_skiesService.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_camera_elem.h>
#include <EditorCore/ec_wndPublic.h>
#include <EditorCore/ec_shaders.h>
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
#include <3d/dag_gpuConfig.h>
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
#include "main/app.h"
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
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/scripts/scripts.h>
#include <daECS/delayedAct/actInThread.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <ecs/weather/skiesSettings.h>
#include <ecs/render/updateStageRender.h>
#include <render/hdrRender.h>
#include <render/tdrGpu.h>
#include <render/antialiasing.h>
#include <rendInst/riexSync.h>
#include <rendInst/rendInstGen.h>
#include <phys/physUtils.h>
#include <folders/folders.h>
#include <dasModules/dasSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <util/dag_threadPool.h>
#include <util/dag_console.h>
#include <util/dag_finally.h>
#include <3d/dag_resPtr.h>
#include <camera/camShakerEvents.h>

#if _TARGET_PC_WIN
#include <windows.h>
#endif

#if HAVE_DNGLIB_DM
#include <dm/damageEvents.h>
// required by ri_phys lib
ECS_REGISTER_EVENT(CmdRendinstDamage)
ECS_REGISTER_EVENT(EventShellExplosionShockWave)
#endif

namespace sqeventbus
{
void process_events(HSQUIRRELVM vm);
}

extern CachedRenderViewports *ec_cached_viewports;
extern void ec_init_stat3d();
extern void ec_stat3d_wait_frame_end(bool frame_start);
extern ShaderBlockIdHolder dynamicSceneBlockId;
extern ShaderBlockIdHolder dynamicSceneTransBlockId;
extern ShaderBlockIdHolder dynamicDepthSceneBlockId;
extern ShaderBlockIdHolder globalFrameBlockId;
extern void init_fx();

namespace dng_based_render
{
extern Tab<IRenderingService *> rendSrv;

static SimpleString main_vromfs_fpath_str, main_vromfs_mount_dir_str;
static SimpleString dng_scene_fname, dng_template_fname, dng_game_params_fname;
static IPoint2 pendingRes = {0, 0};
static bool empty_world_created = false;
static int last_dt_realtime_usec = 0;
static float last_gametime_sec = 0.f;
static UniqueTex nullTarget;
static TexStreamingContext currentTexCtx = {0};
static DataBlock riGenExtraConfig;

static void init_dng_framework();
static void term_dng_framework();

static void act_scene();
static bool scene_ready_for_render();
static void before_draw_scene(int dt_realtime_usec, float dt_gametime_sec);
static void before_render(const Driver3dPerspective &persp, const TMatrix4D &proj_tm, int viewport_width);
static const ManagedTex &get_final_target();
static void draw_scene();
static void after_draw_scene();
}; // namespace dng_based_render

using dng_based_render::rendSrv;

static bool render_enabled = false;
static float wireframeZBias = 0.1;
static bool enable_wireframe = false;
static bool prevIsOrtho = false;
static float saved_motion_blur_strength = -1.f;

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
  UniqueBuf coloredQuadVb;
  struct Vertex
  {
    Point3 p;
    E3DCOLOR c;
  };

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

    coloredQuadVb = dag::create_vb(sizeof(Vertex) * 4, SBCF_BIND_VERTEX | SBCF_DYNAMIC | SBCF_FRAMEMEM, "colored_quad_vb");
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

    auto *vpw = (ViewportWindow *)ec_cached_viewports->getViewportUserData(0);
    if (vpw && render_enabled)
      dng_based_render::before_draw_scene(dt_realtime_usec, dt_gametime_sec);

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

    int clientRectWidth = 0;
    int clientRectHeight = 0;
    d3d::get_screen_size(clientRectWidth, clientRectHeight);

    if (::grs_draw_wire)
      d3d::setwire(0);

    d3d::set_render_target();
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 0, 0);
    renderUI(clientRectWidth, clientRectHeight);
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
      // TODO: update Driver3dRenderTarget to support not null backbuffer texture
      G_ASSERT(rt.isColorUsed(0) && (!rt.getColor(0).tex || d3d::get_backbuffer_tex() == rt.getColor(0).tex));

      if (vpw->needStat3d() && dgs_app_active)
        ec_stat3d_wait_frame_end(true);

      if (enable_wireframe != vpw->wireframeOverlayEnabled())
      {
        enable_wireframe = vpw->wireframeOverlayEnabled();
        console::command(String(0, "render.show_wireframe_overlay %d", enable_wireframe ? 1 : 0));
      }

      if (dng_based_render::scene_ready_for_render())
        beforeRender(vpw->getViewTm(), vpw->getPerspective(), vpw->isOrthogonal(), vpw->getW());
      d3d::set_render_target(getRenderBuffer(), 0);
      if (getRenderBuffer() == rt.getColor(0).tex)
        d3d::setview(viewportX, viewportY, viewportW, viewportH, 0, 1);
      else
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
    if (finalRt != rt.getColor(0).tex)
      d3d_err(d3d::stretch_rect(finalRt, rt.getColor(0).tex, NULL, &rdst));
    d3d::set_srgb_backbuffer_write(false);

    d3d_err(d3d::set_render_target(rt));
    d3d::setview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);

    { // War Thunder fills the alpha channel. We need to make it opaque, otherwise it will be wrong when ImGui renders it.
      TIME_D3D_PROFILE(setAlphaToOpaque);

      shaders::overrides::reset();
      d3d::set_program(d3d::get_debug_program());
      d3d::set_vs_const(0, &TMatrix4_vec4::IDENT, 4);

      static Vertex v[4];
      v[0].p.set(-1, -1, 0);
      v[1].p.set(+1, -1, 0);
      v[2].p.set(-1, +1, 0);
      v[3].p.set(+1, +1, 0);

      shaders::render_states::set(alphaWriterRenderStateId);
      v[0].c = v[1].c = v[2].c = v[3].c = 0xFFFFFFFF;
      coloredQuadVb->updateData(0, sizeof(v), v, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
      d3d::setvsrc(0, coloredQuadVb.getBuf(), sizeof(Vertex));
      d3d::draw(PRIM_TRISTRIP, 0, 2);
      d3d::setvsrc(0, nullptr, 0);

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

  void renderScreenShot(const Driver3dPerspective &persp, bool isOrtho, [[maybe_unused]] bool force_disable_wireframe)
  {
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;
    Driver3dRenderTarget rt;

    d3d::get_render_target(rt);
    d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    updateBackBufSize(viewportW, viewportH);

    TMatrix viewTm;
    d3d::gettm(TM_VIEW, viewTm);

    dng_based_render::before_draw_scene(1, 1e-6f);
    beforeRender(viewTm, persp, isOrtho, viewportW);
    d3d::set_render_target(getRenderBuffer(), 0);
    d3d::setview(0, 0, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(64, 64, 64, 0), 0, 0);
    dng_based_render::draw_scene();
    dng_based_render::after_draw_scene();

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

      static Vertex v[4];
      v[0].p.set(-1, -1, 0);
      v[1].p.set(+1, -1, 0);
      v[2].p.set(-1, +1, 0);
      v[3].p.set(+1, +1, 0);

      shaders::render_states::set(alphaWriterRenderStateId);
      v[0].c = v[1].c = v[2].c = v[3].c = 0xFFFFFFFF;
      coloredQuadVb->updateData(0, sizeof(v), v, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
      d3d::setvsrc(0, coloredQuadVb.getBuf(), sizeof(Vertex));
      d3d::draw(PRIM_TRISTRIP, 0, 2);

      shaders::render_states::set(depthMaskRenderStateId);
      v[0].c = v[1].c = v[2].c = v[3].c = 0x00FFFFFF;
      coloredQuadVb->updateData(0, sizeof(v), v, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
      d3d::draw(PRIM_TRISTRIP, 0, 2);
      d3d::setvsrc(0, nullptr, 0);
      d3d::set_program(BAD_PROGRAM);
      shaders::overrides::reset();
      shaders::render_states::set(defaultRenderStateId);
    }

    d3d_err(d3d::set_render_target(rt));
    d3d::setview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
  }

  void beforeRender(const TMatrix &viewTm, const Driver3dPerspective &persp, bool isOrtho, int viewportWidth)
  {
    if (prevIsOrtho != isOrtho)
    {
      if (auto *daSkies = get_daskies())
        daSkies->setSolidColorMode(isOrtho, E3DCOLOR(0, 0, 0));

      if (auto settingsEid = g_entity_mgr->getSingletonEntity(ECS_HASH("render_settings")))
      {
        if (isOrtho)
        {
          saved_motion_blur_strength = g_entity_mgr->getOr(settingsEid, ECS_HASH("render_settings__motionBlurStrength"), -1.f);
          g_entity_mgr->set(settingsEid, ECS_HASH("render_settings__motionBlurStrength"), 0.f);
        }
        else if (saved_motion_blur_strength > 0.f)
          g_entity_mgr->set(settingsEid, ECS_HASH("render_settings__motionBlurStrength"), saved_motion_blur_strength);
      }

      prevIsOrtho = isOrtho;
    }

    TMatrix4D projTm = isOrtho ? matrix_ortho_lh_reverse(2 / persp.wk, 2 / persp.hk, persp.zn, persp.zf)
                               : dmatrix_perspective_reverse(persp.wk, persp.hk, persp.zn, persp.zf, persp.ox, persp.oy);

    ::grs_cur_view.tm = viewTm;
    ::grs_cur_view.itm = inverse(::grs_cur_view.tm);
    ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);
    dng_based_render::before_render(persp, projTm, viewportWidth);
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

    editor_core_render_dag_imgui_windows();
    imgui_endframe();
    imgui_render();
  }

  void initDngBasedRender() { dng_based_render::init_dng_framework(); }

  void shutdown()
  {
    dafx_helper_globals::ctx = {};
    dafx_helper_globals::cull_id = {};
    dafx_helper_globals::cull_fom_id = {};
    dng_based_render::term_dng_framework();
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
  DataBlock deferredBlk;
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

    String snapshot_dir;
    if (tools3d::get_snapshot_path(appblk, app_dir, snapshot_dir))
      if (alefind_t fs; dd_find_first(snapshot_dir + "*.das", 0, &fs))
        dd_set_named_mount_path("toolScripts", snapshot_dir);

    ec_camera_elem::freeCameraElem.demandInit();
    ec_camera_elem::maxCameraElem.demandInit();
    ec_camera_elem::fpsCameraElem.demandInit();
    ec_camera_elem::tpsCameraElem.demandInit();
    ec_camera_elem::carCameraElem.demandInit();
    ViewportWindow::render_viewport_frame = &render_viewport_frame;
    ViewportWindow::needsOrthoCameraAdjustment = true;

    if (const DataBlock *b = appblk.getBlockByName("dynamicDeferred"))
    {
      deferredBlk = *b;
      if (const DataBlock *b = deferredBlk.getBlockByName("dbgShow"))
        for (int i = 0; i < b->paramCount(); i++)
          if (b->getParamType(i) == b->TYPE_INT)
          {
            if (b->getInt(i) != -1)
            {
              dbgShowTypeNm.push_back(b->getParamName(i));
              dbgShowTypeVal.push_back(b->getInt(i));
            }
            else
              dbgShowTypeNm[0] = b->getParamName(i);
          }
    }
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

  void selectAsGameScene() override
  {
    ::dagor_reset_spent_work_time();
    while (!dng_based_render::scene_ready_for_render())
    {
      dagor_work_cycle();
      dng_based_render::act_scene();
      sleep_msec(10);
    }

    dagor_select_game_scene(dynScene);
  }

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
  void renderScreenshot(const Driver3dPerspective &persp, bool is_ortho, bool force_disable_wireframe = false) override
  {
    if (dynScene)
      dynScene->renderScreenShot(persp, is_ortho, force_disable_wireframe);
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

    static const eastl::hash_map<eastl::string_view, eastl::string_view> modeMap = {{"diffuse", "diffuseColor"},
      {"specular", "specularColor"}, {"normal", "normal"}, {"smoothness", "smoothness"}, {"base_color", "baseColor"},
      {"metallness", "metalness"}, {"material", "materialType"}, {"ssao", "ssao"}, {"ao", "ao"}, {"albedo_ao", "albedo_ao"},
      {"final_ao", "finalAO"}, {"preshadow", "preshadow"}, {"translucency", "translucency"}, {"depth", "depth"}, {"ssr", "ssr"},
      {"ssr_strength", "ssrStrength"}};

    String cmd("render.show_gbuffer");
    if (dbgShowTypeVal[t] >= 0)
      if (auto it = modeMap.find(dbgShowTypeNm[t]); it != modeMap.end())
        cmd.aprintf(0, " %s", it->second);
    console::command(cmd);

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

  static void render_dynmodel_state(dynmodel_renderer::DynModelRenderingState &state, dynmodel_renderer::BufferType buf_type,
    const TMatrix &vtm, int block)
  {
    state.prepareForRender();
    const dynmodel_renderer::DynamicBufferHolder *buffer = state.requestBuffer(buf_type);
    if (!buffer)
      return;

    d3d::settm(TM_VIEW, vtm);
    state.setVars(buffer->buffer.getBufId());
    FRAME_LAYER_GUARD(globalFrameBlockId);
    SCENE_LAYER_GUARD(block);
    state.render(buffer->curOffset);
  }

  void renderOneDynModelInstance(DynamicRenderableSceneInstance *sceneInstance, Stage stage, int *optional_inst_seed,
    bool raw_render) override
  {
    if (raw_render)
    {
      if (stage == Stage::STG_RENDER_DYNAMIC_OPAQUE || stage == Stage::STG_RENDER_SHADOWS || stage == Stage::STG_RENDER_TO_CLIPMAP)
        sceneInstance->render();
      else if (stage == Stage::STG_RENDER_DYNAMIC_TRANS)
        sceneInstance->renderTrans();
      else if (stage == Stage::STG_RENDER_DYNAMIC_DISTORTION)
        sceneInstance->renderDistortion();
      return;
    }

    carray<Point4, 5> params;
    mem_set_0(params);
    if (optional_inst_seed)
      memcpy(&params[4].x, optional_inst_seed, sizeof(*optional_inst_seed));

    dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
    int sm0 = ShaderMesh::STG_opaque, sm1 = ShaderMesh::STG_imm_decal;
    dynmodel_renderer::NeedPreviousMatrices needPrevMatrices = dynmodel_renderer::NeedPreviousMatrices::No;
    uint32_t renderMask = UpdateStageInfoRender::RENDER_MAIN;
    dynmodel_renderer::BufferType bufType = dynmodel_renderer::BufferType::OTHER;
    int blockId = dynamicSceneBlockId;

    switch (stage)
    {
      case Stage::STG_RENDER_DYNAMIC_OPAQUE:
        needPrevMatrices = dynmodel_renderer::NeedPreviousMatrices::Yes;
        bufType = dynmodel_renderer::BufferType::MAIN;
        break;
      case Stage::STG_RENDER_DYNAMIC_TRANS:
        sm0 = sm1 = ShaderMesh::STG_trans;
        bufType = dynmodel_renderer::BufferType::TRANSPARENT_MAIN;
        blockId = dynamicSceneTransBlockId;
        break;
      case Stage::STG_RENDER_DYNAMIC_DISTORTION:
        sm0 = sm1 = ShaderMesh::STG_distortion;
        bufType = dynmodel_renderer::BufferType::MAIN;
        break;
      case Stage::STG_RENDER_SHADOWS:
        renderMask = UpdateStageInfoRender::RENDER_SHADOW;
        bufType = dynmodel_renderer::BufferType::DYNAMIC_SHADOW;
        blockId = dynamicDepthSceneBlockId;
        break;
      case Stage::STG_RENDER_DYNAMIC_DECALS: sm0 = sm1 = ShaderMesh::STG_decal; break;
    }

    for (uint32_t i = 0; i < sceneInstance->getNodeCount(); ++i)
    {
      TMatrix wtm = sceneInstance->getNodeWtm(i);
      wtm.setcol(3, wtm.getcol(3) - ::grs_cur_view.pos);
      sceneInstance->setNodeWtm(i, wtm);
    }
    FINALLY([&] {
      for (uint32_t i = 0; i < sceneInstance->getNodeCount(); ++i)
      {
        TMatrix wtm = sceneInstance->getNodeWtm(i);
        wtm.setcol(3, wtm.getcol(3) + ::grs_cur_view.pos);
        sceneInstance->setNodeWtm(i, wtm);
      }
    });

    // TODO: use NULL for fallback instead
    auto additionalData = animchar_additional_data::prepare_fixed_space<AAD_RAW_INITIAL_TM__HASHVAL>(
      make_span_const(params.data(), optional_inst_seed ? params.size() : 1));

    state.process_animchar(sm0, sm1, sceneInstance, additionalData, needPrevMatrices, {},
      dynmodel_renderer::PathFilterView::NULL_FILTER, renderMask, dynmodel_renderer::RenderPriority::DEFAULT, nullptr,
      dng_based_render::currentTexCtx);

    if (state.empty())
      return;

    TMatrix oldViewTm;
    d3d::gettm(TM_VIEW, oldViewTm);
    TMatrix vtm = oldViewTm;
    vtm.setcol(3, 0, 0, 0);
    render_dynmodel_state(state, bufType, vtm, blockId);
    d3d::settm(TM_VIEW, oldViewTm);
  }

  static void render_viewport_frame(ViewportWindow *vpw);
};

static DngBasedRenderService srv;
void *get_dng_based_render_service() { return &srv; }

void DngBasedRenderService::render_viewport_frame(ViewportWindow *vpw) { srv.renderViewportFrame(vpw); }


// internals of DNG to be used
extern void app_start(bool register_dagor_scene);
extern void app_close();
extern void init_device_reset();

namespace game_scene
{
extern bool parallel_logic_mode;
extern eastl::tuple<float, float, double> updateTime(); // [rtDt, dt, curTime]
extern void on_scene_deselected();
} // namespace game_scene


// implementation of DNG-based update/render for tools
static void dng_update(float dt, float real_dt, float cur_time)
{
  using namespace game_scene;
  TIME_PROFILE(dng_update);

  dump_periodic_gpu_info();

  acesfx::wait_fx_managers_update_and_allow_accum_cmds();
  g_entity_mgr->tick();
  riexsync::update();

  if (is_level_loaded() || dng_based_render::empty_world_created)
  {
    phys_fetch_sim_res(true);
    g_entity_mgr->update(ecs::UpdateStageInfoAct(dt, cur_time));
    g_entity_mgr->broadcastEventImmediate(ParallelUpdateFrameDelayed(dt, cur_time));
    // TODO: problem with calling BulletPhysWorld::fetchSimRes from invalid thread
    // ridestr::update(dt, cur_time, calc_active_camera_globtm());
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

  float locTime = ISkiesService::DEFAULT_LOCALTIME;
  const DataBlock *weatherDesc = &DataBlock::emptyBlock;
  if (auto skiesSrv = EDITORCORE->queryEditorInterface<ISkiesService>())
  {
    locTime = skiesSrv->getAppliedWeatherDesc().getReal("locTime", locTime);
    weatherDesc = &skiesSrv->getAppliedWeatherDesc();
  }
  SkiesPanel skiesData;
  skiesData.time = locTime;
  load_daskies(*weatherDesc, skiesData);
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
  if (sceneload::is_scene_switch_in_progress() || !acesfx::get_dafx_context())
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

  set_timespeed(dagor_game_time_scale);
  float rtDt, dt;
  double curTime;
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
}

static void dng_based_render::before_draw_scene(int dt_realtime_usec, float dt_gametime_sec)
{
  if (auto wr = get_world_renderer(); wr && pendingRes.x)
  {
    debug("update final target resolution: %dx%d", pendingRes.x, pendingRes.y);
    wr->overrideResolution(pendingRes);
    pendingRes.set(0, 0);
  }

  last_dt_realtime_usec = dt_realtime_usec;
  last_gametime_sec += dt_gametime_sec;

  // this has to happen before setting constrained mt mode, because it calls tick between ECS entity deletion and creation
  update_delayed_weather_selection();

  G_ASSERT(!g_entity_mgr->isConstrainedMTMode());
  g_entity_mgr->setConstrainedMTMode(true);

  before_render_daskies();

  if (auto wr = get_world_renderer(); wr && (is_level_loaded() || empty_world_created))
  {
    acesfx::wait_fx_managers_update_and_allow_accum_cmds();
    FxRenderTargetOverride fx_q = FX_RT_OVERRIDE_DEFAULT;
    if (dafx_helper_globals::particles_resolution_preview == 0)
      fx_q = FX_RT_OVERRIDE_LOWRES;
    else if (dafx_helper_globals::particles_resolution_preview == 2)
      fx_q = FX_RT_OVERRIDE_HIGHRES;
    if (acesfx::get_rt_override() != fx_q)
    {
      DataBlock blk;
      blk.addBlock("graphics")->setStr("fxTarget", fx_q == FX_RT_OVERRIDE_HIGHRES ? "highres" : "lowres");
      dgs_apply_config_blk(blk, false, false);
      FastNameMap changed;
      changed.addNameId("graphics/fxTarget");
      wr->onSettingsChanged(changed, false);
    }
    // set actual dafx context to be used in tools code
    dafx_helper_globals::ctx = acesfx::get_dafx_context();
    dafx_helper_globals::cull_id = acesfx::get_cull_id();
    dafx_helper_globals::cull_fom_id = acesfx::get_cull_fom_id();

    phys_fetch_sim_res(true);
  }
}

static void dng_based_render::before_render(const Driver3dPerspective &persp, const TMatrix4D &proj_tm, int viewport_width)
{
  float rtDt = ::dagor_game_act_time;
  float scaledDt = rtDt; // * time_speed;
  if (auto wr = get_world_renderer(); wr && (is_level_loaded() || empty_world_created))
  {
    TMatrix itm = ::grs_cur_view.itm;
    itm.setcol(0, normalize(itm.getcol(0)));
    itm.setcol(1, normalize(itm.getcol(1)));
    itm.setcol(2, normalize(itm.getcol(2)));
    dng_based_render::currentTexCtx = TexStreamingContext(persp, viewport_width);
    wr->beforeRender(scaledDt, rtDt, last_dt_realtime_usec * 1e-6, last_gametime_sec, itm, DPoint3(::grs_cur_view.pos), persp,
      proj_tm);
  }
}


static const ManagedTex &dng_based_render::get_final_target()
{
  if (auto wr = get_world_renderer())
    return wr->getFinalTargetTex();
  return nullTarget;
}

extern void reset_async_game_tasks_flags();

static bool dng_based_render::scene_ready_for_render() { return get_world_renderer() && (is_level_loaded() || empty_world_created); }
static void dng_based_render::draw_scene()
{
  reset_async_game_tasks_flags();
  if (auto wr = get_world_renderer(); wr && (is_level_loaded() || empty_world_created))
  {
    wr->updateFinalTargetFrame();
    d3d::set_render_target(wr->getFinalTargetTex().getTex2D(), 0);
    bool prevAutoBlockFlag = ShaderGlobal::enableAutoBlockChange(false);
    wr->draw(0, last_dt_realtime_usec * 1e-6f);
    ShaderGlobal::enableAutoBlockChange(prevAutoBlockFlag);
  }
  d3d::set_render_target();
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
    if (auto das_entry = config->getStr("game_das_script", nullptr))
    {
      String dasEntryFileName(dd_get_fname(das_entry));
      String dasEntryName(0, "%%toolScripts/%s", dasEntryFileName.c_str());
      simplify_fname(dasEntryName);
      config->setStr("game_das_script", dasEntryName);
    }
    if (auto das_entry = config->getStr("game_das_script_init", nullptr))
    {
      String dasEntryFileName(dd_get_fname(das_entry));
      String dasEntryName(0, "%%toolScripts/%s", dasEntryFileName.c_str());
      simplify_fname(dasEntryName);
      if (dd_file_exists(dasEntryName))
        config->setStr("game_das_script_init", dasEntryName);
    }
    config->setBool("limitFps", config->getBool("limitFps", true));
    config->setInt("actRate", dagor_get_game_act_rate());
    config->setStr("scene", dng_scene_fname);
    config->setStr("entitiesPath", dng_template_fname);
    if (app_ecs_blk.paramCount() || app_ecs_blk.blockCount())
      config->addNewBlock(&app_ecs_blk, "ecs");
    config->setBool("disableMenu", true);
    config->setBool("skipSplashScreenAnimationInThread", true);
    config->addStr("esTag", "inside_tools");
    config->removeBlock("shadersWarmup");
    if (auto *graphics = config->getBlockByName("graphics"))
    {
      graphics->setStr("fxTarget", "highres");
      graphics->setBool("hmapPatchesEnabled", false); // TODO: hmap patches conflics with hmapSrv
      graphics->setBool("shouldRenderWaterRipples", false);
      const auto giAlgorithm = graphics->getStr("giAlgorithm", NULL);
      if (!giAlgorithm || String(giAlgorithm) == "high")
        graphics->setStr("giAlgorithm", "medium"); // TODO: high gi quality has broken screen probes
    }
    // TODO: debug visualization flickers in TSR
    if (auto *video = config->getBlockByName("video"))
    {
      const auto aaMode = render::antialiasing::get_method_from_name(video->getStr("antialiasing_mode", "off"));
      if (aaMode == render::antialiasing::AntialiasingMethod::TSR)
        video->setStr("antialiasing_mode", "high_fxaa");
      video->setBool("overrideAAforAutoResolution", false); // to prevent reloading settings.blk from disk
    }
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

  d3d_apply_gpu_settings(*::dgs_get_settings());
  app_start(false);

  init_fx();
  dafx_helper_globals::ctx = acesfx::get_dafx_context();
  dafx_helper_globals::cull_id = acesfx::get_cull_id();
  dafx_helper_globals::cull_fom_id = acesfx::get_cull_fom_id();
  G_ASSERT(dafx_helper_globals::ctx);
}
static void dng_based_render::term_dng_framework()
{
  sceneload::unload_current_game();
  game_scene::on_scene_deselected();
  destroy_world_renderer();
  app_close();
}

namespace webui
{
void init_dmviewer_plugin() {}
} // namespace webui

// Compatibility with DNG. Right now it doesn't impact anything here, so let's not complicate things.
bool is_initial_loading_complete() { return true; }
