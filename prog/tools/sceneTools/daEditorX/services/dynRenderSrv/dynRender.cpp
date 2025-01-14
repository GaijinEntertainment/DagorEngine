// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_dynRenderService.h>
#include <de3_skiesService.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_camera_elem.h>
#include <sepGui/wndGlobal.h>
#include <libTools/renderViewports/cachedViewports.h>
#include <libTools/renderViewports/renderViewport.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/util/strUtil.h>
#include <render/variance.h>
#include <render/renderType.h>
#include <render/waterRender.h>
#include <render/waterObjects.h>
#include <render/rain.h>
#include <render/deferredRenderer.h>
#include <render/cascadeShadows.h>
#include <render/lightCube.h>
#include <render/ssao.h>
#include <render/screenSpaceReflections.h>
#include <render/downsampleDepth.h>
#include <render/motionVectorAccess.h>
#include <render/viewVecs.h>
#include <render/sphHarmCalc.h>
#include <render/preIntegratedGF.h>
#include <render/genericLUT/fullColorGradingLUT.h>
#include <render/dag_cur_view.h>
#include <startup/dag_globalSettings.h>
#include <3d/dag_textureIDHolder.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <rendInst/rendInstGenRtTools.h>
#include <rendInst/debugCollisionVisualization.h>
#include <math/dag_cube_matrix.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_mathAng.h>
#include <math/dag_viewMatrix.h>

#include <de3_interface.h>
#include <de3_lightProps.h>
#include <de3_lightService.h>
#include <de3_waterSrv.h>
#include <de3_landmesh.h>
#include <EditorCore/ec_interface_ex.h>
#include <sepGui/wndPublic.h>

#include <scene/dag_visibility.h>
#include <shaders/dag_renderScene.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSettings.h>
#include <render/fx/dag_postfx.h>
#include <render/fx/dag_demonPostFx.h>
#include <render/wind/ambientWind.h>
#include <fx/dag_hdrRender.h>
#include <fx/dag_leavesWind.h>
#include <fx/toolsHeatHazeGlue.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_render.h>
#include <3d/dag_texPackMgr2.h>
#include <perfMon/dag_graphStat.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <windows.h>
#include <stdio.h>
#include <debug/dag_debug3d.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_renderStateId.h>
#include <drv/3d/dag_renderStates.h>
#include <render/upscale/upscaleSampling.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <vr/vrGuiSurface.h>

#include <drv/dag_vr.h>

static const uint32_t EXPOSURE_BUF_SIZE = 8;

extern CachedRenderViewports *ec_cached_viewports;
extern void ec_init_stat3d();
extern void ec_stat3d_wait_frame_end(bool frame_start);

struct DynamicRenderOption
{
  bool on, allowed;
  const char *name;

  DynamicRenderOption(const char *nm) : on(false), allowed(false), name(nm) {}
  void setup(bool allow, bool enable) { allowed = allow, on = enable; }

  operator bool() const { return on; }
  bool operator=(bool _on) { return on = allowed ? _on : on; }
};
#define DECL_ROPT(var, name) static DynamicRenderOption var(name);

DECL_ROPT(renderShadow, "render shadows");
DECL_ROPT(renderShadowVsm, "render VSM shadows");
DECL_ROPT(renderShadowFom, "render FOM shadows");
DECL_ROPT(renderWater, "render water reflections");
DECL_ROPT(renderEnvironmentFirst, "render envi first");
DECL_ROPT(renderSSAO, "use SSAO");
DECL_ROPT(renderSSR, "use SSR");
DECL_ROPT(renderNoSun, "no sun");
DECL_ROPT(renderNoAmb, "no amb");
DECL_ROPT(renderNoEnvRefl, "no envi refl");
DECL_ROPT(renderNoTransp, "no transp");
DECL_ROPT(renderNoPostfx, "no postfx");

static int shadowQuality = 0;
static bool dynamicRenderInited = false;
static bool vsm_allowed = true;
static bool render_enabled = false;
static float visibleScale = 1.0;
static float maxVisibleRange = MAX_REAL;
static float visibility_multiplier = 1.0;
static float sunScale = 1.0;
static float ambScale = 1.0;
static float CSMlambda = 0.7;
static float shadowDistance = 450;
static float shadowCasterDist = 50;
static float wireframeZBias = 0.1;
static int fogVarId = -1;
static TEXTUREID lin_float0000_texid = BAD_TEXTUREID;
static TEXTUREID lin_float1111_texid = BAD_TEXTUREID;
static DataBlock gameGlobVars;
static bool disable_postfx = false;
static bool enable_wireframe = false;

static int camera_rightVarId = -1;
static int camera_upVarId = -1;

static int fom_shadows_tm_xVarId;
static int fom_shadows_tm_yVarId;
static int fom_shadows_tm_zVarId;

static Tab<IRenderingService *> rendSrv(tmpmem);

// Starting with draw skipping by default. This way we ensure that before the first drawScene call actScene has already
// ran at least once.
static bool skip_next_frame = true;

static bool preparing_light_probe = false;
static bool use_cmpf_less_for_screenshots = false;

static bool render_vr_shadows = true;
static bool render_shadow() { return renderShadow && (DAEDITOR3.getStereoIndex() == StereoIndex::Mono || render_vr_shadows); }
static bool render_fom_shadow() { return renderShadowFom && (DAEDITOR3.getStereoIndex() == StereoIndex::Mono || render_vr_shadows); }


static bool dump_frame = false;
static void dump_ingame_profiler_cb(void *, uintptr_t cNode, uintptr_t pNode, const char *name, const TimeIntervalInfo &ti)
{
  char tabs[65];
  memset(tabs, ' ', 64);
  tabs[min<int>(ti.depth, 16) * 4] = 0;

  char gpuinterval[64], gpuData[128];
  if (ti.gpuValid)
  {
    SNPRINTF(gpuinterval, sizeof(gpuinterval), "%.2f;%.2f;%.2f", ti.tGpuMin, ti.tGpuMax, ti.tGpuSpike);
    SNPRINTF(gpuData, sizeof(gpuData), "%d;%d;%d;%d;%d;%d", ti.stat.val[DRAWSTAT_DP], (int)ti.stat.tri, ti.stat.val[DRAWSTAT_RT],
      ti.stat.val[DRAWSTAT_PS], ti.stat.val[DRAWSTAT_INS], ti.stat.val[DRAWSTAT_LOCKVIB]);
  }
  else
  {
    SNPRINTF(gpuinterval, sizeof(gpuinterval), ";;");
    SNPRINTF(gpuData, sizeof(gpuData), ";;;;;;;;;");
  }

  char skipped[16];
  if (ti.skippedFrames)
    SNPRINTF(skipped, sizeof(skipped), "%d", ti.skippedFrames);
  else
    skipped[0] = 0;
  char averageFrameCnt[32];
  double avFrames = ((double)ti.lifetimeCalls) / max(1u, ti.lifetimeFrames);
  if (avFrames <= 0.99)
    SNPRINTF(averageFrameCnt, sizeof(averageFrameCnt), "%.2f", avFrames);
  else
    averageFrameCnt[0] = 0;

  DAEDITOR3.conNote("%s%s;%d;%s;%.2f;%.2f;%.2f;%.2f;%s;%s", tabs, name, ti.perFrameCalls, averageFrameCnt, ti.tMin, ti.tMax, ti.tSpike,
    ti.unknownTimeMsec, gpuinterval, gpuData, skipped);
}

static void internal_dump_profiler()
{
  if (!dump_frame || !(da_profiler::get_active_mode() & da_profiler::EVENTS))
    return;

  DAEDITOR3.conNote("=======Frame Log========");
  DAEDITOR3.conNote("name;calls;avFrameCnt;cpuMin;cpuMax;cpuSpike;cpuOwn;gpuMin;gpuMax;gpuSpike;"
                    "dip;tri;rt;prog;ins;lockivb;skipped");
  da_profiler::dump_frames(dump_ingame_profiler_cb, NULL);
  dump_frame = false;
}

static int parseTexFmt(const char *sfmt, int def_fmt)
{
  int fmt = def_fmt;
  if (strcmp(sfmt, "A16B16G16R16F") == 0)
    fmt = TEXFMT_A16B16G16R16F;
  else if (strcmp(sfmt, "R11G11B10F") == 0)
    fmt = TEXFMT_R11G11B10F;
  else if (strcmp(sfmt, "A2B10G10R10") == 0)
    fmt = TEXFMT_A2B10G10R10;
  else if (strcmp(sfmt, "A8B8G8R8") == 0)
    fmt = TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  else if (strcmp(sfmt, "sRGB8") == 0)
    fmt = TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  else if (strcmp(sfmt, "ARGB8") == 0)
    fmt = TEXFMT_A8R8G8B8;
  else if (strcmp(sfmt, "R8G8") == 0)
    fmt = TEXFMT_R8G8;
  else if (strcmp(sfmt, "A8L8") == 0)
    fmt = TEXFMT_R8G8;
  else if (strcmp(sfmt, "L8") == 0 || strcmp(sfmt, "R8") == 0)
    fmt = TEXFMT_L8;
  else
    logwarn("fmt <%s> not recognized, using %08X", sfmt, fmt);
  return fmt;
}

static ToolsHeatHazeRendererGlue heat_haze_glue;
static bool use_heat_haze = false;
static int globalFrameBlockId = -1;
class DaEditor3DynamicScene : public DagorGameScene, public ICascadeShadowsClient //-V553
{
public:
  enum
  {
    RTYPE_CLASSIC = IDynRenderService::RTYPE_CLASSIC,
    RTYPE_DYNAMIC_A = IDynRenderService::RTYPE_DYNAMIC_A,
    RTYPE_DYNAMIC_DEFERRED = IDynRenderService::RTYPE_DYNAMIC_DEFERRED,
  };
  int rtype;
  bool reqEnviProbeUpdate;
  int dbgShowType;
  int effects_depth_texVarId = -1;
  bool renderMatrixOk;
  bool tryToggleVr = false;
  shaders::OverrideStateId noCullStateId;
  shaders::OverrideStateId geomEnviId;
  shaders::RenderStateId defaultRenderStateId;
  shaders::RenderStateId alphaWriterRenderStateId;
  shaders::RenderStateId depthMaskRenderStateId;
  eastl::unique_ptr<FullColorGradingTonemapLUT> tonemapLUT;

public:
  DaEditor3DynamicScene()
  {
    globalFrameBlockId = ShaderGlobal::getBlockId("global_frame", ShaderGlobal::LAYER_FRAME);
    renderMatrixOk = true;
    rtype = -1;
    dbgShowType = -1;
    gameParamsBlk.reset();
    DataBlock *b = gameParamsBlk.addBlock("postfx_rt");
    b->setInt("use_w", 4);
    b->setInt("use_h", 4);

    pfxLevelBlk = &gameParamsBlk;
    postFx = NULL;

    allowDynamicRender = false;
    enviReqSceneBlk = false;
    targetW = 0, targetH = 0;
    sceneFmt = 0;
    sceneRt = postfxRt = NULL;
    sceneRtId = postfxRtId = BAD_TEXTUREID;
    enviProbe = NULL;
    enviProbeBlack = NULL;
    deferredCsm = NULL;
    reqEnviProbeUpdate = true;
    deferredRtFmt = TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
    postfxRtFmt = TEXFMT_A8R8G8B8;
    deferred_mrt_cnt = 0;
    memset(deferred_mrt_fmts, 0, sizeof(deferred_mrt_fmts));
    srgb_backbuf_wr = false;
    csmMinSparseDist = 100000;
    csmMinSparseFrame = -1000;
    csmLambda = 0.8;
    csmMaxDist = 2000;
    vsmSz = 512;
    vsmMaxDist = 1000;
    vsmType = vsm.VSM_HW;
    enviCubeTexId = BAD_TEXTUREID;
    enviCubeVarName = "local_light_probe_tex";
    camera_rightVarId = ::get_shader_glob_var_id("camera_right", true);
    camera_upVarId = ::get_shader_glob_var_id("camera_up", true);
    fom_shadows_tm_xVarId = ::get_shader_variable_id("fom_shadows_tm_x", true);
    fom_shadows_tm_yVarId = ::get_shader_variable_id("fom_shadows_tm_y", true);
    fom_shadows_tm_zVarId = ::get_shader_variable_id("fom_shadows_tm_z", true);
    combinedShadowsRenderer.init("combine_shadows", true);

    String fn;
    fn.printf(0, "%s%s", sgg::get_exe_path_full(), "/../commonData/isoLightEnvi/lin_float0000.ddsx");
    dd_simplify_fname_c(fn);
    if (dd_file_exists(fn))
      lin_float0000_texid = add_managed_texture(fn);
    fn.printf(0, "%s%s", sgg::get_exe_path_full(), "/../commonData/isoLightEnvi/lin_float1111.ddsx");
    dd_simplify_fname_c(fn);
    if (dd_file_exists(fn))
      lin_float1111_texid = add_managed_texture(fn);

    shaders::OverrideState state;
    state.set(shaders::OverrideState::CULL_NONE);
    noCullStateId = shaders::overrides::create(state);

    state = shaders::OverrideState();
    state.set(shaders::OverrideState::Z_WRITE_DISABLE);
    state.set(shaders::OverrideState::BLEND_SRC_DEST);
    state.sblend = BLEND_ONE;
    state.dblend = BLEND_ZERO;
    state.set(shaders::OverrideState::BLEND_SRC_DEST_A);
    state.sblenda = BLEND_ZERO;
    state.dblenda = BLEND_ZERO;
    geomEnviId = shaders::overrides::create(state);

    defaultRenderStateId = shaders::render_states::create(shaders::RenderState());

    shaders::RenderState rs;
    rs.cull = CULL_NONE;
    rs.colorWr = WRITEMASK_ALPHA;
    rs.zFunc = CMPF_GREATEREQUAL;
    rs.zwrite = 0;
    depthMaskRenderStateId = shaders::render_states::create(rs);

    rs.zFunc = CMPF_ALWAYS;
    alphaWriterRenderStateId = shaders::render_states::create(rs);

    effects_depth_texVarId = ::get_shader_variable_id("effects_depth_tex", true);

    if (VariableMap::isVariablePresent(get_shader_variable_id("Exposure", true)))
    {
      exposureBuffer = dag::buffers::create_ua_sr_structured(4, EXPOSURE_BUF_SIZE, "Exposure");
      writeExposure(1.0f);
    }
  }

  ~DaEditor3DynamicScene()
  {
    shaders::overrides::destroy(geomEnviId);
    shaders::overrides::destroy(noCullStateId);
    del_it(postFx);
    combinedShadowsRenderer.clear();
    deferredTarget.reset();
    light_probe::destroy(enviProbe);
    ShaderGlobal::set_texture(get_shader_variable_id("envi_probe_specular"), BAD_TEXTUREID);
    enviProbe = NULL;
    light_probe::destroy(enviProbeBlack);
    enviProbeBlack = NULL;
    del_it(deferredCsm);
    cubeData.clear();
    bkgData.clear();
    voltexData.clear();
    vrResources.teardown();
    shutdown();
  }

  void toggleVrMode() { tryToggleVr = true; }

  void onTonemapSettingsChanged()
  {
    if (tonemapLUT)
      tonemapLUT->requestUpdate();
  }

  const ManagedTex &getDownsampledFarDepth()
  {
    return DAEDITOR3.getStereoIndex() == StereoIndex::Mono ? downsampledFarDepth : vrResources.downsampledFarDepth;
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
    if (dgs_app_active && CCameraElem::getCamera() != CCameraElem::MAX_CAMERA)
    {
      ViewportWindow *vpw = (ViewportWindow *)EDITORCORE->getCurrentViewport();
      if (!vpw)
        CCameraElem::switchCamera(false, false);
    }

    windEffect.updateWind(::dagor_game_act_time * ::dagor_game_time_scale);
    update_ambient_wind();

    // do not waste resources when app is minimized
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
  }

  void gatherRendSrv()
  {
    rendSrv.clear();
    int plugin_cnt = IEditorCoreEngine::get()->getPluginCount();
    for (int i = 0; i < plugin_cnt; ++i)
    {
      IGenEditorPluginBase *p = EDITORCORE->getPluginBase(i);
      if (p->getVisible())
      {
        IRenderingService *iface = p->queryInterface<IRenderingService>();
        if (iface)
          rendSrv.push_back(iface);
      }
    }
  }

  virtual void beforeDrawScene(int realtime_elapsed_usec, float /*gametime_elapsed_sec*/)
  {
    d3d::set_render_target();
    ::advance_shader_global_time(realtime_elapsed_usec * 1e-6f);

    if (ec_cached_viewports->getCurRenderVp() != -1)
      return;
    if (skip_next_frame)
      return;

    if (rtype == RTYPE_DYNAMIC_DEFERRED && reqEnviProbeUpdate && enviProbe)
    {
      debug("deferredRender: update enviProbe");
      IEditorCoreEngine::get()->beforeRenderObjects();
      gatherRendSrv();
      updateEnviProbe();
      reqEnviProbeUpdate = false;
    }
    if (tonemapLUT)
      tonemapLUT->render();
    IEditorCoreEngine::get()->beforeRenderObjects();
    d3d::set_render_target();
  }

  virtual void drawScene()
  {
    if (skip_next_frame)
      return;
    if (!IEditorCoreEngine::get() || !ec_cached_viewports || !render_enabled)
      return;
    if (ec_cached_viewports->getCurRenderVp() != -1)
      return;

    gatherRendSrv();

    ISceneLightService *ltService = EDITORCORE->queryEditorInterface<ISceneLightService>();
    if (!ltService)
    {
      memset(&ltSky, 0, sizeof(ltSky));
      memset(&ltSun0, 0, sizeof(ltSun0));
      memset(&ltSun1, 0, sizeof(ltSun1));
    }
    else
    {
      ltSky = ltService->getSky();
      if (ltService->getSunCount() > 0)
        ltSun0 = *ltService->getSun(0);
      else
        memset(&ltSun0, 0, sizeof(ltSun0));
      if (ltService->getSunCount() > 1)
        ltSun1 = *ltService->getSun(1);
      else
        memset(&ltSun1, 0, sizeof(ltSun1));
    }

    if (tryToggleVr)
    {
      toggleVrOnDemand();
      tryToggleVr = false;
    }

    if (VRDevice::getInstance())
    {
      if (ec_cached_viewports->viewportsCount() > 0)
      {
        const int viewportIndex = 0;
        ViewportWindow *vpw = (ViewportWindow *)ec_cached_viewports->getViewportUserData(viewportIndex);
        if (vpw && vpw->isViewportTextureReady())
        {
          ec_cached_viewports->startRender(viewportIndex);

          check_and_restore_3d_device();
          updateBackBufSize(vpw->getW(), vpw->getH());
          renderVrGui(vpw);
          renderVr(vpw);

          ec_cached_viewports->endRender();
        }
      }
    }
    else
    {
      for (int i = ec_cached_viewports->viewportsCount() - 1; i >= 0; i--)
      {
        ViewportWindow *vpw = (ViewportWindow *)ec_cached_viewports->getViewportUserData(i);
        if (!vpw || !vpw->isViewportTextureReady())
          continue;

        if (!ec_cached_viewports->startRender(i))
        {
          if (!ec_cached_viewports->getViewport(i))
            logwarn("ec_cached_viewports->startRender(%d) returns false, getViewport(%d)=%p", i, i,
              ec_cached_viewports->getViewport(i));
          continue;
        }

        renderViewportFrame(vpw);

        ec_cached_viewports->endRender();
      }
    }

    HWND hwnd = (HWND)EDITORCORE->getWndManager()->getMainWindow();

    RECT rect;
    GetClientRect(hwnd, &rect);
    const int clientRectWidth = rect.right - rect.left;
    const int clientRectHeight = rect.bottom - rect.top;

    if (::grs_draw_wire)
      d3d::setwire(0);

    d3d::set_render_target();
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 0, 0);
    renderUI(clientRectWidth, clientRectHeight);

    if (VRDevice::getInstance())
    {
      RectInt rect;
      rect.left = 0;
      rect.top = 0;
      rect.right = clientRectWidth;
      rect.bottom = clientRectHeight;
      d3d::stretch_rect(nullptr, vrResources.imguiTex.getTex2D(), &rect, &rect);
    }

    // When toggling the application's window between minimized and maximized state the client size could be 0. This is
    // here to avoid the assert in d3d::stretch_rect.
    if (clientRectWidth <= 0 || clientRectHeight <= 0)
      hwnd = nullptr;

    d3d::pcwin32::set_present_wnd(hwnd);
  }

  BaseTexture *getRenderBuffer() { return sceneRt; }

  D3DRESID getRenderBufferId() { return sceneRtId; }

  BaseTexture *getDepthBuffer() { return deferredTarget ? deferredTarget->getDepth() : nullptr; }

  D3DRESID getDepthBufferId() { return deferredTarget ? deferredTarget->getDepthId() : BAD_D3DRESID; }

  Texture *resolvePostProcessing(bool use_postfx, bool vr_mode)
  {
    Texture *sceneRt = vr_mode ? vrResources.sceneRt.getTex2D() : this->sceneRt;                                    //-V688
    Texture *postfxRt = vr_mode ? vrResources.postfxRt.getTex2D() : this->postfxRt;                                 //-V688
    TEXTUREID sceneRtId = vr_mode ? vrResources.sceneRt.getTexId() : this->sceneRtId;                               //-V688
    TEXTUREID postfxRtId = vr_mode ? vrResources.postfxRt.getTexId() : this->postfxRtId;                            //-V688
    DeferredRenderTarget *deferredTarget = vr_mode ? vrResources.deferredTarget.get() : this->deferredTarget.get(); //-V688

    if (!deferredTarget)
      return sceneRt;

    if (use_postfx)
    {
      if (::grs_draw_wire)
        d3d::setwire(0);
      if (postFx->getDemonPostFx())
        postFx->getDemonPostFx()->setSunDir(ltSun0.ltDir);

      static int depthTexVarId = get_shader_variable_id("depth_tex", true);
      ShaderGlobal::set_texture(depthTexVarId, deferredTarget->getDepthId());

      TMatrix4 projTm;
      d3d::gettm(TM_PROJ, &projTm);
      postFx->downsample(sceneRt, sceneRtId);
      postFx->apply(sceneRt, sceneRtId, postfxRt, postfxRtId, ::grs_cur_view.tm, projTm, true);
      d3d::set_render_target(postfxRt, 0);
      d3d::set_depth(deferredTarget->getDepth(), DepthAccess::SampledRO);
      return postfxRt;
    }
    else if (noPfxResolve.getMat())
    {
      static int sourceTexVarId = get_shader_variable_id("source_tex");
      if (::grs_draw_wire)
        d3d::setwire(0);
      d3d::set_render_target(postfxRt, 0);
      d3d::set_depth(deferredTarget->getDepth(), DepthAccess::SampledRO);
      ShaderGlobal::set_texture(sourceTexVarId, sceneRtId);
      noPfxResolve.render();
      ShaderGlobal::set_texture(sourceTexVarId, BAD_TEXTUREID);
      return postfxRt;
    }
    return sceneRt;
  }

  void setupMotionParams(const ViewportWindow &vpw)
  {
    TMatrix viewItm;
    vpw.getCameraTransform(viewItm);
    float zn, zf;
    vpw.getZnearZfar(zn, zf);
    motion_vector_access::CameraParams currentCamera{vpw.getViewTm(), viewItm, vpw.getProjTm(), viewItm.getcol(3), zn, zf};
    motion_vector_access::set_motion_vector_type(motion_vector_access::MotionVectorType::StaticUVZ);
    motion_vector_access::set_params(currentCamera, previousCamera.has_value() ? previousCamera.value() : currentCamera, {}, {},
      eastl::nullopt);
    previousCamera = currentCamera;
  }

  void renderViewportFrame(ViewportWindow *vpw, bool cached_render = false)
  {
    G_ASSERT(ec_cached_viewports->getCurRenderVp() >= 0);

    if (!render_enabled)
    {
    empty_render:
      int target_w, target_h;
      d3d::get_target_size(target_w, target_h);
      d3d::setview(0, 0, target_w, target_h, 0, 1);
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(10, 10, 64, 0), 0, 0);
      return;
    }

    if (!check_and_restore_3d_device())
      goto empty_render;

    Driver3dRenderTarget rt;
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;
    bool use_postfx = (::hdr_render_mode != HDR_MODE_NONE) && postFx && !renderNoPostfx;

    d3d::get_render_target(rt);
    d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);

    if (renderNoPostfx)
      setExposure(1.0f);

    if (use_heat_haze)
    {
      auto renderHazeParticles = [this]() { renderGeomDistortionFx(); };
      auto renderHazeRI = [this]() { renderGeomDistortion(); };
      heat_haze_glue.setFunctions(renderHazeParticles, renderHazeRI);
    }

    if (cached_render && sceneRt && viewportW == targetW && viewportH == targetH && hdr_render_format == sceneFmt)
      goto skip_render_scene;
    else
      cached_render = false;

    updateBackBufSize(viewportW, viewportH);
    use_postfx = (::hdr_render_mode != HDR_MODE_NONE) && postFx && !renderNoPostfx;
    if (renderNoPostfx)
      setExposure(1.0f);
    if (!sceneRt)
      goto empty_render;

    // G_ASSERT(!rt.tex);
    G_ASSERT(rt.isBackBufferColor());

    if (vpw->needStat3d() && dgs_app_active)
    {
      ec_stat3d_wait_frame_end(true);
    }

    enable_wireframe = vpw->wireframeOverlayEnabled();

    beforeRender();
    d3d::set_render_target(sceneRt, 0);
    d3d::set_depth(fallbackSceneDepth.getTex2D(), DepthAccess::RW);
    d3d::setview(0, 0, viewportW, viewportH, 0, 1);
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(64, 64, 64, 0), 0, 0);

    if (rtype == RTYPE_CLASSIC)
    {
      classicRender();
      classicRenderTrans();
    }
    else if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      setupMotionParams(*vpw);
      deferredRender(vpw->getViewTm(), vpw->getProjTm(), false);
    }

    if (use_heat_haze)
      heat_haze_glue.render();

  skip_render_scene:
    Texture *finalRt = resolvePostProcessing(use_postfx, false);

    if (vpw->needStat3d() && dgs_app_active && !cached_render)
    {
      ec_stat3d_wait_frame_end(false);
    }

    if (rtype == RTYPE_DYNAMIC_DEFERRED && dbgShowType != -1)
    {
      if (::grs_draw_wire)
        d3d::setwire(0);
      d3d::set_render_target(finalRt, 0);
      deferredTarget->debugRender(dbgShowType);
      goto skip_non_geom_render;
    }

    if (enable_wireframe && wireframeTex && wireframeRenderer.getMat())
    {
      wireframeTex.setVar();
      wireframeRenderer.render();
    }

    IEditorCoreEngine::get()->renderObjects();
    IEditorCoreEngine::get()->renderTransObjects();

    if (::grs_draw_wire)
      d3d::setwire(0);

    ec_camera_elem::freeCameraElem->render();
    ec_camera_elem::maxCameraElem->render();
    ec_camera_elem::fpsCameraElem->render();
    ec_camera_elem::tpsCameraElem->render();
    ec_camera_elem::carCameraElem->render();

  skip_non_geom_render:
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

    internal_dump_profiler();
    dagor_frame_no_increment();
  }

  void renderScreenShot()
  {
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;
    Driver3dRenderTarget rt;

    d3d::get_render_target(rt);
    d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    bool rt_ready = (sceneRt && viewportW == targetW && viewportH == targetH && sceneFmt == hdr_render_format);
    updateBackBufSize(viewportW, viewportH);

    bool use_postfx = (::hdr_render_mode != HDR_MODE_NONE) && postFx && !renderNoPostfx;

    if (renderNoPostfx)
      setExposure(1.0f);

  render_again:
    beforeRender();
    d3d::set_render_target(sceneRt, 0);
    d3d::set_depth(fallbackSceneDepth.getTex2D(), DepthAccess::RW);
    d3d::setview(0, 0, viewportW, viewportH, viewportMinZ, viewportMaxZ);
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(64, 64, 64, 0), 0, 0);

    if (rtype == RTYPE_CLASSIC)
    {
      classicRender();
      classicRenderTrans();
    }
    else if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      TMatrix viewTm;
      TMatrix4 projTm;
      d3d::gettm(TM_VIEW, viewTm);
      d3d::gettm(TM_PROJ, &projTm);
      deferredRender(viewTm, projTm, false);
    }
    if (!rt_ready)
    {
      rt_ready = true;
      goto render_again;
    }

    Texture *finalRt = resolvePostProcessing(use_postfx, false);

    if (rtype == RTYPE_DYNAMIC_DEFERRED && dbgShowType != -1)
    {
      if (::grs_draw_wire)
        d3d::setwire(0);
      d3d::set_render_target(finalRt, 0);
      deferredTarget->debugRender(dbgShowType);
      goto skip_non_geom_render;
    }

    IEditorCoreEngine::get()->renderObjects();
    IEditorCoreEngine::get()->renderTransObjects();
    ec_camera_elem::freeCameraElem->render();
    ec_camera_elem::maxCameraElem->render();
    ec_camera_elem::fpsCameraElem->render();
    ec_camera_elem::tpsCameraElem->render();
    ec_camera_elem::carCameraElem->render();

  skip_non_geom_render:
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
      if (rtype == RTYPE_DYNAMIC_DEFERRED)
        d3d::set_depth(deferredTarget->getDepth(), DepthAccess::RW);
      else
        d3d::set_depth(fallbackSceneDepth.getTex2D(), DepthAccess::RW);
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
  void closeMicroDetails()
  {
    if (characterMicrodetailsId == BAD_TEXTUREID)
      return;
    ShaderGlobal::reset_from_vars(characterMicrodetailsId);
    release_managed_tex(characterMicrodetailsId);
    characterMicrodetailsId = BAD_TEXTUREID;
  }
  void reloadMicroDetails(const DataBlock &deferredBlk)
  {
    if (get_shader_variable_id("character_micro_details", true) >= 0)
    {
      closeMicroDetails();
      const DataBlock *micro = deferredBlk.getBlockByName("character_micro_details");
      if (micro)
      {
        extern TEXTUREID load_texture_array_immediate(const char *name, const char *param_name, const DataBlock &blk, int &count);
        int microDetailCount = 0;
        characterMicrodetailsId = load_texture_array_immediate("character_micro_details*", "micro_detail", *micro, microDetailCount);
        ShaderGlobal::set_texture(get_shader_variable_id("character_micro_details"), characterMicrodetailsId);
        {
          d3d::SamplerInfo smpInfo;
          smpInfo.anisotropic_max = ::dgs_tex_anisotropy;
          ShaderGlobal::set_sampler(get_shader_variable_id("character_micro_details_samplerstate", true),
            d3d::request_sampler(smpInfo));
        }
        ShaderGlobal::set_int(get_shader_variable_id("character_micro_details_count", true), microDetailCount);
        debug("microDetailCount = %d", microDetailCount);

        // WT character microdetail shader code uses land_micro_details sampler for character microdetails
        // If it's not set, the character microdetails will use an invalid sampler
        // This is a workaround until separate samplers are supported in driver
        int land_micro_detailsVarId = get_shader_variable_id("land_micro_details", true);
        if (land_micro_detailsVarId >= 0)
          ShaderGlobal::set_texture(land_micro_detailsVarId, characterMicrodetailsId);
      }
    }
  }
  void prepareVSM()
  {
    if (!vsm_allowed)
      return;
    if (!renderShadowVsm)
    {
      vsm.setOff();
      return;
    }

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    BBox3 viewBox;
    static Point3 oldViewPos(0, 0, 0);
    if (lengthSq(::grs_cur_view.pos - oldViewPos) > 400)
      oldViewPos = ::grs_cur_view.pos;
    viewBox[0] = oldViewPos - Point3(vsmMaxDist, 2000, vsmMaxDist);
    viewBox[1] = oldViewPos + Point3(vsmMaxDist, 2000, vsmMaxDist);

    if (vsm.startShadowMap(viewBox, -ltSun0.ltDir, vsmMaxDist))
    {
      TIME_D3D_PROFILE_NAME(render_vsm, "prepare_vsm");
      static int transformZVarId = ::get_shader_variable_id("transform_z", true);
      uint32_t overrideFlags = 0;
      Color4 tz;

      if (rtype == RTYPE_DYNAMIC_DEFERRED)
      {
        tz = ShaderGlobal::get_color4_fast(transformZVarId);
        ShaderGlobal::set_color4_fast(transformZVarId, 0.f, 0.f, 0, 1.f);
      }

      for (int i = 0; i < rendSrv.size(); i++)
        rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_SHADOWS_VSM);

      if (rtype == RTYPE_DYNAMIC_DEFERRED)
      {
        ShaderGlobal::set_color4_fast(transformZVarId, tz.r, tz.g, tz.b, tz.a);
      }
    }
    vsm.endShadowMap();
  }
  void prepareFOM()
  {
    float worldLevel = -FLT_MAX;
    if (auto landmesh = EDITORCORE->getInterfaceEx<ILandmesh>())
    {
      float landHeight = landmesh->getBBoxWithHMapWBBox()[0].y;
      worldLevel = landHeight;
    }
    if (auto waterService = EDITORCORE->queryEditorInterface<IWaterService>())
    {
      float waterLevel = waterService->get_level();
      worldLevel = max(worldLevel, waterLevel);
    }

    if (worldLevel == -FLT_MAX)
      worldLevel = -fomZViewBoxSize / 4;

    SCOPE_VIEW_PROJ_MATRIX;

    Point3 dirToSun = ltSun0.ltDir;
    Point3 groundPos = Point3::xVz(::grs_cur_view.pos, worldLevel);
    Point3 fomViewPos = groundPos + dirToSun * (fomZDistance + fomZViewBoxSize / dirToSun.y);

    BBox3 worldBox(groundPos + Point3(-fomXyViewBoxSize, 0, -fomXyViewBoxSize),
      groundPos + Point3(+fomXyViewBoxSize, +fomZViewBoxSize, +fomXyViewBoxSize));

    TMatrix lightViewITm = TMatrix::IDENT;
    view_matrix_from_look(-dirToSun, lightViewITm);
    lightViewITm.setcol(3, fomViewPos);
    TMatrix fomView = orthonormalized_inverse(lightViewITm);

    const BBox3 spacebox = fomView * worldBox;
    TMatrix4 fomProj = matrix_ortho_off_center_lh(spacebox[0].x, spacebox[1].x, spacebox[0].y, spacebox[1].y,
      spacebox[0].z - fomZDistance, spacebox[1].z + fomZDistance);

    TMatrix4 viewproj = TMatrix4(fomView) * fomProj;
    TMatrix4 offset = TMatrix4::IDENT;

    // align to fight aliasing
    const float halfTexel = 0.5f * fomTexSize;
    const float texCoordX = viewproj._41 * halfTexel, texCoordY = viewproj._42 * halfTexel;
    offset._41 = floor(texCoordX) / halfTexel - viewproj._41;
    offset._42 = floor(texCoordY) / halfTexel - viewproj._42;
    fomProj = fomProj * offset;

    d3d::settm(TM_PROJ, &fomProj);
    d3d::settm(TM_VIEW, fomView);

    d3d::set_render_target({}, DepthAccess::RW, {{fomShadowsCos.getTex2D(), 0}, {fomShadowsSin.getTex2D(), 0}});
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_SHADOWS_FOM);

    TMatrix4 texTm = TMatrix4(fomView) * fomProj * screen_to_tex_scale_tm_xy();
    ShaderGlobal::set_color4(fom_shadows_tm_xVarId, texTm.m[0][0], texTm.m[1][0], texTm.m[2][0], texTm.m[3][0]);
    ShaderGlobal::set_color4(fom_shadows_tm_yVarId, texTm.m[0][1], texTm.m[1][1], texTm.m[2][1], texTm.m[3][1]);
    ShaderGlobal::set_color4(fom_shadows_tm_zVarId, texTm.m[0][2], texTm.m[1][2], texTm.m[2][2], texTm.m[3][2]);

    d3d::resource_barrier({fomShadowsCos.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({fomShadowsSin.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    fomShadowsCos.setVar();
    fomShadowsSin.setVar();
    fomShadowsCos->texaddr(TEXADDR_BORDER);
    fomShadowsSin->texaddr(TEXADDR_BORDER);
    fomShadowsCos->texbordercolor(0);
    fomShadowsSin->texbordercolor(0);
  }
  void disableFOM()
  {
    ShaderGlobal::set_color4(fom_shadows_tm_xVarId, 0, 0, 0, -1);
    ShaderGlobal::set_color4(fom_shadows_tm_yVarId, 0, 0, 0, -1);
    ShaderGlobal::set_color4(fom_shadows_tm_zVarId, 0, 0, 0, -1e7);
  }
  void beforeRender()
  {
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::set_color4(worldViewPosVarId, Color4(::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f));
    IEditorCoreEngine::get()->beforeRenderObjects();
    if (hasEnvironmentSnapshot())
      applyEnvironmentSnapshot();
    TIME_D3D_PROFILE_NAME(render_vsm, "before_render");
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_BEFORE_RENDER);

    ShaderGlobal::set_color4(worldViewPosVarId, Color4(::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f));

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
    EDITORCORE->queryEditorInterface<IVisibilityFinderProvider>()->getVisibilityFinder().set(v_ldu(&::grs_cur_view.pos.x), f, 0, 0, 1,
      p.hk, current_occlusion);


    if (globalFrameBlockId != -1)
      ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    // render with parallax
    static int render_with_normalmapVarId = ::get_shader_glob_var_id("render_with_normalmap", true);
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 2);
    if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      static int zn_zfarVarId = get_shader_variable_id("zn_zfar", true);
      static int from_sun_directionVarId = get_shader_variable_id("from_sun_direction", true);
      static int sky_plane_wVarId = get_shader_variable_id("sky_plane_w", true);

      Driver3dPerspective p(0, 0, 0.1, 1000, 0, 0);
      d3d::getpersp(p);
      ShaderGlobal::set_color4(zn_zfarVarId, p.zn, p.zf, 0, 0);
      ShaderGlobal::set_real(sky_plane_wVarId, p.zf * 0.999f);

      ShaderGlobal::set_color4(from_sun_directionVarId, -ltSun0.ltDir.x, -ltSun0.ltDir.y, -ltSun0.ltDir.z, 0);
      setDeferredEnviLight();
      TMatrix4 projTm;
      d3d::gettm(TM_PROJ, &projTm);
      if (render_shadow())
      {
        TMatrix4 globtm;
        d3d::getglobtm(globtm);

        CascadeShadows::ModeSettings mode;
        mode.powWeight = csmLambda;
        mode.maxDist = csmMaxDist;
        mode.shadowStart = p.zn;
        mode.numCascades = 4;
        deferredCsm->prepareShadowCascades(mode, ltSun0.ltDir, ::grs_cur_view.tm, ::grs_cur_view.pos, projTm, Frustum(globtm),
          Point2(p.zn, p.zf), p.zn);
      }

      TMatrix viewRot;
      d3d::gettm(TM_VIEW, viewRot);
      viewRot.setcol(3, 0.f, 0.f, 0.f);

      TMatrix4 resTm;
      float det;
      renderMatrixOk = inverse44(TMatrix4(viewRot) * projTm, resTm, det);
      if (!renderMatrixOk)
      {
        static TMatrix4 lastProj = TMatrix4::IDENT;
        static TMatrix lastView = TMatrix::IDENT;
        if (memcmp(&lastProj, &projTm, sizeof(lastProj)) != 0 || memcmp(&lastView, &viewRot, sizeof(lastView)) != 0) //-V1014
        {
          DAEDITOR3.conError("inverse failed for view=%@ proj=%@,%@,%@,%@", viewRot, projTm.getrow(0), projTm.getrow(1),
            projTm.getrow(2), projTm.getrow(3));
          lastProj = projTm;
          lastView = viewRot;
        }
      }
    }

    if (hasExposure() && hasExposureChanged)
      if (writeExposure(exposure))
        hasExposureChanged = false;
  }


  void classicRender()
  {
    if (renderEnvironmentFirst)
    {
      renderGeomEnvi();
      renderGeomOpaque();
    }
    else
    {
      renderGeomOpaque();
      renderGeomEnvi();
    }
  }
  void classicRenderTrans() { renderGeomTrans(); }

  void setDeferredEnviLight()
  {
    static int local_light_probe_texVarId = get_shader_variable_id("local_light_probe_tex", true);
    static int sun_color_0_varId = get_shader_variable_id("sun_color_0", true);
    static int envi_sph_N_texVarId[SphHarmCalc::SPH_COUNT];
    static bool envi_sph_N_texVarId_inited = false;
    if (!envi_sph_N_texVarId_inited)
    {
      for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
        envi_sph_N_texVarId[i] = get_shader_variable_id(String(128, "enviSPH%d", i));
      envi_sph_N_texVarId_inited = true;
    }

    static const Color4 black(0, 0, 0, 0);
    const Color4 *sphHarm = light_probe::getSphHarm(enviProbe);

    if (renderNoSun)
      ShaderGlobal::set_color4(sun_color_0_varId, black);

    bool no_ess = !hasEnvironmentSnapshot();
    if (renderNoAmb || no_ess)
      for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
        ShaderGlobal::set_color4(envi_sph_N_texVarId[i], renderNoAmb ? black : sphHarm[i]);

    if (enviProbeBlack && (renderNoEnvRefl || no_ess)) // set light probe only after updateEnviProbe() called at least once
      ShaderGlobal::set_texture(local_light_probe_texVarId, *light_probe::getManagedTex(renderNoEnvRefl ? enviProbeBlack : enviProbe));
  }

  void deferredRender(const TMatrix &view_tm, const TMatrix4 &proj_tm, bool vr_mode)
  {
    static int global_frame_const_blockid = ShaderGlobal::getBlockId("global_const_block");
    ShaderGlobal::setBlock(global_frame_const_blockid, ShaderGlobal::LAYER_GLOBAL_CONST);

    if (render_shadow())
    {
      deferredCsm->renderShadowsCascades();
      prepareVSM();
    }

    if (render_fom_shadow())
      prepareFOM();
    else
      disableFOM();

    auto deferredTarget = vr_mode ? vrResources.deferredTarget.get() : this->deferredTarget.get();                            //-V688
    auto &resolvedDepth = vr_mode ? vrResources.resolvedDepth : this->resolvedDepth;                                          //-V688
    auto &downsampledFarDepth = vr_mode ? vrResources.downsampledFarDepth : this->downsampledFarDepth;                        //-V688
    auto &combinedShadowsTex = vr_mode ? vrResources.combinedShadowsTex : this->combinedShadowsTex;                           //-V688
    auto sceneRt = vr_mode ? vrResources.sceneRt.getTex2D() : this->sceneRt;                                                  //-V688
    bool renderSSR = vr_mode ? false : (bool)::renderSSR;                                                                     //-V688
    auto ssao = vr_mode ? vrResources.ssao.get() : this->ssao.get();                                                          //-V688
    auto upscaleSamplingRenderer = vr_mode ? vrResources.upscaleSamplingRenderer.get() : this->upscaleSamplingRenderer.get(); //-V688

    deferredTarget->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
    if (!renderMatrixOk)
      return;

    renderGeomOpaque();

    d3d::set_depth(deferredTarget->getDepth(), DepthAccess::SampledRO);
    renderGeomDeferredDecals();
    d3d::set_depth(deferredTarget->getDepth(), DepthAccess::RW);

    if (::grs_draw_wire)
      d3d::setwire(0);

    static int intz_depth_texVarId = get_shader_variable_id("intz_depth_tex", true);
    ShaderGlobal::set_texture(intz_depth_texVarId, deferredTarget->getDepthId());
    d3d::set_render_target(resolvedDepth.getTex2D(), 0);
    resolvedDepthRenderer.render();

    ShaderGlobal::set_texture(combinedShadowsTex.getVarId(), BAD_TEXTUREID);

    if (render_shadow() && combinedShadowsTex && combinedShadowsRenderer.getMat())
    {
      deferredTarget->setVar();
      deferredCsm->setCascadesToShader();
      d3d::set_render_target(combinedShadowsTex.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0xffffffff, 0, 0);
      ::set_viewvecs_to_shader(view_tm, proj_tm);
      combinedShadowsRenderer.render();
      combinedShadowsTex.setVar();
    }
    else if (render_shadow())
      deferredCsm->setCascadesToShader();

    deferredTarget->setRt();

    // always downsample depth to downsampledFarDepth
    {
      deferredTarget->setVar();
      TextureIDPair fullDepthPair = TextureIDPair(deferredTarget->getDepthAll().getTex2D(), deferredTarget->getDepthAll().getTexId());
      TextureIDPair farDepthPair = TextureIDPair(downsampledFarDepth.getTex2D(), downsampledFarDepth.getTexId());
      TextureIDPair normalPair = TextureIDPair(downsampledNormals.getTex2D(), downsampledNormals.getTexId());
      TextureIDPair checkerDepthPair = TextureIDPair(checkerboardDepth.getTex2D(), checkerboardDepth.getTexId());

      downsample_depth::downsamplePS(fullDepthPair, deferredTarget->getWidth(), deferredTarget->getHeight(), &farDepthPair, nullptr,
        &normalPair, nullptr, &checkerDepthPair);
      downsampledFarDepth.setVar();
      downsampledNormals.setVar();
      checkerboardDepth.setVar();
      if (upscaleSamplingRenderer)
        upscaleSamplingRenderer->render();
      if (renderSSAO)
      {
        TMatrix viewTm;
        TMatrix4 projTm;
        d3d::gettm(TM_VIEW, viewTm);
        d3d::gettm(TM_PROJ, &projTm);
        ssao->render(viewTm, projTm, downsampledFarDepth.getTex2D());
      }
      if (renderSSR)
      {
        static int causticsOptionsVarId = get_shader_variable_id("caustics_options", true);
        static int causticsHeroPosVarId = get_shader_variable_id("caustics_hero_pos", true);
        if ((causticsOptionsVarId >= 0) && (causticsHeroPosVarId >= 0))
        {
          Point2 causticsHeight = Point2(5.0f, 5.0f);
          Point4 causticsHeroPos = Point4(0, -100000, 0, 0);

          ShaderGlobal::set_color4(causticsOptionsVarId, 0.2f,
            0.0f, // todo: insert time
            causticsHeight.x, causticsHeight.y);
          ShaderGlobal::set_color4(causticsHeroPosVarId, Color4::xyzw(causticsHeroPos));
        }

        static int ssrWorldViewPosVarId = get_shader_variable_id("ssr_world_view_pos", true);
        if (ssrWorldViewPosVarId >= 0)
        {
          ShaderGlobal::set_color4(ssrWorldViewPosVarId, Color4::xyz1(::grs_cur_view.pos));
        }
        TMatrix viewTm;
        TMatrix4 projTm;
        d3d::gettm(TM_VIEW, viewTm);
        d3d::gettm(TM_PROJ, &projTm);
        ssr->render(viewTm, projTm);
      }
    }
    if (!renderSSAO)
      ShaderGlobal::set_texture(get_shader_variable_id("ssao_tex", true), BAD_TEXTUREID);
    if (!renderSSR)
      ShaderGlobal::set_texture(get_shader_variable_id("ssr_target", true), blackTex.getTexId());

    static int ao_ssrVarId = ::get_shader_glob_var_id("ao_ssr", true);
    if (ao_ssrVarId >= 0)
      ShaderGlobal::set_color4(ao_ssrVarId, (int)renderSSAO, (int)renderSSR, 0, 0);

    if (!tonemapLUT)
    {
      const char *mat1 = "render_full_tonemap", *mat2 = "compute_full_tonemap";
      static int lut_mat_search_tries = 0;
      Ptr<ShaderMaterial> lut_mat_ps = new_shader_material_by_name_optional(mat1);
      Ptr<ShaderMaterial> lut_mat_cs = new_shader_material_by_name_optional(mat2);
      if (lut_mat_ps || lut_mat_cs)
        tonemapLUT = eastl::make_unique<FullColorGradingTonemapLUT>(::hdr_render_mode != HDR_MODE_NONE);
      else if (++lut_mat_search_tries < 10)
        debug("tonemapLUT is not inited (both '%s' and '%s' shaders are missing), attempt=%d", mat1, mat2, lut_mat_search_tries);
    }
    if (effects_depth_texVarId != -1)
      ShaderGlobal::set_texture(effects_depth_texVarId, deferredTarget->getDepthId());

    ShaderGlobal::setBlock(global_frame_const_blockid, ShaderGlobal::LAYER_GLOBAL_CONST);

    deferredTarget->resolve(sceneRt, view_tm, proj_tm);

    d3d::set_render_target(sceneRt, 0);
    d3d::set_depth(deferredTarget->getDepth(), DepthAccess::SampledRO);

    renderGeomEnvi();

    if (vr_mode)
    {
      renderGeomTrans();
      return;
    }

    d3d::stretch_rect(sceneRt, downsampledOpaqueTarget.getTex2D(), NULL, NULL);
    d3d::setwire(::grs_draw_wire);
    renderGeomTrans();

    d3d::set_render_target({checkerboardDepth.getBaseTex(), 0}, DepthAccess::SampledRO, {{lowresFxTex.getBaseTex(), 0}});
    d3d::clearview(CLEAR_TARGET, 0xFF000000, 0, 0);
    if (effects_depth_texVarId != -1)
      ShaderGlobal::set_texture(effects_depth_texVarId, checkerboardDepth.getTexId());

    renderGeomEffects(true);

    d3d::set_render_target({deferredTarget->getDepth(), 0}, DepthAccess::SampledRO, {{sceneRt, 0}});
    if (effects_depth_texVarId != -1)
      ShaderGlobal::set_texture(effects_depth_texVarId, deferredTarget->getDepthId());

    static int lowres_fx_source_texVarId = ::get_shader_variable_id("lowres_fx_source_tex", /*optional*/ true);
    ShaderGlobal::set_texture(lowres_fx_source_texVarId, lowresFxTex);
    applyLowResFx.render();

    renderGeomEffects();

    d3d::set_depth(deferredTarget->getDepth(), DepthAccess::RW);

    if (enable_wireframe)
    {
      TIME_D3D_PROFILE_NAME(render_vsm, "render_wireframe");

      d3d::set_depth(deferredTarget->getDepth(), DepthAccess::RW);
      if (!::grs_draw_wire)
        d3d::setwire(1);

      if (wireframeTex)
      {
        d3d::set_render_target(wireframeTex.getTex2D(), 0);
        d3d::clearview(CLEAR_TARGET, E3DCOLOR(255, 255, 255, 255), 0, 0);
      }

      static int use_atestVarId = ::get_shader_variable_id("use_atest", true);
      int use_atest = ShaderGlobal::get_int(use_atestVarId);
      ShaderGlobal::set_int(use_atestVarId, 0);

      d3d::set_depth(deferredTarget->getDepth(), DepthAccess::SampledRO);
      shaders::overrides::set(wireframeState);
      renderGeomForWireframe();
      shaders::overrides::reset();

      ShaderGlobal::set_int(use_atestVarId, use_atest);

      if (!::grs_draw_wire)
        d3d::setwire(0);
    }

#if DAGOR_DBGLEVEL > 0
    // Workaround for z-fighting
    constexpr float drawCollisionsBias = 0.00001f;

    TMatrix4 proj;
    d3d::gettm(TM_PROJ, &proj);
    TMatrix4 savedProj = proj;
    proj[3][2] += drawCollisionsBias;
    d3d::settm(TM_PROJ, &proj);

    mat44f globtm;
    d3d::getglobtm(globtm);
    rendinst::drawDebugCollisions({}, globtm, ::grs_cur_view.pos, true);

    d3d::settm(TM_PROJ, &savedProj);
#endif
  }
  virtual Point4 getCascadeShadowAnchor(int cascade_no) { return Point4::xyz0(-::grs_cur_view.itm.getcol(3)); }
  virtual void renderCascadeShadowDepth(int cascade_no, const Point2 &znzf)
  {
    d3d::settm(TM_VIEW, TMatrix::IDENT);
    d3d::settm(TM_PROJ, &deferredCsm->getWorldRenderMatrix(cascade_no));
    const TMatrix &shadowViewItm = deferredCsm->getShadowViewItm(cascade_no);
    windEffect.setNoAnimShaderVars(shadowViewItm.getcol(0), shadowViewItm.getcol(1), shadowViewItm.getcol(2));
    if (render_shadow())
      renderGeomForShadows();
    else
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 1, 0);
  }
  virtual void getCascadeShadowSparseUpdateParams(int cascade_no, const Frustum & /*cascade_frustum*/, float &out_min_sparse_dist,
    int &out_min_sparse_frame)
  {
    out_min_sparse_dist = csmMinSparseDist;
    out_min_sparse_frame = csmMinSparseFrame;
  }

  void restartPostfx(const DataBlock &game_params)
  {
    gameParamsBlk.setFrom(&game_params);
    DataBlock *b = gameParamsBlk.addBlock("postfx_rt");
    b->setInt("use_w", sceneRt ? targetW : 4);
    b->setInt("use_h", sceneRt ? targetH : 4);
    b->setInt("preserve_adapt_val", 0);
    gameParamsBlk.removeBlock("hdr");
    if (disable_postfx)
      if (DataBlock *b = gameParamsBlk.getBlockByName("DemonPostFx"))
        b->setBool("use", false);
    if (!postFx)
      return createPostFx();
    if (!postFx->updateSettings(pfxLevelBlk, &gameParamsBlk))
    {
      postFx->restart(pfxLevelBlk, &gameParamsBlk, &pfx);
      if (postFx->getDemonPostFx())
        postFx->getDemonPostFx()->setVolFogCallback(&volFogCallback);
    }
  }

  void renderGeomEnvi()
  {
    TIME_D3D_PROFILE_NAME(render_vsm, "render_envi");
    bool ortho = false;
    if (IGenViewportWnd *vp = EDITORCORE->getRenderViewport())
      ortho = vp->isOrthogonal();

    if (enviCubeTexId != BAD_TEXTUREID && !ortho)
    {
      d3d::setwire(false);
      shaders::overrides::set(geomEnviId);

      if (BaseTexture *bt = acquire_managed_tex(enviCubeTexId))
      {
        renderEnviCubeTexture(bt, Color4(1, 1, 1, 1) * getExposure(), Color4(0, 0, 0, 1));
        release_managed_tex(enviCubeTexId);
      }

      shaders::overrides::reset();
      d3d::setwire(::grs_draw_wire);
      ShaderElement::invalidate_cached_state_block();
      return;
    }

    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_ENVI);
    if (enviReqSceneBlk)
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_CLOUDS);
  }
  void renderGeomOpaque()
  {
    TIME_D3D_PROFILE_NAME(render_vsm, "render_opaque");
    windEffect.setShaderVars(::grs_cur_view.itm);

    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_STATIC_OPAQUE);
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);

    ec_camera_elem::carCameraElem->externalRender();
  }
  void renderGeomDeferredDecals()
  {
    TIME_D3D_PROFILE_NAME(render_vsm, "render_decal");
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_STATIC_DECALS);
  }
  void renderGeomDistortion()
  {
    ShaderGlobal::set_color4(camera_rightVarId, ::grs_cur_view.itm.getcol(0));
    ShaderGlobal::set_color4(camera_upVarId, ::grs_cur_view.itm.getcol(1));

    TIME_D3D_PROFILE_NAME(render_vsm, "render_distortion");
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_STATIC_DISTORTION);
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_DISTORTION);
  }
  void renderGeomTrans()
  {
    if (renderNoTransp)
      return;
    TIME_D3D_PROFILE_NAME(render_vsm, "render_trans");
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_STATIC_TRANS);
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_TRANS);
  }
  void renderGeomEffects(bool draw_lowres = false)
  {
    if (renderNoTransp)
      return;
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(draw_lowres ? IRenderingService::STG_RENDER_FX_LOWRES : IRenderingService::STG_RENDER_FX);
  }
  void renderGeomDistortionFx()
  {
    if (renderNoTransp)
      return;
    TIME_D3D_PROFILE_NAME(render_vsm, "render_distortion_fx");
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_FX_DISTORTION);
  }
  void renderGeomForShadows()
  {
    TIME_D3D_PROFILE_NAME(render_vsm, "render_shadows");
    GeomObject::isRenderingShadows = true;
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_SHADOWS);

    ec_camera_elem::carCameraElem->externalRender();

    GeomObject::isRenderingShadows = false;
  }

  void renderGeomForWireframe()
  {
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_STATIC_OPAQUE);
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);

    if (renderNoTransp)
      return;
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_STATIC_TRANS);
    for (int i = 0; i < rendSrv.size(); i++)
      rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_TRANS);
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

    for (int i = 0; i < rendSrv.size(); i++)
    {
      rendSrv[i]->renderUI();
    }

    imgui_perform_registered(/*with_menu_bar = */ false);
    imgui_endframe();
    imgui_render();
  }

  void initVsm()
  {
    if (!vsm_allowed)
      return;
    vsm.init(vsmSz, vsmSz, vsmType);
    // vsm.box_update_threshold = (updateVSMThreshold*20000);
    // vsm.box_update_threshold *= vsm.box_update_threshold;
    // vsm.box_full_update_threshold = vsm.box_update_threshold*4;
    vsm.forceUpdate();
  }

  void initClassic()
  {
    initCommon();

    allowDynamicRender = false;
  }

  TEXTUREID characterMicrodetailsId = BAD_TEXTUREID;

  void initDeferred(const DataBlock &blk, const DataBlock *shadow_preset_blk)
  {
    renderEnvironmentFirst = false;
    enviCubeVarName = blk.getStr("enviCubeVarName", "local_light_probe_tex");

    int fmt = parseTexFmt(blk.getStr("lightProbeFmt", "A16B16G16R16F"), TEXFMT_A16B16G16R16F);

    enviProbe = light_probe::create("envi", blk.getInt("lightProbeSz", 128), fmt);

    deferredRtFmt = parseTexFmt(blk.getStr("sceneFmt", "A2B10G10R10"), TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE);
    postfxRtFmt = parseTexFmt(blk.getStr("postfxFmt", "ARGB8"), TEXFMT_A8R8G8B8);
    srgb_backbuf_wr = blk.getBool("postfx_sRGB_backbuf_write", false);

    deferred_mrt_cnt = blk.getInt("gbufCount", 0);
    if (deferred_mrt_cnt > 0 && deferred_mrt_cnt < 8)
    {
      for (int i = 0; i < deferred_mrt_cnt; i++)
        deferred_mrt_fmts[i] = parseTexFmt(blk.getStr(String(0, "gbufFmt%d", i), "ARGB8"), TEXFMT_A8R8G8B8);
    }
    else
    {
      deferred_mrt_cnt = 3;
      deferred_mrt_fmts[0] = TEXFMT_DEFAULT | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
      deferred_mrt_fmts[1] = TEXFMT_A2B10G10R10;
      deferred_mrt_fmts[2] = TEXFMT_A8R8G8B8;
    }
    for (int i = 0; i < deferred_mrt_cnt; i++)
      if (!(d3d::get_texformat_usage(deferred_mrt_fmts[i]) & d3d::USAGE_RTARGET))
      {
        logerr("deferred_mrt_fmts[%d]=%d -> %d", i, deferred_mrt_fmts[i], TEXFMT_DEFAULT);
        deferred_mrt_fmts[i] = TEXFMT_DEFAULT;
      }

    pfx.hdrMode = blk.getStr("hdrMode", "none");
    pfx.hdrInstantAdaptation = blk.getBool("hdrInstantAdaptation", false);
    setPfxLevelBlk(&blk);
    debug("hdrMode=%s instantAdaptation=%d", pfx.hdrMode, (int)pfx.hdrInstantAdaptation);

    initDeferredShadows(shadow_preset_blk ? *shadow_preset_blk : blk);
    initCommon();
    downsample_depth::init("downsample_depth2x");
    volFogCallback.init();
    noPfxResolve.init("deferred_no_postfx_resolve", true);
    preIntegratedGF = render_preintegrated_fresnel_GGX("preIntegratedGF", PREINTEGRATE_SPECULAR_DIFFUSE_QUALITY_MAX);
    reloadMicroDetails(blk);

    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_BIAS);
    state.zBias = wireframeZBias;
    state.set(shaders::OverrideState::BLEND_SRC_DEST);
    state.sblend = BLEND_ZERO;
    state.dblend = BLEND_ZERO;
    wireframeState = shaders::overrides::create(state);
    wireframeRenderer.init("apply_wireframe", true);
  }

  void initDeferredShadows(const DataBlock &blk)
  {
    CascadeShadows::Settings csmSettings;
    csmSettings.cascadeWidth = blk.getInt("csmSize", 512);
    csmSettings.splitsW = blk.getInt("csmW", 2);
    csmSettings.splitsH = blk.getInt("csmH", 2);
    csmSettings.shadowFadeOut = 10.0f;
    csmSettings.fadeOutMul = 1.0f;
    csmMinSparseDist = blk.getReal("csmMinSparseDist", 100000);
    csmMinSparseFrame = blk.getReal("csmMinSparseFrame", -1000);
    csmLambda = blk.getReal("csmLambda", 0.8);
    csmMaxDist = blk.getReal("csmMaxDist", 100);
    deferredCsm = CascadeShadows::make(this, csmSettings);

    vsmSz = blk.getInt("vsmSize", 1024);
    vsmMaxDist = blk.getReal("vsmMaxDist", 1000.0f);
    vsmType = vsm.VSM_BLEND;
    initVsm();
    DAEDITOR3.conNote("initDeferredShadows: CSM(%d,%dx%d,%.0fm,%.2f) VSM(%d,%.0fm)", csmSettings.cascadeWidth, csmSettings.splitsW,
      csmSettings.splitsH, csmMaxDist, csmLambda, vsmSz, vsmMaxDist);

    // prepare cascades at least once after init
    CascadeShadows::ModeSettings mode;
    mode.powWeight = csmLambda;
    mode.maxDist = csmMaxDist;
    mode.shadowStart = 1;
    mode.numCascades = 4;
    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    deferredCsm->prepareShadowCascades(mode, Point3(0, -1, 0), ::grs_cur_view.tm, ::grs_cur_view.pos, projTm, Frustum(globtm),
      Point2(1, 15000), 1.f);

    fomZDistance = blk.getInt("fom_z_distance", 80);
    fomXyViewBoxSize = blk.getInt("fom_xy_distance", 128);
    fomZViewBoxSize = blk.getInt("fom_height", 128);
    fomTexSize = blk.getInt("fom_tex_sz", 256);
    fomShadowsCos = dag::create_tex(nullptr, fomTexSize, fomTexSize, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "fom_shadows_cos");
    fomShadowsSin = dag::create_tex(nullptr, fomTexSize, fomTexSize, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "fom_shadows_sin");
  }

  void initCommon()
  {
    GeomObject::initWater();
    windEffect.initWind(*dgs_get_game_params()->getBlockByNameEx("leavesWind"));
    worldViewPosVarId = ::get_shader_variable_id("world_view_pos");
    resolvedDepthRenderer.init("intz_scene_to_float", true);
  }

  void updateEnviProbe()
  {
    float cur_exposure = getExposure();
    setExposure(1.0f);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    static int zn_zfarVarId = get_shader_variable_id("zn_zfar", true);
    static int light_probe_posVarId = get_shader_variable_id("light_probe_pos", true);
    const float zn = 0.5, zf = 100;

    ShaderGlobal::set_color4(worldViewPosVarId, 0, 0, 0, 1);
    ShaderGlobal::set_color4(zn_zfarVarId, zn, zf, 0, 0);
    ShaderGlobal::set_color4(light_probe_posVarId, 0, 0, 0, 1);
    if (!enviProbeBlack)
    {
      enviProbeBlack = light_probe::create("enviBlack", 4, TEXFMT_A16B16G16R16F);
      for (int i = 0; i < 6; ++i)
      {
        d3d::setpersp(Driver3dPerspective(1, 1, zn, zf, 0, 0));
        TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
        d3d::settm(TM_VIEW, inverse(cameraMatrix));

        d3d::set_render_target(light_probe::getManagedTex(enviProbeBlack)->getCubeTex(), i, 0);
        d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
      }
      light_probe::update(enviProbeBlack, NULL);
    }

    Point3 last_pos = ::grs_cur_view.pos;
    ::grs_cur_view.pos.zero();

    carray<int, SphHarmCalc::SPH_COUNT> enviSPHVarId;
    for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
      enviSPHVarId[i] = get_shader_variable_id(String(128, "enviSPH%d", i));
    for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
      ShaderGlobal::set_color4(enviSPHVarId[i], 0, 0, 0, 0);

    preparing_light_probe = true;
    for (int i = 0; i < 6; ++i)
    {
      d3d::setpersp(Driver3dPerspective(1, 1, zn, zf, 0, 0));
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
      d3d::settm(TM_VIEW, inverse(cameraMatrix));

      d3d::set_render_target(light_probe::getManagedTex(enviProbe)->getCubeTex(), i, 0);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      renderGeomEnvi();
    }
    preparing_light_probe = false;
    light_probe::update(enviProbe, NULL);

    float gamma = 1.0f;
    if (light_probe::calcDiffuse(enviProbe, NULL, gamma, 1.0f, true))
    {
      const Color4 *sphHarm = light_probe::getSphHarm(enviProbe);
      for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
        ShaderGlobal::set_color4(enviSPHVarId[i], sphHarm[i]);
    }
    else
      logerr("Can't calculate harmonics");
    ::grs_cur_view.pos = last_pos;

    d3d::set_render_target();

    setExposure(cur_exposure);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::set_texture(get_shader_variable_id("envi_probe_specular"), *light_probe::getManagedTex(enviProbe));
    ShaderGlobal::set_sampler(get_shader_variable_id("envi_probe_specular_samplerstate"), d3d::request_sampler({}));

    // static int ord = 0;
    // save_cubetex_as_ddsx(light_probe::getManagedTex(enviProbe)->getCubeTex(),
    //   String(0, "D:/dagor2_tools/skyquake/develop/enviProbe.%d.cube.ddsx", ord++));
  }

  void afterD3DReset(bool full_reset)
  {
    if (ssao)
      ssao->reset();
    updateEnviProbe();
  }

  void shutdown()
  {
    shaders::overrides::destroy(wireframeState);
    closeMicroDetails();
    lowresFxTex.close();
    blackTex.close();
    downsampledNormals.close();
    downsampledFarDepth.close();
    checkerboardDepth.close();
    downsampledOpaqueTarget.close();
    tonemapLUT.reset();
    ssr.reset();
    ssao.reset();
    fomShadowsSin.close();
    fomShadowsCos.close();
    if (vsm_allowed)
      vsm.close();

    droplets.close();
    downsample_depth::close();

    imgui_shutdown();
  }

  void setRenderType(int t)
  {
    if (rtype == t)
      return;

    rtype = t;
  }

  void reInitCsm(const DataBlock *preset_blk)
  {
    if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      if (vsm_allowed)
        vsm.close();
      del_it(deferredCsm);
      G_ASSERT(preset_blk);
      initDeferredShadows(*preset_blk);
      initVsm();
      updateCsm();
      return;
    }
  }

  void updateBackBufSize(int w, int h)
  {
    if (!w || !h)
    {
      del_d3dres(sceneRt);
      targetW = 0, targetH = 0;
      sceneFmt = 0;
      return;
    }

    int vrWidth = 0, vrHeight = 0;
    if (auto vrDevice = VRDevice::getInstance())
      vrDevice->getViewResolution(vrWidth, vrHeight);

    bool needVRInit = vrWidth && !vrResources.sceneRt;

    if (sceneRt && !needVRInit && w == targetW && h == targetH && sceneFmt == hdr_render_format)
      return;

    del_d3dres(sceneRt);
    del_d3dres(postfxRt);

    vrResources.teardown();

    {
      TextureInfo ti;
      ti.w = ti.h = 0;

      if (combinedShadowsTex)
        combinedShadowsTex->getinfo(ti);

      if (ti.w != w || ti.h != h)
        combinedShadowsTex.close();

      if (!combinedShadowsTex)
      {
        if (VariableMap::isGlobVariablePresent(get_shader_variable_id("combined_shadows", true)))
          combinedShadowsTex = dag::create_tex(nullptr, w, h, TEXFMT_L8 | TEXCF_RTARGET, 1, "combined_shadows");
        else
          debug("'%s' var not present, %s is not used", "combined_shadows", "combinedShadowsTex");
      }

      if (vrWidth && !vrResources.combinedShadowsTex)
        if (VariableMap::isGlobVariablePresent(get_shader_variable_id("combined_shadows", true)))
          vrResources.combinedShadowsTex = UniqueTexHolder(
            dag::create_tex(nullptr, vrWidth, vrHeight, TEXFMT_L8 | TEXCF_RTARGET, 1, "vr_combined_shadows"), "combined_shadows");
    }

    if (vrWidth)
    {
      vrResources.imguiTex = dag::create_tex(nullptr, w, h, TEXCF_RTARGET, 1, "imgui_tex");
      vrgui::destroy_surface();
      vrgui::init_surface(w, h, vrgui::SurfaceCurvature::VRGUI_SURFACE_PLANE);
    }

    if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      deferredTarget.reset();
      ssao.reset();
      ssr.reset();
      lowresFxTex.close();
      downsampledNormals.close();
      downsampledOpaqueTarget.close();
      downsampledFarDepth.close();
      checkerboardDepth.close();
      volFogCallback.depthId = BAD_TEXTUREID;
    }

    static bool loggingEnabled = dgs_get_settings()->getBlockByNameEx("debug")->getBool("view_resizing_related_logging_enabled", true);
    if (loggingEnabled)
      debug("recreate target %dx%d - > %dx%d (fmt=%p)", targetW, targetH, w, h, hdr_render_format);

    targetW = w, targetH = h;
    sceneFmt = hdr_render_format;

    static int screen_pos_to_texcoordVarId = ::get_shader_variable_id("screen_pos_to_texcoord");
    ShaderGlobal::set_color4(screen_pos_to_texcoordVarId, 1.f / w, 1.f / h, 0, 0);
    static int lowres_tex_sizeVarId = ::get_shader_variable_id("lowres_tex_size");
    ShaderGlobal::set_color4(lowres_tex_sizeVarId, w / 2, h / 2, 1.0f / (w / 2), 1.0f / (h / 2));

    if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      sceneRt = d3d::create_tex(NULL, w, h, TEXCF_RTARGET | deferredRtFmt, 1);
      postfxRt = d3d::create_tex(NULL, w, h, TEXCF_RTARGET | postfxRtFmt, 1);

      if (vrWidth)
      {
        vrResources.sceneRt = dag::create_tex(NULL, vrWidth, vrHeight, TEXCF_RTARGET | deferredRtFmt, 1, "vr_sceneRt");
        vrResources.postfxRt = dag::create_tex(NULL, vrWidth, vrHeight, TEXCF_RTARGET | postfxRtFmt, 1, "vr_postfxRt");
      }
    }
    else
    {
      sceneRt = d3d::create_tex(NULL, w, h, TEXCF_RTARGET | hdr_render_format, 1);
      postfxRt = d3d::create_tex(NULL, w, h, TEXCF_RTARGET | postfxRtFmt, 1);
      fallbackSceneDepth = dag::create_tex(nullptr, w, h, TEXCF_RTARGET | TEXFMT_DEPTH32, 1, "scene_depth");
    }
    resolvedDepth = dag::create_tex(nullptr, w, h, TEXCF_RTARGET | TEXFMT_R32F, 1, "depth_tex");

    if (vrWidth)
      vrResources.resolvedDepth = dag::create_tex(nullptr, vrWidth, vrHeight, TEXCF_RTARGET | TEXFMT_R32F, 1, "depth_tex_vr");

    if (VariableMap::isGlobVariablePresent(get_shader_variable_id("wireframe_tex", true)))
      wireframeTex = dag::create_tex(nullptr, w, h, TEXCF_RTARGET | TEXFMT_A8R8G8B8, 1, "wireframe_tex");
    else
      debug("'%s' var not present, %s is not used", "wireframe_tex", "wireframeTex");

    DataBlock *b = gameParamsBlk.addBlock("postfx_rt");
    b->setInt("use_w", targetW);
    b->setInt("use_h", targetH);
    b->setInt("preserve_adapt_val", 1);
    if (postFx)
    {
      if (disable_postfx)
        pfx.hdrMode = "none";
      postFx->restart(pfxLevelBlk, &gameParamsBlk, &pfx);
      if (postFx->getDemonPostFx())
        postFx->getDemonPostFx()->setVolFogCallback(&volFogCallback);
    }

    ShaderGlobal::reset_textures(true);

    if (sceneRtId == BAD_TEXTUREID)
      sceneRtId = ::register_managed_tex("@:sceneRt", sceneRt);
    else
      ::change_managed_texture(sceneRtId, sceneRt);
    if (postfxRtId == BAD_TEXTUREID)
      postfxRtId = ::register_managed_tex("@:postfxRt", postfxRt);
    else
      ::change_managed_texture(postfxRtId, postfxRt);

    if (rtype == RTYPE_DYNAMIC_DEFERRED)
    {
      deferredTarget = eastl::make_unique<DeferredRenderTarget>("deferred_shadow_to_buffer", "main", targetW, targetH,
        DeferredRT::StereoMode::MonoOrMultipass, 0, deferred_mrt_cnt, deferred_mrt_fmts, TEXFMT_DEPTH32);

      if (loggingEnabled)
        debug("deferredRender: reinit for %dx%d", targetW, targetH);

      if (vrWidth)
        vrResources.deferredTarget = eastl::make_unique<DeferredRenderTarget>("deferred_shadow_to_buffer", "vr", vrWidth, vrHeight,
          DeferredRT::StereoMode::MonoOrMultipass, 0, deferred_mrt_cnt, deferred_mrt_fmts, TEXFMT_DEPTH32);

      int halfW = targetW / 2, halfH = h / 2;
      ShaderGlobal::set_color4(::get_shader_variable_id("lowres_rt_params", true), halfW, halfH, 0, 0);

      ssao = eastl::make_unique<SSAORenderer>(targetW / 2, targetH / 2, 1, SSAO_ALLOW_IMPLICIT_FLAGS_FROM_SHADER_ASSUMES);
      ssr = eastl::make_unique<ScreenSpaceReflections>(targetW / 2, targetH / 2);

      if (vrWidth)
        vrResources.ssao = eastl::make_unique<SSAORenderer>(vrWidth / 2, vrHeight / 2, 1,
          SSAO_IMMEDIATE | SSAO_ALLOW_IMPLICIT_FLAGS_FROM_SHADER_ASSUMES, true, "vr_");

      uint16_t blackImg[4] = {1, 1, 0, 0};
      if (!blackTex)
        blackTex = dag::create_tex((TexImage32 *)blackImg, 1, 1, TEXCF_LOADONCE | TEXCF_SYSTEXCOPY, 1, "black_tex1x1");

      downsampledNormals = dag::create_tex(nullptr, targetW / 2, targetH / 2, TEXCF_RTARGET, 1, "downsampled_normals");
      downsampledNormals->texfilter(TEXFILTER_POINT);
      downsampledNormals->texaddr(TEXADDR_CLAMP);
      downsampledNormals.setVar();

      uint32_t fmt = TEXFMT_A16B16G16R16F;
      downsampledOpaqueTarget = dag::create_tex(nullptr, targetW / 2, targetH / 2, fmt | TEXCF_RTARGET, 1, "prev_frame_tex");
      downsampledOpaqueTarget->texaddr(TEXADDR_CLAMP);
      downsampledOpaqueTarget.setVar();

      lowresFxTex = dag::create_tex(nullptr, targetW / 2, targetH / 2, fmt | TEXCF_RTARGET, 1, "low_res_fx_rt");

      downsampledFarDepth =
        dag::create_tex(nullptr, targetW / 2, targetH / 2, TEXCF_RTARGET | TEXFMT_R32F, 1, "downsampled_far_depth_tex");
      downsampledFarDepth->texfilter(TEXFILTER_POINT);
      downsampledFarDepth->texaddr(TEXADDR_BORDER);
      downsampledFarDepth->texbordercolor(0xFFFFFFFF);
      downsampledFarDepth.setVar();

      if (vrWidth)
      {
        vrResources.downsampledFarDepth = UniqueTexHolder(
          dag::create_tex(nullptr, vrWidth / 2, vrHeight / 2, TEXCF_RTARGET | TEXFMT_R32F, 1, "downsampled_far_depth_tex_vr"),
          "downsampled_far_depth_tex");
        vrResources.downsampledFarDepth->texfilter(TEXFILTER_POINT);
        vrResources.downsampledFarDepth->texaddr(TEXADDR_BORDER);
        vrResources.downsampledFarDepth->texbordercolor(0xFFFFFFFF);
        vrResources.downsampledFarDepth.setVar();
      }

      checkerboardDepth =
        dag::create_tex(nullptr, targetW / 2, targetH / 2, TEXCF_RTARGET | TEXFMT_DEPTH32, 1, "downsampled_checkerboard_depth_tex");
      checkerboardDepth->texfilter(TEXFILTER_POINT);
      checkerboardDepth->texaddr(TEXADDR_BORDER);
      checkerboardDepth->texbordercolor(0xFFFFFFFF);
      checkerboardDepth.setVar();

      upscaleSamplingRenderer.reset();
      Ptr<ShaderMaterial> upscale_mat = new_shader_material_by_name_optional("upscale_sampling");
      if (upscale_mat)
      {
        upscaleSamplingRenderer = eastl::make_unique<UpscaleSamplingTex>(targetW, targetH);

        if (vrWidth)
          vrResources.upscaleSamplingRenderer = eastl::make_unique<UpscaleSamplingTex>(vrWidth, vrHeight, "vr_");
      }

      applyLowResFx.init(upscale_mat ? "apply_lowres_fx" : "fast_apply_lowres_fx", /*optional*/ true);
    }
  }

  void setEnviReqSceneBlk(bool req) { enviReqSceneBlk = req; }
  void updateVsm()
  {
    if (rtype == RTYPE_CLASSIC)
      return;
    if (!vsm_allowed)
      return;

    if (!renderShadowVsm || !render_shadow())
      vsm.setOff();
    vsm.forceUpdate();
  }
  void updateCsm()
  {
    if (rtype == RTYPE_CLASSIC)
      return;

    if (render_shadow())
      ddsx::tex_pack2_perform_delayed_data_loading();
    rendinst::set_global_shadows_needed(render_shadow());
    if (!render_shadow())
      if (rtype == RTYPE_DYNAMIC_DEFERRED)
        deferredCsm->renderShadowsCascades();
    updateVsm();
  }

  void setEnvironmentSnapshot(const char *blk_fn, bool render_cubetex_from_snapshot)
  {
    Tab<TEXTUREID> prev_id;
    for (int i = enviBlk.paramCount() - 1; i >= 0; i--)
      if (enviBlk.getParamType(i) == DataBlock::TYPE_STRING)
      {
        TEXTUREID id = get_managed_texture_id(enviBlk.getStr(i));
        if (id == BAD_TEXTUREID)
          continue;
        ShaderGlobal::set_texture_fast(get_shader_glob_var_id(enviBlk.getParamName(i), true), BAD_TEXTUREID);
        prev_id.push_back(id);
      }

    // process environment snapshot
    if (blk_fn && *blk_fn)
    {
      String loc_path(blk_fn);
      enviBlk.load(loc_path);
      location_from_path(loc_path);

      for (int i = enviBlk.paramCount() - 1; i >= 0; i--)
        if (enviBlk.getParamType(i) == DataBlock::TYPE_STRING)
        {
          const char *fn = enviBlk.getStr(i);
          if (!strstr(fn, ".ddsx"))
          {
            enviBlk.removeParam(i);
            continue;
          }
          String full_fn(0, "%s/%s", loc_path, fn);
          dd_simplify_fname_c(full_fn);
          if (dd_file_exists(full_fn))
          {
            enviBlk.setStr(i, full_fn);
            TEXTUREID texId = get_managed_texture_id(full_fn);
            if (texId == BAD_TEXTUREID)
              texId = add_managed_texture(full_fn);
            acquire_managed_tex(texId);
          }
          else
            enviBlk.removeParam(i);
        }
    }
    else
      enviBlk.reset();
    for (int i = 0; i < prev_id.size(); i++)
      release_managed_tex(prev_id[i]);

    enviSlp.color = E3DCOLOR(255, 255, 255, 255);
    enviSlp.brightness = 0;
    enviSlp.angSize = 0;
    enviSlp.enabled = true;
    enviSlp.specPower = 1;
    enviSlp.specStrength = 1;
    enviSlp.ltDir.set_xyz(-enviBlk.getPoint4("sun_light_dir_0", Point4(0, -1, 0, 0)));
    Point4 col = enviBlk.getPoint4("sun_color_0", Point4(0, 0, 0, 0));
    enviSlp.ltCol.set(col.x, col.y, col.z);

    Point2 a = dir_to_sph_ang(enviSlp.ltDir);
    enviSlp.azimuth = a.x;
    enviSlp.zenith = a.y;

    const char *cubetex_nm = enviBlk.getStr(enviCubeVarName, NULL);
    if (render_cubetex_from_snapshot && cubetex_nm && *cubetex_nm)
    {
      TEXTUREID tid = add_managed_texture(cubetex_nm);
      BaseTexture *t = acquire_managed_tex(tid);
      if (t && t->restype() != RES3D_CUBETEX)
      {
        release_managed_tex(tid);
        t = NULL;
      }
      if (t)
        release_managed_tex(enviCubeTexId);
      enviCubeTexId = t ? tid : BAD_TEXTUREID;
    }
    else if (enviCubeTexId != BAD_TEXTUREID)
    {
      release_managed_tex(enviCubeTexId);
      enviCubeTexId = BAD_TEXTUREID;
    }
  }

  bool hasEnvironmentSnapshot() { return enviBlk.paramCount() > 0; }
  void applyEnvironmentSnapshot()
  {
    if (ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>())
    {
      if (ltSrv->getSunCount() < 1)
        ltSrv->setSunCount(1);
      *ltSrv->getSun(0) = enviSlp;
    }
    ltSun0 = enviSlp;
    ShaderGlobal::set_vars_from_blk(gameGlobVars);
    ShaderGlobal::set_vars_from_blk(enviBlk, true);
  }

  void renderEnviCubeTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd)
  {
    if (!tex || preparing_light_probe)
      return;
    if (!cubeData.inited())
      prepareCubeData();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // since we change render states and constants/samplers
    tex->texaddr(TEXADDR_CLAMP);
    shaders::overrides::set(noCullStateId);
    shaders::render_states::set(defaultRenderStateId);
    d3d::settex(0, tex);

    TMatrix cubeWtm;
    cubeWtm.identity();
    cubeWtm.setcol(3, ::grs_cur_view.pos);
    d3d::settm(TM_WORLD, cubeWtm);

    TMatrix4_vec4 gtm;
    d3d::getglobtm(gtm);
    process_tm_for_drv_consts(gtm);
    d3d::set_vs_const(0, gtm[0], 4);
    d3d::set_ps_const1(0, cMul.r, cMul.g, cMul.b, cMul.a);
    d3d::set_ps_const1(1, cAdd.r, cAdd.g, cAdd.b, cAdd.a);

    d3d::set_program(cubeData.prog);
    d3d::setvsrc(0, cubeData.vb, sizeof(Point3));
    d3d::setind(cubeData.ib);
    d3d::drawind(PRIM_TRILIST, 0, 12, 0);

    d3d::setvdecl(BAD_VDECL);
    d3d::set_program(BAD_PROGRAM);
    d3d::settex(0, NULL);
    shaders::overrides::reset();
  }
  void renderEnviBkgTexture(BaseTexture *tex, const Color4 &scales)
  {
    if (!tex || preparing_light_probe)
      return;
    if (bkgData.prog == BAD_PROGRAM)
      prepareBkgData();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // since we change render states and constants/samplers
    d3d::set_vs_const1(0, scales.r, scales.g, scales.b, scales.a);
    shaders::overrides::set(noCullStateId);
    shaders::render_states::set(defaultRenderStateId);
    d3d::settex(0, tex);

    d3d::set_program(bkgData.prog);
    d3d::setvsrc(0, bkgData.vb, sizeof(Point2));
    d3d::draw(PRIM_TRISTRIP, 0, 2);

    d3d::set_program(BAD_PROGRAM);
    d3d::settex(0, NULL);
    shaders::overrides::reset();
  }
  void renderEnviVolTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd, const Color4 &scales, float tz)
  {
    if (!tex || preparing_light_probe)
      return;
    if (voltexData.prog == BAD_PROGRAM)
      prepareVolTexData();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // since we change render states and constants/samplers
    d3d::set_ps_const1(0, cMul.r, cMul.g, cMul.b, cMul.a);
    d3d::set_ps_const1(1, cAdd.r, cAdd.g, cAdd.b, cAdd.a);
    d3d::set_ps_const1(2, tz, 0, 0, 0);
    d3d::set_vs_const1(0, scales.r, scales.g, scales.b, scales.a);
    shaders::overrides::set(noCullStateId);
    shaders::render_states::set(defaultRenderStateId);
    d3d::settex(0, tex);

    d3d::set_program(voltexData.prog);
    d3d::setvsrc(0, voltexData.vb, sizeof(Point2));
    d3d::draw(PRIM_TRISTRIP, 0, 2);

    d3d::set_program(BAD_PROGRAM);
    d3d::settex(0, NULL);
    shaders::overrides::reset();
  }

  void prepareCubeData()
  {
    // DX9 HLSL
    static const char *vs_hlsl = "struct VSInput  { float3 pos:POSITION; };\n"
                                 "struct VSOutput { float4 pos:POSITION; float3 tc:TEXCOORD0; };\n"
                                 "float4x4 globTm: register(c0);\n"
                                 "VSOutput vs_main(VSInput input)\n"
                                 "{\n"
                                 "  VSOutput output;\n"
                                 "  output.pos = mul(float4(input.pos, 1), globTm); output.pos.z = 0;\n"
                                 "  output.tc  = input.pos;\n"
                                 "  return output;\n"
                                 "}\n";
    static const char *ps_hlsl = "struct VSOutput { float4 pos:POSITION; float3 tc:TEXCOORD0; };\n"
                                 "samplerCUBE tex : register(s0);\n"
                                 "float4 cMul: register(c0);\n"
                                 "float4 cAdd: register(c1);\n"
                                 "float4 ps_main(VSOutput input):COLOR0 {\n"
                                 "  float4 c = texCUBE(tex, input.tc);\n"
                                 "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                 "}\n";

    // DX11 HLSL
    static const char *vs_hlsl11 = "struct VSInput  { float3 pos:POSITION; };\n"
                                   "struct VSOutput { float4 pos:SV_POSITION; float3 tc:TEXCOORD0; };\n"
                                   "float4x4 globTm: register(c0);\n"
                                   "VSOutput vs_main(VSInput input)\n"
                                   "{\n"
                                   "  VSOutput output;\n"
                                   "  output.pos = mul(float4(input.pos, 1), globTm); output.pos.z = 0;\n"
                                   "  output.tc  = input.pos;\n"
                                   "  return output;\n"
                                   "}\n";
    static const char *ps_hlsl11 = "struct VSOutput { float4 pos:SV_POSITION; float3 tc:TEXCOORD0; };\n"
                                   "TextureCube tex: register(t0);\n"
                                   "SamplerState tex_samplerstate : register(s0);\n"
                                   "float4 cMul: register(c0);\n"
                                   "float4 cAdd: register(c1);\n"
                                   "float4 ps_main(VSOutput input):SV_Target0 {\n"
                                   "  float4 c = tex.Sample(tex_samplerstate, input.tc);\n"
                                   "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                   "}\n";

    static VSDTYPE dcl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_END};

    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" : "ps_3_0");
    VDECL vd = d3d::create_vdecl(dcl);
    cubeData.prog = d3d::create_program(vs, ps, vd);
    d3d::set_program(cubeData.prog);

    cubeData.vb = d3d::create_vb(sizeof(Point3) * 8, 0);
    cubeData.ib = d3d::create_ib(sizeof(short) * 3 * 12, 0);
    debug("cubeData: prog=%d, vs=%d, ps=%d, vd=%d vb=%p ib=%p", cubeData.prog, vs, ps, vd, cubeData.vb, cubeData.ib);

    {
      Point3 *vp;
      d3d_err(cubeData.vb->lockEx(0, 0, &vp, VBLOCK_WRITEONLY));
      vp[0].set(-0.5, -0.5, -0.5);
      vp[1].set(+0.5, -0.5, -0.5);
      vp[2].set(-0.5, +0.5, -0.5);
      vp[3].set(+0.5, +0.5, -0.5);
      vp[4].set(-0.5, -0.5, +0.5);
      vp[5].set(+0.5, -0.5, +0.5);
      vp[6].set(-0.5, +0.5, +0.5);
      vp[7].set(+0.5, +0.5, +0.5);
      d3d_err(cubeData.vb->unlock());
    }
    {
      unsigned short *ip;
      d3d_err(cubeData.ib->lock(0, 0, &ip, VBLOCK_WRITEONLY));
      int ind = 0;
      ip[ind++] = 0;
      ip[ind++] = 1;
      ip[ind++] = 2;
      ip[ind++] = 1;
      ip[ind++] = 3;
      ip[ind++] = 2;

      ip[ind++] = 4;
      ip[ind++] = 5;
      ip[ind++] = 6;
      ip[ind++] = 5;
      ip[ind++] = 7;
      ip[ind++] = 6;

      ip[ind++] = 0;
      ip[ind++] = 4;
      ip[ind++] = 2;
      ip[ind++] = 2;
      ip[ind++] = 4;
      ip[ind++] = 6;

      ip[ind++] = 1;
      ip[ind++] = 3;
      ip[ind++] = 5;
      ip[ind++] = 3;
      ip[ind++] = 7;
      ip[ind++] = 5;

      ip[ind++] = 0;
      ip[ind++] = 4;
      ip[ind++] = 1;
      ip[ind++] = 1;
      ip[ind++] = 4;
      ip[ind++] = 5;

      ip[ind++] = 2;
      ip[ind++] = 6;
      ip[ind++] = 3;
      ip[ind++] = 3;
      ip[ind++] = 6;
      ip[ind++] = 7;

      d3d_err(cubeData.ib->unlock());
    }
  }

  void prepareBkgData()
  {
    // DX9 HLSL
    static const char *vs_hlsl = "struct VSInput  { float2 pos:POSITION; };\n"
                                 "struct VSOutput { float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "float4 tcMA: register(c0);\n"
                                 "VSOutput vs_main(VSInput input)\n"
                                 "{\n"
                                 "  VSOutput output;\n"
                                 "  output.pos = float4(input.pos, 0, 1);\n"
                                 "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                 "  return output;\n"
                                 "}\n";
    static const char *ps_hlsl = "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "sampler tex : register(s0);\n"
                                 "float4 ps_main(VSOutput input):COLOR0 { return float4(tex2D(tex, input.tc).rgb, 1); }\n";

    // DX11 HLSL
    static const char *vs_hlsl11 = "struct VSInput  { float2 pos:POSITION; };\n"
                                   "struct VSOutput { float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
                                   "float4 tcMA: register(c0);\n"
                                   "VSOutput vs_main(VSInput input)\n"
                                   "{\n"
                                   "  VSOutput output;\n"
                                   "  output.pos = float4(input.pos, 0, 1);\n"
                                   "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                   "  return output;\n"
                                   "}\n";

    static const char *ps_hlsl11 =
      "struct VSOutput{float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
      "Texture2D tex: register(t0);\n"
      "SamplerState tex_samplerstate : register(s0);\n"
      "float4 ps_main(VSOutput input):SV_Target0 { return float4(tex.Sample(tex_samplerstate, input.tc).rgb, 1); }\n";

    static VSDTYPE dcl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END};

    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" : "ps_3_0");
    VDECL vd = d3d::create_vdecl(dcl);
    bkgData.prog = d3d::create_program(vs, ps, vd);

    bkgData.vb = d3d::create_vb(sizeof(Point2) * 4, 0);
    debug("bkgData: prog=%d, vs=%d, ps=%d, vd=%d vb=%p", bkgData.prog, vs, ps, vd, bkgData.vb);

    {
      Point2 *vp;
      d3d_err(bkgData.vb->lock(0, 0, (void **)&vp, VBLOCK_WRITEONLY));
      vp[0].set(-1, -1);
      vp[1].set(-1, +1);
      vp[2].set(+1, -1);
      vp[3].set(+1, +1);
      d3d_err(bkgData.vb->unlock());
    }
  }

  void prepareVolTexData()
  {
    // DX9 HLSL
    static const char *vs_hlsl = "struct VSInput  { float2 pos:POSITION; };\n"
                                 "struct VSOutput { float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "float4 tcMA: register(c0);\n"
                                 "VSOutput vs_main(VSInput input)\n"
                                 "{\n"
                                 "  VSOutput output;\n"
                                 "  output.pos = float4(input.pos, 0, 1);\n"
                                 "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                 "  return output;\n"
                                 "}\n";
    static const char *ps_hlsl = "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "sampler3D tex : register(s0);\n"
                                 "float4 cMul: register(c0);\n"
                                 "float4 cAdd: register(c1);\n"
                                 "float4 cTcZ: register(c2);\n"
                                 "float4 ps_main(VSOutput input):COLOR0 {\n"
                                 "  float4 c = tex3D(tex, float3(input.tc, cTcZ.x));\n"
                                 "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                 "}\n";

    // DX11 HLSL
    static const char *vs_hlsl11 = "struct VSInput  { float2 pos:POSITION; };\n"
                                   "struct VSOutput { float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
                                   "float4 tcMA: register(c0);\n"
                                   "VSOutput vs_main(VSInput input)\n"
                                   "{\n"
                                   "  VSOutput output;\n"
                                   "  output.pos = float4(input.pos, 0, 1);\n"
                                   "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                   "  return output;\n"
                                   "}\n";

    static const char *ps_hlsl11 = "struct VSOutput { float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
                                   "Texture3D tex: register(t0);\n"
                                   "SamplerState tex_samplerstate : register(s0);\n"
                                   "float4 cMul: register(c0);\n"
                                   "float4 cAdd: register(c1);\n"
                                   "float4 cTcZ: register(c2);\n"
                                   "float4 ps_main(VSOutput input):SV_Target0 {\n"
                                   "  float4 c = tex.Sample(tex_samplerstate, float3(input.tc, cTcZ.x));\n"
                                   "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                   "}\n";

    static VSDTYPE dcl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END};

    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" : "ps_3_0");
    VDECL vd = d3d::create_vdecl(dcl);
    voltexData.prog = d3d::create_program(vs, ps, vd);

    voltexData.vb = d3d::create_vb(sizeof(Point2) * 4, 0);
    debug("voltexData: prog=%d, vs=%d, ps=%d, vd=%d vb=%p", voltexData.prog, vs, ps, vd, voltexData.vb);

    {
      Point2 *vp;
      d3d_err(voltexData.vb->lock(0, 0, (void **)&vp, VBLOCK_WRITEONLY));
      vp[0].set(-1, -1);
      vp[1].set(-1, +1);
      vp[2].set(+1, -1);
      vp[3].set(+1, +1);
      d3d_err(voltexData.vb->unlock());
    }
  }

  void setPfxLevelBlk(const DataBlock *b)
  {
    pfxLevelBlk = b;
    createPostFx();
  }
  void createPostFx()
  {
    if (postFx)
      return;
    if (disable_postfx)
      pfx.hdrMode = "none";
    postFx = new PostFx(*pfxLevelBlk, gameParamsBlk, pfx);
    if (postFx->getDemonPostFx())
      postFx->getDemonPostFx()->setVolFogCallback(&volFogCallback);
  }

  void getPostFxSettings(DemonPostFxSettings &set)
  {
    if (postFx && postFx->getDemonPostFx())
      postFx->getDemonPostFx()->getSettings(set);
    else
      memset(&set, 0, sizeof(set));
  }
  void setPostFxSettings(DemonPostFxSettings &set)
  {
    if (postFx && postFx->getDemonPostFx())
      postFx->getDemonPostFx()->setSettings(set);
  }

  bool hasExposure() const { return exposureBuffer.getBufId() != BAD_TEXTUREID; }

  float getExposure() const { return exposure; }

  void setExposure(float exposure_value)
  {
    exposure = exposure_value;
    hasExposureChanged = true;
  }

private:
  bool writeExposure(float exposure_value)
  {
    if (!exposureBuffer.getBuf())
      return false;

    float *exposureDestination;
    if (exposureBuffer.getBuf()->lock(0, 0, (void **)&exposureDestination, VBLOCK_WRITEONLY) && exposureDestination)
    {
      static constexpr float EXPOSURE_MIN_LOG = -12;
      static constexpr float EXPOSURE_MAX_LOG = 3;
      alignas(16) float exposureData[] = {exposure_value, 1.0f / exposure_value, 1.0f, 0.0f, EXPOSURE_MIN_LOG, EXPOSURE_MAX_LOG,
        EXPOSURE_MAX_LOG - EXPOSURE_MIN_LOG, 1.0f / (EXPOSURE_MAX_LOG - EXPOSURE_MIN_LOG)};
      memcpy(exposureDestination, exposureData, EXPOSURE_BUF_SIZE * sizeof(float));
      exposureBuffer.getBuf()->unlock();
      return true;
    }
    else
    {
      debug("dynRender: can't lock buffer for exposure");
      return false;
    }
  }

  enum
  {
    SHADOWS_QUALITY_MAX = 5,
    SSAO_QUALITY_MAX = 3
  };

  PostFx *postFx;
  bool allowDynamicRender;
  bool enviReqSceneBlk;

  SkyLightProps ltSky;
  SunLightProps ltSun0, ltSun1;

  Variance vsm;

  RainDroplets droplets;
  LeavesWindEffect windEffect;
  int worldViewPosVarId;
  int targetW, targetH, sceneFmt;
  Texture *sceneRt, *postfxRt;
  TEXTUREID sceneRtId, postfxRtId;
  UniqueTex fallbackSceneDepth; // For case when we don't have deferred rendering (do we have such case on practice?).

  UniqueTexHolder wireframeTex;
  shaders::OverrideStateId wireframeState;
  PostFxRenderer wireframeRenderer;

  UniqueTex resolvedDepth;
  PostFxRenderer resolvedDepthRenderer;
  UniqueTexHolder preIntegratedGF;

  DataBlock gameParamsBlk;

  eastl::unique_ptr<DeferredRenderTarget> deferredTarget;
  CascadeShadows *deferredCsm;
  PostFxRenderer combinedShadowsRenderer;
  UniqueTexHolder combinedShadowsTex;
  light_probe::Cube *enviProbe;
  light_probe::Cube *enviProbeBlack;
  int deferredRtFmt;
  int postfxRtFmt;
  uint32_t deferred_mrt_fmts[8];
  int deferred_mrt_cnt;
  bool srgb_backbuf_wr;
  eastl::unique_ptr<SSAORenderer> ssao;
  eastl::unique_ptr<ScreenSpaceReflections> ssr;
  UniqueTexHolder downsampledNormals, downsampledOpaqueTarget, downsampledFarDepth, checkerboardDepth;
  UniqueTex lowresFxTex;
  UniqueTex blackTex;
  eastl::unique_ptr<UpscaleSamplingTex> upscaleSamplingRenderer;
  PostFxRenderer applyLowResFx;
  eastl::optional<motion_vector_access::CameraParams> previousCamera;

  PostFxUserSettings pfx;
  const DataBlock *pfxLevelBlk;

  UniqueBufHolder exposureBuffer;
  float exposure = 1.0f;
  bool hasExposureChanged = true;

  struct VolFogCallback : public DemonPostFxCallback
  {
    PostFxRenderer skyPlaneFx;
    TEXTUREID depthId;

    void init()
    {
      skyPlaneFx.init("demon_postfx_sky_plane", true);
      depthId = BAD_TEXTUREID;
    }
    int process(int target_size_x, int target_size_y, Color4 target_coeffs) override
    {
      if (skyPlaneFx.getMat())
      {
        static int depthTexVarId = get_shader_variable_id("depth_tex", true);
        ShaderGlobal::set_texture(depthTexVarId, depthId);
        skyPlaneFx.render();
      }
      else
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      return 0;
    }
  };
  VolFogCallback volFogCallback;
  PostFxRenderer noPfxResolve;


  float csmMinSparseDist;
  float csmMinSparseFrame;
  float csmLambda;
  float csmMaxDist;
  int vsmSz;
  float vsmMaxDist;
  Variance::VsmType vsmType;
  float fomZDistance;
  float fomXyViewBoxSize;
  float fomZViewBoxSize;
  float fomTexSize;
  UniqueTexHolder fomShadowsCos, fomShadowsSin;
  DataBlock enviBlk;
  SunLightProps enviSlp;
  TEXTUREID enviCubeTexId;
  SimpleString enviCubeVarName;

  struct ShaderBufData
  {
    PROGRAM prog;
    Vbuffer *vb;
    Ibuffer *ib;

    ShaderBufData() : prog(BAD_PROGRAM), vb(NULL), ib(NULL) {}

    bool inited() { return prog != BAD_PROGRAM; }
    void clear()
    {
      del_d3dres(vb);
      del_d3dres(ib);
      d3d::delete_program(prog);
      prog = BAD_PROGRAM;
    }
  };
  ShaderBufData cubeData, voltexData, bkgData;

private:
  ///////////////////////////////////////////////////////////////////////////////
  // VR rendering
  ///////////////////////////////////////////////////////////////////////////////

  struct VRRenderingResouces
  {
    eastl::unique_ptr<DeferredRenderTarget> deferredTarget;

    UniqueTex sceneRt, postfxRt;
    UniqueTex resolvedDepth;
    UniqueTex imguiTex;
    UniqueTexHolder downsampledFarDepth;
    UniqueTexHolder combinedShadowsTex;

    eastl::unique_ptr<SSAORenderer> ssao;

    eastl::unique_ptr<UpscaleSamplingTex> upscaleSamplingRenderer;

    void teardown()
    {
      deferredTarget.reset();
      sceneRt.close();
      postfxRt.close();
      resolvedDepth.close();
      imguiTex.close();
      downsampledFarDepth.close();
      combinedShadowsTex.close();
      ssao.reset();
      upscaleSamplingRenderer.reset();
    }
  } vrResources;

  void toggleVrOnDemand()
  {
    if (!VRDevice::getInstance())
    {
      VRDevice::ApplicationData vrAppData;
      vrAppData.name = "Dagor tools";
      vrAppData.version = 1;
      VRDevice::create(VRDevice::RenderingAPI::Default, vrAppData);

      if (auto vrDevice = VRDevice::getInstance()) //-V547
      {
        vrDevice->getAdapterId();
        if (!vrDevice->setRenderingDevice())
          VRDevice::deleteInstance();
      }
    }
    else
    {
      VRDevice::deleteInstance();
      vrResources.teardown();
      vrgui::destroy_surface();
    }

    const bool vrModeActive = VRDevice::getInstance() != nullptr;
    if (imgui_window_is_visible("VR", "VR controls") != vrModeActive)
      imgui_window_set_visible("VR", "VR controls", vrModeActive);

    ImGui::GetIO().MouseDrawCursor = vrModeActive;
  }

  void renderVrGui(ViewportWindow *vpw)
  {
    // With free camera, we don't want to render GUI in VR
    if (CCameraElem::getCamera() == CCameraElem::FREE_CAMERA)
      return;

    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
  }

  void renderVRView(const VRDevice::FrameData &frame_data, StereoIndex stereo_index, const TMatrix &orig_cam_transform)
  {
    DAEDITOR3.setStereoIndex(stereo_index);

    auto vrDevice = VRDevice::getInstance();

    int width, height;
    vrDevice->getViewResolution(width, height);

    auto &target = frame_data.frameTargets[stereo_index == StereoIndex::Right ? 1 : 0];
    auto &view = frame_data.views[stereo_index == StereoIndex::Right ? 1 : 0];
    auto viewTransform = TMatrix(view.viewTransform);
    auto cameraTransform = TMatrix(view.cameraTransform);
    TMatrix4 projTransform;
    d3d::calcproj(view.projection, projTransform);

    d3d::settm(TM_VIEW, viewTransform);
    d3d::setpersp(view.projection);

    ::grs_cur_view.tm = viewTransform;
    ::grs_cur_view.itm = cameraTransform;
    ::grs_cur_view.pos = cameraTransform.getcol(3);

    beforeRender();
    d3d::set_render_target(vrResources.sceneRt.getTex2D(), 0);
    d3d::set_depth(nullptr, DepthAccess::RW);
    d3d::setview(0, 0, width, height, 0, 1);
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(64, 64, 64, 0), 0, 0);

    deferredRender(viewTransform, projTransform, true);

    if (use_heat_haze)
      heat_haze_glue.render();

    bool use_postfx = (::hdr_render_mode != HDR_MODE_NONE) && postFx && !renderNoPostfx;

    Texture *finalRt = resolvePostProcessing(use_postfx, true);

    IEditorCoreEngine::get()->renderObjects();
    IEditorCoreEngine::get()->renderTransObjects();

    d3d_err(d3d::stretch_rect(finalRt, target.getTex2D(), NULL, NULL));

    if (CCameraElem::getCamera() != CCameraElem::FREE_CAMERA)
    {
      TMatrix rotY;
      rotY.rotyTM(-PI / 2);
      d3d::settm(TM_WORLD, orig_cam_transform * rotY);
      d3d::set_render_target(target.getTex2D(), 0);
      vrgui::render_to_surface(vrResources.imguiTex.getTexId());
      d3d::settm(TM_WORLD, TMatrix::IDENT);
    }
  }

  void renderVr(ViewportWindow *vpw)
  {
    auto vrDevice = VRDevice::getInstance();

    auto dummyCallback = [](VRDevice::Session) { return true; };
    vrDevice->tick(dummyCallback);

    constexpr int view_config_count = 3;
    static VRDevice::FrameData view_configs[view_config_count];
    static int active_view_config = 0;

    active_view_config = (active_view_config + 1) % view_config_count;
    auto &vrViewConfig = view_configs[active_view_config];

    float znear, zfar;
    vpw->getZnearZfar(znear, zfar);
    if (vrDevice->isActive() && vrDevice->prepareFrame(vrViewConfig, znear, zfar))
    {
      TMatrix camTransform;
      vpw->getCameraTransform(camTransform);
      TMatrix4D camTransformD = camTransform;

      auto setUpView = [&camTransformD](VRDevice::FrameData::ViewData &view) {
        view.cameraTransform *= camTransformD;
        view.viewTransform = orthonormalized_inverse(view.cameraTransform);
      };

      for (int viewIx = 0; viewIx < 2; ++viewIx)
        setUpView(vrViewConfig.views[viewIx]);

      setUpView(vrViewConfig.boundingView);

      vrDevice->beginFrame(vrViewConfig);

      for (auto index : {StereoIndex::Left, StereoIndex::Right})
        renderVRView(vrViewConfig, index, camTransform);

      DAEDITOR3.setStereoIndex(StereoIndex::Mono);

      BaseTexture *sa[] = {vrViewConfig.frameTargets[0].getBaseTex(), vrViewConfig.frameTargets[1].getBaseTex(),
        vrViewConfig.frameDepths[0].getBaseTex(), vrViewConfig.frameDepths[1].getBaseTex()};
      d3d::driver_command(Drv3dCommand::PREPARE_TEXTURES_FOR_VR_CONSUMPTION, sa, (void *)4);
    }
  }
};

class GenericDynRenderService : public IDynRenderService
{
public:
  DaEditor3DynamicScene *dynScene;
  Tab<int> rtypeSupp;
  DataBlock dynamicABlk;
  DataBlock deferredBlk;
  Tab<const char *> dbgShowTypeNm;
  Tab<int> dbgShowTypeVal;
  int dbgShowType;
  DynamicRenderOption *dynRendOpt[ROPT_COUNT];
  Tab<const char *> shadowQualityNm;
  Tab<const DataBlock *> shadowQualityProps;

  GenericDynRenderService() : dynScene(NULL)
  {
    dbgShowTypeNm.push_back("- RESULT -");
    dbgShowTypeVal.push_back(-1);
    dbgShowType = 0;

    memset(dynRendOpt, 0, sizeof(dynRendOpt));
    dynRendOpt[ROPT_SHADOWS] = &renderShadow;
    dynRendOpt[ROPT_SHADOWS_VSM] = &renderShadowVsm;
    dynRendOpt[ROPT_SHADOWS_FOM] = &renderShadowFom;
    dynRendOpt[ROPT_WATER_REFL] = &renderWater;
    dynRendOpt[ROPT_ENVI_ORDER] = &renderEnvironmentFirst;
    dynRendOpt[ROPT_SSAO] = &renderSSAO;
    dynRendOpt[ROPT_SSR] = &renderSSR;
    dynRendOpt[ROPT_NO_SUN] = &renderNoSun;
    dynRendOpt[ROPT_NO_AMB] = &renderNoAmb;
    dynRendOpt[ROPT_NO_ENV_REFL] = &renderNoEnvRefl;
    dynRendOpt[ROPT_NO_TRANSP] = &renderNoTransp;
    dynRendOpt[ROPT_NO_POSTFX] = &renderNoPostfx;
  }

  virtual void setup(const char *app_dir, const DataBlock &appblk)
  {
    dynScene = new DaEditor3DynamicScene;

    if (const DataBlock *b = appblk.getBlockByName("dynamicDeferred"))
    {
      deferredBlk = *b;
      vsm_allowed = b->getBool("vsmAllowed", true);
      rtypeSupp.push_back(dynScene->rtype = RTYPE_DYNAMIC_DEFERRED);
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

      dynRendOpt[ROPT_WATER_REFL]->setup(false, false); //== enable when render supports water
      dynRendOpt[ROPT_ENVI_ORDER]->setup(false, false); // we always render envi after opaque
      for (int i = 0; i < ROPT_COUNT; i++)
        if (deferredBlk.getBlockByNameEx("supportsRopt")->getBool(dynRendOpt[i]->name, false))
          dynRendOpt[i]->setup(true, deferredBlk.getBlockByNameEx("defaultRopt")->getBool(dynRendOpt[i]->name, true));

      int nid = deferredBlk.getNameId("shadowPreset");
      for (int i = 0; i < deferredBlk.blockCount(); i++)
        if (deferredBlk.getBlock(i)->getBlockNameId() == nid)
          if (const char *nm = deferredBlk.getBlock(i)->getStr("name", NULL))
          {
            shadowQualityNm.push_back(nm);
            shadowQualityProps.push_back(deferredBlk.getBlock(i));
          }
      if (!shadowQualityNm.size())
      {
        shadowQualityNm.push_back(NULL);
        shadowQualityProps.push_back(&deferredBlk);
      }
      if (DataBlock *b = deferredBlk.getBlockByName("hdr"))
        b->setStr("mode", deferredBlk.getStr("hdrMode", "none"));
      if (strcmp(deferredBlk.getStr("hdrMode", "none"), "none") == 0)
        dynRendOpt[ROPT_NO_POSTFX]->setup(false, true);

      if (const char *pfx = deferredBlk.getStr("postfx", NULL))
        if (!pfx[0] || strcmp(pfx, "demonPostfx") == 0)
          deferredBlk.removeParam("postfx");
    }
    else
    {
      rtypeSupp.push_back(dynScene->rtype = RTYPE_CLASSIC);
      dynRendOpt[ROPT_ENVI_ORDER]->setup(true, true);
      if (appblk.getBool("disablePostfx", false))
      {
        dynRendOpt[ROPT_NO_POSTFX]->setup(false, true);
        disable_postfx = true;
        DAEDITOR3.conNote("postfx disabled");
      }
      else
        dynRendOpt[ROPT_NO_POSTFX]->setup(true, true);
    }

    ec_camera_elem::freeCameraElem.demandInit();
    ec_camera_elem::maxCameraElem.demandInit();
    ec_camera_elem::fpsCameraElem.demandInit();
    ec_camera_elem::tpsCameraElem.demandInit();
    ec_camera_elem::carCameraElem.demandInit();
    ViewportWindow::render_viewport_frame = &render_viewport_frame;
    dynScene->setEnviReqSceneBlk(appblk.getBool("enviReqStaticSceneBlk", false));

    const char *skies_glob_fn =
      appblk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi")->getStr("skiesGlobal", "gameData/environments/global.blk");
    String globalFileName(skies_glob_fn);
    if (!dd_file_exists(globalFileName))
      globalFileName.printf(0, "%s/develop/%s", app_dir, skies_glob_fn);
    if (!dd_file_exists(globalFileName))
      globalFileName.printf(0, "%s/develop/gameBase/%s", app_dir, skies_glob_fn);
    if (!dd_file_exists(globalFileName))
      globalFileName.printf(0, "%s/%s/%s", app_dir,
        appblk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi")->getStr("skiesFilePathPrefix", "."), skies_glob_fn);
    debug("dynRender: globalFileName=<%s> exists=%d", globalFileName, dd_file_exists(globalFileName));
    if (dd_file_exists(globalFileName))
      gameGlobVars.load(globalFileName);
    use_heat_haze = get_managed_texture_id("haze_noise*") != BAD_TEXTUREID;
    DEBUG_DUMP_VAR(use_heat_haze);
  }

  virtual void init()
  {
    ec_init_stat3d();
    if (dynScene->rtype == RTYPE_CLASSIC)
      dynScene->initClassic();
    else if (dynScene->rtype == RTYPE_DYNAMIC_DEFERRED)
      dynScene->initDeferred(deferredBlk, shadowQualityProps[shadowQuality]);

    dagor_select_game_scene(dynScene);
  }
  virtual void term()
  {
    del_it(dynScene);
    deferredBlk.reset();
  }

  virtual void enableRender(bool enable) { render_enabled = enable; }

  virtual void setEnvironmentSnapshot(const char *blk_fn, bool render_cubetex_from_snapshot)
  {
    if (dynScene)
      dynScene->setEnvironmentSnapshot(blk_fn, render_cubetex_from_snapshot);
  }
  virtual bool hasEnvironmentSnapshot() { return dynScene && dynScene->hasEnvironmentSnapshot(); }

  virtual void renderEnviCubeTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd)
  {
    if (dynScene)
      dynScene->renderEnviCubeTexture(tex, cMul, cAdd);
  }
  virtual void renderEnviBkgTexture(BaseTexture *tex, const Color4 &scales)
  {
    if (dynScene)
      dynScene->renderEnviBkgTexture(tex, scales);
  }
  virtual void renderEnviVolTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd, const Color4 &scales, float tz)
  {
    if (dynScene)
      dynScene->renderEnviVolTexture(tex, cMul, cAdd, scales, tz);
  }

  virtual void restartPostfx(const DataBlock &game_params)
  {
    if (dynScene)
      dynScene->restartPostfx(game_params);
  }
  virtual void onLightingSettingsChanged()
  {
    if (dynScene)
      dynScene->reqEnviProbeUpdate = true;
  }

  virtual void afterD3DReset(bool full_reset)
  {
    if (dynScene)
    {
      d3d::GpuAutoLock gpuLock;
      dynScene->afterD3DReset(full_reset);
      dynScene->reInitCsm(shadowQualityProps.size() ? shadowQualityProps[shadowQuality] : NULL);
    }
  }

  virtual void renderViewportFrame(ViewportWindow *vpw)
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
  virtual void renderScreenshot()
  {
    if (dynScene)
      dynScene->renderScreenShot();
  }

  virtual void renderFramesToGetStableAdaptation(float max_da, int max_frames)
  {
    // NOTE: ImGui porting: with the current continuous rendering this seems unnecessary and causes a crash in the Environment settings
    // dialog.
    //     if (renderNoPostfx || !dynScene)
    //       return;
    //
    //     float cur_exposure = getExposure();
    //     while (max_frames)
    //     {
    //       renderViewportFrame(NULL);
    //       float new_exposure = getExposure();
    //       // debug("renderFramesToGetStableAdaptation: %d frames left, a=%.3f da=%.3f max_a=%.3f",
    //       //   max_frames, cur_ob, new_ob-cur_ob, max_da);
    //       if (fabsf(new_exposure - cur_exposure) < max_da)
    //         break;
    //       cur_exposure = new_exposure;
    //       max_frames--;
    //     }
  }

  virtual void enableFrameProfiler(bool enable)
  {
    if (enable)
      da_profiler::add_mode(da_profiler::EVENTS | da_profiler::GPU);
    else
      da_profiler::remove_mode(da_profiler::EVENTS | da_profiler::GPU);
    DAEDITOR3.conNote("Time profiler %s", enable ? "ON" : "OFF");
  }
  virtual void profilerDumpFrame()
  {
    if ((da_profiler::get_active_mode() & da_profiler::EVENTS))
      dump_frame = true;
  }

  virtual dag::ConstSpan<int> getSupportedRenderTypes() const { return rtypeSupp; }
  virtual int getRenderType() const { return dynScene->rtype; }
  virtual bool setRenderType(int t)
  {
    if (find_value_idx(rtypeSupp, t) < 0)
      return false;
    dynScene->setRenderType(t);
    return true;
  }

  virtual dag::ConstSpan<const char *> getDebugShowTypeNames() const { return dbgShowTypeNm; }
  virtual int getDebugShowType() const { return dbgShowType; }
  virtual bool setDebugShowType(int t)
  {
    if (t < 0 || t >= dbgShowTypeNm.size())
      return false;
    dbgShowType = t;
    dynScene->dbgShowType = dbgShowTypeVal[t];
    return true;
  }

  virtual const char *getRenderOptName(int ropt) { return (ropt >= 0 && ropt < ROPT_COUNT) ? dynRendOpt[ropt]->name : NULL; }
  virtual bool getRenderOptSupported(int ropt)
  {
    if (ropt < 0 || ropt >= ROPT_COUNT)
      return false;
    return dynRendOpt[ropt]->allowed;
  }
  virtual void setRenderOptEnabled(int ropt, bool enable)
  {
    if (ropt < 0 || ropt >= ROPT_COUNT)
      return;
    d3d::GpuAutoLock gpuLock;
    *dynRendOpt[ropt] = enable;
    if (*dynRendOpt[ropt] == enable)
      switch (ropt)
      {
        case ROPT_SHADOWS: dynScene->updateCsm(); break;
        case ROPT_SHADOWS_VSM: dynScene->updateVsm(); break;
      }
    if (ropt == ROPT_NO_POSTFX && !enable)
    {
      setExposure(1.0f);
    }
  }
  virtual bool getRenderOptEnabled(int ropt)
  {
    if (ropt < 0 || ropt >= ROPT_COUNT)
      return false;
    return *dynRendOpt[ropt];
  }

  virtual dag::ConstSpan<const char *> getShadowQualityNames() const { return shadowQualityNm; }
  virtual int getShadowQuality() const { return shadowQuality; }
  virtual void setShadowQuality(int q)
  {
    if (q < 0)
      q = 0;
    else if (q >= shadowQualityNm.size())
      q = shadowQualityNm.size() - 1;
    if (shadowQuality == q)
      return;
    shadowQuality = q;

    d3d::GpuAutoLock gpuLock;
    if (dynScene)
      dynScene->reInitCsm(shadowQualityProps.size() ? shadowQualityProps[q] : NULL);
  }

  virtual bool hasExposure() const { return dynScene ? dynScene->hasExposure() : false; }

  virtual void setExposure(float exposure)
  {
    if (dynScene)
      dynScene->setExposure(exposure);
  };

  virtual float getExposure()
  {
    if (dynScene)
      return dynScene->getExposure();
    else
      return 0.0f;
  };

  virtual void setWaterReflectionEnabled(bool enable) { renderWater = enable; }
  virtual bool getWaterReflectionEnabled() { return renderWater; }


  virtual void setRenderEnviFirst(bool first) { renderEnvironmentFirst = first && dynScene->rtype != RTYPE_DYNAMIC_DEFERRED; }
  virtual bool getRenderEnviFirst() { return renderEnvironmentFirst; }

  virtual void getPostFxSettings(DemonPostFxSettings &set)
  {
    if (dynScene)
      dynScene->getPostFxSettings(set);
    else
      memset(&set, 0, sizeof(set));
  }
  virtual void setPostFxSettings(DemonPostFxSettings &set)
  {
    if (dynScene)
      dynScene->setPostFxSettings(set);
  }

  virtual BaseTexture *getRenderBuffer() override { return dynScene ? dynScene->getRenderBuffer() : nullptr; }

  virtual D3DRESID getRenderBufferId() override { return dynScene ? dynScene->getRenderBufferId() : BAD_TEXTUREID; }

  virtual BaseTexture *getDepthBuffer() override { return dynScene ? dynScene->getDepthBuffer() : nullptr; }

  virtual D3DRESID getDepthBufferId() override { return dynScene ? dynScene->getDepthBufferId() : BAD_TEXTUREID; }

  void onTonemapSettingsChanged() { dynScene->onTonemapSettingsChanged(); }

  virtual const ManagedTex &getDownsampledFarDepth() override { return dynScene->getDownsampledFarDepth(); }

  virtual void toggleVrMode() override { dynScene->toggleVrMode(); }

  static void render_viewport_frame(ViewportWindow *vpw);
};

static GenericDynRenderService srv;
void GenericDynRenderService::render_viewport_frame(ViewportWindow *vpw) { srv.renderViewportFrame(vpw); }

void set_tonemap_changed_callback(void (*)());

void *get_generic_dyn_render_service()
{
  set_tonemap_changed_callback([] { srv.onTonemapSettingsChanged(); });
  return &srv;
}

///////////////////////////////////////////////////////////////////////////////
// VR GUI rendering
///////////////////////////////////////////////////////////////////////////////
#ifdef TOOLS_HAVE_VR

extern ISkiesService *av_skies_srv;
extern SimpleString av_skies_preset, av_skies_env, av_skies_wtype;

static void vr_controls_window()
{
  Tab<String> preset(midmem), env(midmem), wtype(midmem);
  av_skies_srv->fillPresets(preset, env, wtype);

  const char *newPreset = nullptr;
  const char *newEnv = nullptr;
  const char *newWtype = nullptr;

  ImGui::Text("Weather presets");
  if (ImGui::BeginListBox("##preset", ImVec2(-FLT_MIN, 0)))
  {
    for (auto &p : preset)
    {
      auto displayName = p;
      auto lastDash = displayName.find('/', nullptr, false);
      if (lastDash)
        displayName.erase(displayName.begin(), lastDash + 1);

      bool selected = strcmp(p.data(), av_skies_preset.c_str()) == 0;
      if (ImGui::Selectable(displayName, &selected))
        newPreset = p;
    }
    ImGui::EndListBox();
  }

  ImGui::Text("Environment types");
  if (ImGui::BeginListBox("##env", ImVec2(-FLT_MIN, 0)))
  {
    for (auto &e : env)
    {
      bool selected = strcmp(e.data(), av_skies_env.c_str()) == 0;
      if (ImGui::Selectable(e, &selected))
        newEnv = e;
    }
    ImGui::EndListBox();
  }

  ImGui::Text("Weather types");
  if (ImGui::BeginListBox("##wtype", ImVec2(-FLT_MIN, 0)))
  {
    for (auto &t : wtype)
    {
      bool selected = strcmp(t.data(), av_skies_wtype.c_str()) == 0;
      if (ImGui::Selectable(t, &selected))
        newWtype = t;
    }
    ImGui::EndListBox();
  }

  ImGui::Checkbox("Enable shadows", &render_vr_shadows);

  if (newPreset || newEnv || newWtype)
  {
    if (newPreset)
      av_skies_preset = newPreset;
    if (newEnv)
      av_skies_env = newEnv;
    if (newWtype)
      av_skies_wtype = newWtype;

    av_skies_srv->setWeather(av_skies_preset, av_skies_env, av_skies_wtype);
  }
}

REGISTER_IMGUI_WINDOW("VR", "VR controls", vr_controls_window);

#endif