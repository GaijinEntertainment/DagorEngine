// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landscape_preview_service.h"

#include <de3_interface.h>
#include <de3_skiesService.h>
#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <EditorCore/ec_interface.h>
#include <render/dag_cur_view.h>

#include <render/deferredRenderer.h>
#include <render/cascadeShadows.h>
#include <render/preIntegratedGF.h>
#include <render/scopeRenderTarget.h>
#include <render/viewVecs.h>
#include <render/ssao.h>
#include <render/downsampleDepth.h>
#include <render/screenSpaceReflections.h>
#include <render/set_reprojection.h>
#include <shaders/dag_postFxRenderer.h>

#include <heightmap/heightmapRenderer.h>
#include <heightmap/lodGrid.h>
#include <heightmap/heightmapCulling.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_sampler.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_info.h>

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_frustum.h>
#include <math/dag_color.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>

#include <debug/dag_debug.h>
#include <memory/dag_mem.h>

#include <EditorCore/ec_wndGlobal.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

#include <generic/dag_carray.h>
#include <util/dag_string.h>

#include <float.h>

namespace
{
constexpr int GBUF_RT = 3;
const unsigned gbufFmts[GBUF_RT] = {
  TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE,
  TEXFMT_A2B10G10R10,
  TEXFMT_R8G8,
};
constexpr unsigned DEPTH_FMT = TEXFMT_DEPTH32;

constexpr float CAMERA_ZNEAR = 0.05f;
constexpr float CAMERA_ZFAR = 80000.0f;
constexpr float CAMERA_FOV_WK = 1.3f;

// The preview backs all of its render targets at this fixed size and reuses them
// for the lifetime of the service. The panel displays a UV-cropped sub-rect of
// the output to match its own aspect. Sizing the backing to the panel would
// re-allocate the DeferredRT depth tex (landscape_preview_intzDepthTex) on every
// drag frame on DX11 (no aliasing support).
constexpr int ALLOC_W = 1920;
constexpr int ALLOC_H = 1080;
} // namespace

class LandscapePreviewServiceImpl final : public IEditorService, public ILandscapePreviewService, public ICascadeShadowsClient
{
public:
  LandscapePreviewServiceImpl() { dirToSun = normalize(Point3(0.5f, 0.8f, 0.3f)); }
  ~LandscapePreviewServiceImpl() override { closeResources(); }

  // ---- IEditorService ----
  const char *getServiceName() const override { return "landscape-preview"; }
  const char *getServiceFriendlyName() const override { return "(srv) Landscape Preview Service"; }
  void setServiceVisible(bool) override {}
  bool getServiceVisible() const override { return false; }
  void actService(float) override {}
  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, ILandscapePreviewService);
    return nullptr;
  }

  // ---- ILandscapePreviewService ----
  bool isAvailable() const override
  {
    // We test lazily on first render. Before that, assume yes.
    return !availabilityChecked || shadersAvailable;
  }

  const char *getUnavailableReason() const override { return unavailableReason.c_str(); }

  void setHeightmapParams(float scale, float min_height, float cell_size) override
  {
    // Keep the last good values when the caller passes sentinels. Graph
    // regeneration can briefly send FLT_MAX (empty heightScale/cellSize
    // strings in the graph json, see main_graph_editor.cpp) and we don't
    // want the stub plane's LOD grid to resize while the graph is
    // preparing a new texture.
    if (scale != FLT_MAX && fabsf(scale) >= 0.01f)
    {
      heightmapScale = scale;
    }
    if (min_height != FLT_MAX)
    {
      heightmapMin = min_height;
    }
    if (cell_size != FLT_MAX)
    {
      heightmapCellSize = max(cell_size, 0.01f);
    }
  }

  void setHeightmapTexture(TEXTUREID tex_id) override { heightmapTexId = tex_id; }

  void setCameraTransform(const TMatrix &view_tm, const Point3 &camera_pos) override
  {
    viewTm = view_tm;
    cameraPos = camera_pos;
  }


  void setSunDirection(const Point3 &dir_to_sun) override { dirToSun = normalize(dir_to_sun); }

  void setRenderParams(float dt) override
  {
    lastDt = dt;
    renderRequested = true;
  }

  void executeRender() override
  {
    if (!renderRequested)
    {
      return;
    }

    renderRequested = false;

    if (!tryInit())
    {
      return;
    }

    renderFrame(lastDt);
  }

  BaseTexture *getOutputTexture() override { return postfxOut.getBaseTex(); }

  IPoint2 getOutputAllocSize() const override { return IPoint2(ALLOC_W, ALLOC_H); }

  // ---- ICascadeShadowsClient ----
  Point4 getCascadeShadowAnchor(int) override { return Point4::xyzV(cameraPos, 0.0f); }

  void renderCascadeShadowDepth(int, const Point2 &) override { drawHeightmapOnly(); }

  void getCascadeShadowSparseUpdateParams(int, const Frustum &, float &out_min_sparse_dist, int &out_min_sparse_frame) override
  {
    out_min_sparse_dist = 0;
    out_min_sparse_frame = 0;
  }

private:
  // ---- Init / resources ----
  bool tryInit()
  {
    if (availabilityChecked)
    {
      return shadersAvailable;
    }

    availabilityChecked = true;

    shadersAvailable = meshRenderer.init("landscape_preview_heightmap", "", "hmap_tess_factor", false /*do_fatal*/, 7);
    if (!shadersAvailable)
    {
      unavailableReason =
        "Landscape preview shader ('landscape_preview_heightmap') is not present in this project's daEditorX tool shader dump. "
        "Add it to the project's shader list (e.g. shadersListCommon.blk or shadersList.blk).";
      DAEDITOR3.conError("LandscapePreviewService: %s", unavailableReason.c_str());
      return false;
    }

    if (!createTargets(ALLOC_W, ALLOC_H))
    {
      shadersAvailable = false;
      unavailableReason = "Failed to allocate render targets for landscape preview.";
      return false;
    }

    cacheShaderVars();
    ensureStubHeightmap();
    global_frame_const_blockid = ShaderGlobal::getBlockId("global_const_block");

    CascadeShadows::Settings csmSettings;
    csmSettings.cascadeWidth = 512;
    // External strategy: we own the depth texture under a service-scoped name so that two
    // CascadeShadows instances (dynRenderSrv + preview) can coexist without colliding on the
    // hardcoded managed name "shadowCascadeDepthTex2D" used by the Internal path.
    csmSettings.resourceAccessStrategy = CascadeShadows::Settings::ResourceAccessStrategy::External;
    const int csmTexW = csmSettings.splitsW * csmSettings.cascadeWidth;
    const int csmTexH = csmSettings.splitsH * csmSettings.cascadeWidth;
    shadowCascadeDepthTex.close();
    shadowCascadeDepthTex = dag::create_tex(nullptr, csmTexW, csmTexH, TEXFMT_DEPTH32 | TEXCF_RTARGET | TEXCF_TC_COMPATIBLE, 1,
      "landscape_preview_shadow_cascade_depth_tex");
    if (!shadowCascadeDepthTex)
    {
      shadersAvailable = false;
      unavailableReason = "Failed to allocate CSM depth texture for landscape preview.";
      return false;
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Compare;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      shadowCascadeDepthSampler = d3d::request_sampler(smpInfo);
      ShaderGlobal::set_sampler(get_shader_variable_id("shadow_cascade_depth_tex_samplerstate", true), shadowCascadeDepthSampler);
    }
    csm = CascadeShadows::make(this, csmSettings, String("landscape_preview"));

    // Reuse the host's DaSkies via ISkiesService. We do NOT create our own
    // DaSkies (it registers process-global Sbuffers like atmosphere_cb).
    // Instead, ISkiesService::renderSky() reads the CURRENT d3d view/proj
    // matrices and ::grs_cur_view, so if we set those to our preview camera
    // and bind our RT before calling it, the host's DaSkies renders sky
    // into our render target at our camera position.
    skiesService = EDITORCORE->queryEditorInterface<ISkiesService>();
    lightService = EDITORCORE->queryEditorInterface<ISceneLightService>();

    // Post-fx
    combineShadowsRenderer.init("combine_shadows", true);
    postfx.init("postfx", true);

    preIntegratedGF = render_preintegrated_fresnel_GGX("preIntegratedGF_lp", PREINTEGRATE_SPECULAR_DIFFUSE_QUALITY_MAX);

    // SSAO, SSR, and the depth pyramid are allocated by createTargets(). We
    // own private instances (unique SSAO tag; our own SSR instance) so the
    // host dynRenderSrv's resources are untouched; shader globals we overwrite
    // during renderFrame are saved and restored around our pass.

    return true;
  }

  // Called exactly once from tryInit() at the fixed alloc size. The preview
  // backs everything at a single size for its lifetime; the panel crops a UV
  // sub-rect for display.
  bool createTargets(int w, int h)
  {
    if (!target)
    {
      target = eastl::make_unique<DeferredRenderTarget>("deferred_shadow_to_buffer", "landscape_preview", w, h,
        DeferredRT::StereoMode::MonoOrMultipass, 0, GBUF_RT, gbufFmts, DEPTH_FMT);
    }

    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
    {
      rtFmt = TEXFMT_A16B16G16R16F;
    }

    releaseShaderVarBindings();

    frame.close();
    frame = dag::create_tex(nullptr, w, h, rtFmt | TEXCF_RTARGET, 1, "landscape_preview_frame");
    if (!frame)
    {
      return false;
    }

    postfxOut.close();
    postfxOut = dag::create_tex(nullptr, w, h, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "landscape_preview_postfx_out");
    if (!postfxOut)
    {
      return false;
    }

    combinedShadows.close();
    combinedShadows = dag::create_tex(nullptr, w, h, TEXFMT_R8 | TEXCF_RTARGET, 1, "landscape_preview_combined_shadows");
    if (!combinedShadows)
    {
      return false;
    }

    // Depth pyramid at half resolution. downsample_depth module is init'd globally
    // by daEditorX's dynRenderSrv (dynRender.cpp) at startup; we only own the
    // output textures here. Unique names avoid colliding with dynRender's
    // "downsampled_far_depth_tex".
    ssr.reset();
    ssao.reset();
    for (int i = 0; i < farDownsampledDepth.size(); ++i)
    {
      farDownsampledDepth[i].close();
    }
    const int halfW = max(w / 2, 1);
    const int halfH = max(h / 2, 1);
    int numMips = max(min(get_log2w(halfW), get_log2w(halfH)) - 1, 4);
    for (int i = 0; i < farDownsampledDepth.size(); ++i)
    {
      String name(64, "landscape_preview_far_downsampled_depth%d", i);
      farDownsampledDepth[i] = dag::create_tex(nullptr, halfW, halfH, TEXFMT_R32F | TEXCF_RTARGET, numMips, name.str());
    }
    currentDownsampledDepth = 0;

    ssao = eastl::make_unique<SSAORenderer>(halfW, halfH, 1, SSAO_ALLOW_IMPLICIT_FLAGS_FROM_SHADER_ASSUMES, true, "landscape_preview");

    // SSR consumes the depth pyramid and G-buffer normals/roughness; writes ssr_target
    // shader global internally. Own textures keyed at half-res; the "_landscape_preview"
    // tag keeps the managed texture name distinct from the host dynRenderSrv's SSR.
    ssr =
      eastl::make_unique<ScreenSpaceReflections>(halfW, halfH, 1, 0, SSRQuality::Low, SSRFlag::CreateTextures, "_landscape_preview");

    int renderingResVarId = get_shader_variable_id("rendering_res", true);
    if (renderingResVarId >= 0)
    {
      ShaderGlobal::set_float4(renderingResVarId, w, h, 1.0f / w, 1.0f / h);
    }

    return true;
  }

  void releaseShaderVarBindings()
  {
    if (combined_shadowsVarId >= 0)
    {
      ShaderGlobal::set_texture(combined_shadowsVarId, BAD_TEXTUREID);
    }

    static int frame_texVarId = get_shader_variable_id("frame_tex", true);

    if (frame_texVarId >= 0)
    {
      ShaderGlobal::set_texture(frame_texVarId, BAD_TEXTUREID);
    }
    if (ssao_texVarId >= 0)
    {
      ShaderGlobal::set_texture(ssao_texVarId, BAD_TEXTUREID);
    }
    if (ssr_targetVarId >= 0)
    {
      ShaderGlobal::set_texture(ssr_targetVarId, BAD_TEXTUREID);
    }
    if (shadow_cascade_depth_texVarId >= 0)
    {
      ShaderGlobal::set_texture(shadow_cascade_depth_texVarId, BAD_TEXTUREID);
    }
    if (downsampled_far_depth_texVarId >= 0)
    {
      ShaderGlobal::set_texture(downsampled_far_depth_texVarId, BAD_TEXTUREID);
    }
    if (prev_downsampled_far_depth_texVarId >= 0)
    {
      ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, BAD_TEXTUREID);
    }
    // Drop our stub heightmap binding so stubHeightmapTex.close() releases cleanly.
    if (tex_hmap_lowVarId >= 0)
    {
      ShaderGlobal::set_texture(tex_hmap_lowVarId, BAD_TEXTUREID);
    }
    if (tex_hmap_highVarId >= 0)
    {
      ShaderGlobal::set_texture(tex_hmap_highVarId, BAD_TEXTUREID);
    }
  }

  void ensureStubHeightmap()
  {
    if (stubHeightmapTex)
    {
      return;
    }
    stubHeightmapTex = dag::create_tex(nullptr, 1, 1, TEXFMT_L16 | TEXCF_CLEAR_ON_CREATE, 1, "landscape_preview_stub_hmap");
  }

  void cacheShaderVars()
  {
    world_view_posVarId = get_shader_variable_id("world_view_pos", true);
    zn_zfarVarId = get_shader_variable_id("zn_zfar", true);
    heightmap_scaleVarId = get_shader_variable_id("heightmap_scale", true);
    world_to_hmap_lowVarId = get_shader_variable_id("world_to_hmap_low", true);
    tex_hmap_inv_sizesVarId = get_shader_variable_id("tex_hmap_inv_sizes", true);
    // Engine shader (heightmap_common.dshl) texture bindings.
    tex_hmap_lowVarId = get_shader_variable_id("tex_hmap_low", true);
    tex_hmap_highVarId = get_shader_variable_id("tex_hmap_high", true);
    world_to_hmap_highVarId = get_shader_variable_id("world_to_hmap_high", true);
    heightmap_regionVarId = get_shader_variable_id("heightmap_region", true);
    from_sun_directionVarId = get_shader_variable_id("from_sun_direction", true);
    sun_light_colorVarId = get_shader_variable_id("sun_light_color", true);
    amb_light_colorVarId = get_shader_variable_id("amb_light_color", true);
    combined_shadowsVarId = get_shader_variable_id("combined_shadows", true);
    downsampled_far_depth_texVarId = get_shader_variable_id("downsampled_far_depth_tex", true);
    prev_downsampled_far_depth_texVarId = get_shader_variable_id("prev_downsampled_far_depth_tex", true);
    ssao_texVarId = get_shader_variable_id("ssao_tex", true);
    ssr_targetVarId = get_shader_variable_id("ssr_target", true);
    shadow_cascade_depth_texVarId = get_shader_variable_id("shadow_cascade_depth_tex", true);
  }

  void closeResources()
  {
    if (csm)
    {
      delete csm;
      csm = nullptr;
    }
    meshRenderer.close();

    releaseShaderVarBindings();

    // Tear down in reverse-dependency order: SSR/SSAO first (use depth),
    // then far depth textures, then other RTs, then DeferredRT last.
    ssr.reset();
    ssao.reset();
    for (int i = 0; i < farDownsampledDepth.size(); ++i)
    {
      farDownsampledDepth[i].close();
    }

    preIntegratedGF.close();
    combinedShadows.close();
    frame.close();
    postfxOut.close();
    stubHeightmapTex.close();
    shadowCascadeDepthTex.close();
    target.reset();
  }

  // ---- Render ----
  void setGlobalLightShaderVars()
  {
    Point3 toSun = dirToSun;
    if (lightService)
    {
      Point3 sun1Dir, sun2Dir;
      Color3 sun1Col, sun2Col, skyCol;
      lightService->getDirectLight(sun1Dir, sun1Col, sun2Dir, sun2Col, skyCol, true);
      if (lengthSq(sun1Dir) > 0.0f)
      {
        toSun = normalize(sun1Dir);
      }
    }

    if (from_sun_directionVarId >= 0)
    {
      ShaderGlobal::set_float4(from_sun_directionVarId, -toSun.x, -toSun.y, -toSun.z, 0.0f);
    }
    if (sun_light_colorVarId >= 0)
    {
      ShaderGlobal::set_float4(sun_light_colorVarId, 1.0f, 0.95f, 0.85f, 0.0f);
    }
    if (amb_light_colorVarId >= 0)
    {
      ShaderGlobal::set_float4(amb_light_colorVarId, 0.25f, 0.28f, 0.33f, 0.0f);
    }
    if (world_view_posVarId >= 0)
    {
      ShaderGlobal::set_float4(world_view_posVarId, cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
    }
    if (zn_zfarVarId >= 0)
    {
      ShaderGlobal::set_float4(zn_zfarVarId, CAMERA_ZNEAR, CAMERA_ZFAR, 0, 0);
    }

    dirToSun = toSun;
  }

  // Sets shader globals for the engine heightmap shader (heightmap_common.dshl).
  void drawHeightmapOnly()
  {
    if (!meshRenderer.isInited())
    {
      return;
    }

    // When the graph has no heightmap output, fall back to a 1x1 zero-filled
    // stub so the VS samples a defined value (0) and renders a flat plane at
    // heightmapMin -- matching the old standalone graphEditor. Without this,
    // tex_hmap_low keeps whatever another daEditorX plugin left in the shader
    // global and the preview paints garbage.
    const TEXTUREID boundHmapId = (heightmapTexId != BAD_TEXTUREID) ? heightmapTexId : stubHeightmapTex.getTexId();
    if (tex_hmap_lowVarId >= 0)
    {
      ShaderGlobal::set_texture(tex_hmap_lowVarId, boundHmapId);
    }
    if (tex_hmap_highVarId >= 0)
    {
      ShaderGlobal::set_texture(tex_hmap_highVarId, boundHmapId);
    }

    static int tex_hmap_low_samplerstateVarId = get_shader_variable_id("tex_hmap_low_samplerstate", true);
    if (tex_hmap_low_samplerstateVarId >= 0)
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      smpInfo.address_mode_u = d3d::AddressMode::Clamp;
      smpInfo.address_mode_v = d3d::AddressMode::Clamp;
      smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(tex_hmap_low_samplerstateVarId, d3d::request_sampler(smpInfo));
    }

    // Disable the high-res path and reset heightmap_region.
    if (world_to_hmap_highVarId >= 0)
    {
      ShaderGlobal::set_float4(world_to_hmap_highVarId, 10e+3f, 10e+3f, 10e+10f, 10e+10f);
    }
    if (heightmap_regionVarId >= 0)
    {
      ShaderGlobal::set_float4(heightmap_regionVarId, -65536.0f, -65536.0f, 65536.0f, 65536.0f);
    }

    // The graph's heightmap texture stores normalized height values; the shader
    // decodes them as decode_height(a) = a*heightmap_scale.x + heightmap_scale.y
    // so world_height = texel * heightmapScale + heightmapMin. Matches
    // prog/tools/graphEditor/app.cpp:1238.
    if (heightmap_scaleVarId >= 0)
    {
      ShaderGlobal::set_float4(heightmap_scaleVarId, heightmapScale, heightmapMin, 0.0f, 0.0f);
    }

    Color4 invSizes(1.0f / 2048.0f, 1.0f / 2048.0f, 1.0f / 2048.0f, 1.0f / 2048.0f);
    if (heightmapTexId != BAD_TEXTUREID)
    {
      if (BaseTexture *tex = acquire_managed_tex(heightmapTexId))
      {
        TextureInfo ti;
        tex->getinfo(ti);
        if (ti.w > 0 && ti.h > 0)
        {
          invSizes = Color4(1.0f / ti.w, 1.0f / ti.h, 1.0f / ti.w, 1.0f / ti.h);
        }
        release_managed_tex(heightmapTexId);
      }
    }

    if (tex_hmap_inv_sizesVarId >= 0)
    {
      ShaderGlobal::set_float4(tex_hmap_inv_sizesVarId, invSizes);
    }

    // Negative Y matches graphEditor's drawHeightmap convention (the texture's
    // V axis is flipped relative to world Z). The ported shader is written against
    // that convention.
    if (world_to_hmap_lowVarId >= 0)
    {
      ShaderGlobal::set_float4(world_to_hmap_lowVarId, invSizes.r / heightmapCellSize, -invSizes.g / heightmapCellSize, 0.5f, 0.5f);
    }

    mat44f globtm;
    d3d::getglobtm(globtm);
    Frustum frustum(globtm);

    const int lodCount = 5;
    const int lodRad = 2;
    const int lastLodRad = 8;

    LodGrid lodGrid;
    lodGrid.init(lodCount, lodRad, 0, lastLodRad);

    LodGridCullData cullData;
    const float scaledCell = heightmapCellSize;
    const float minHt = heightmapMin;
    const float maxHt = heightmapMin + heightmapScale;
    float lod0Size = 0.0f;
    const float originX = floorf(cameraPos.x + 0.5f);
    const float originZ = floorf(cameraPos.z + 0.5f);
    cull_lod_grid(lodGrid, lodGrid.lodsCount, originX, originZ, scaledCell, scaledCell, -1, -1, minHt, maxHt, &frustum, nullptr,
      cullData, nullptr, lod0Size, meshRenderer.get_hmap_tess_factorVarId(), meshRenderer.getDim(), true);

    if (!cullData.getCount())
    {
      return;
    }

    meshRenderer.render(lodGrid, cullData);
  }

  void combineShadowsPass()
  {
    SCOPE_RENDER_TARGET_NAME(_combinedShadows);
    d3d::set_render_target({}, DepthAccess::RW, {{combinedShadows.getBaseTex(), 0, 0}});
    // Clear to white = "no shadow" everywhere. The daNetGame combine_shadows
    // shader does full-resolution contact-shadow raycasting against
    // depth_gbuf_read + viewProjectionMatrixNoOfs; running it from our
    // floating preview panel produced per-pixel noise that persisted even
    // with all reprojection globals set to our view (see
    // combine_shadows_common.dshl:395-428, contactShadows.hlsl). Sky
    // soft-shadowing and CSM dot(N,L) still happen inside the resolve via
    // from_sun_direction and the G-buffer normals; losing the separate
    // CSM cascade sampling here trades preview shadow quality for clean
    // image. Revisit if the preview ever needs real hard shadows.
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 0, 0);
    if (combined_shadowsVarId >= 0)
    {
      ShaderGlobal::set_texture(combined_shadowsVarId, combinedShadows.getTexId());
    }
  }

  void renderFrame(float /*dt*/)
  {
    // Save the host's reprojection globals (globtm_no_ofs_psf_*, view_vec*,
    // prev_world_view_pos, etc.) for the duration of our render. They are
    // sampled by combine_shadows' contact-shadow raycast and by SSR's
    // temporal reprojection -- using the host's values at our camera would
    // produce per-pixel noise.
    ScopeReprojection scopeReproj;

    // Save previous state
    Driver3dRenderTarget prevRt;
    d3d::get_render_target(prevRt);
    int prevVX = 0, prevVY = 0, prevVW = 0, prevVH = 0;
    float prevVZN = 0, prevVZF = 1;
    d3d::getview(prevVX, prevVY, prevVW, prevVH, prevVZN, prevVZF);
    TMatrix prevViewTm;
    TMatrix4 prevProjTm;
    d3d::gettm(TM_VIEW, prevViewTm);
    d3d::gettm(TM_PROJ, &prevProjTm);
    Driver3dPerspective prevPersp;
    const bool hadPrevPersp = d3d::getpersp(prevPersp);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    // Set our view/proj. Camera aspect matches the backing alloc size; the
    // panel crops a centered UV sub-rect of the result to fit its own aspect.
    const float aspect = (float)ALLOC_W / (float)ALLOC_H;
    const float wk = CAMERA_FOV_WK;
    const float hk = wk * aspect;
    Driver3dPerspective persp(wk, hk, CAMERA_ZNEAR, CAMERA_ZFAR, 0, 0);
    d3d::setpersp(persp);
    d3d::settm(TM_VIEW, viewTm);
    d3d::setview(0, 0, ALLOC_W, ALLOC_H, 0, 1);

    setGlobalLightShaderVars();

    // G-buffer
    target->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0.0f, 0);
    drawHeightmapOnly();

    TMatrix viewNow;
    d3d::gettm(TM_VIEW, viewNow);
    TMatrix4 projTmNow;
    d3d::gettm(TM_PROJ, &projTmNow);

    // Publish our view's reprojection globals: globtm_no_ofs_psf_*, projtm_psf_*,
    // view_vec*, prev_view_vec*, prev_world_view_pos, prev_zn_zfar. These are
    // required by combine_shadows' contact-shadow raycast (viewProjectionMatrixNoOfs)
    // and by any temporal reprojection path. Also updates our prev* state for
    // the next frame.
    set_reprojection(viewNow, projTmNow, prevWorldPos, prevGlobTm, prevViewVecLT, prevViewVecRT, prevViewVecLB, prevViewVecRB,
      nullptr);

    // Save host bindings for globals we are about to overwrite, so the host's
    // dynRenderSrv viewport rendering on the next frame sees its own textures.
    const TEXTUREID prevSsaoTex = ssao_texVarId >= 0 ? ShaderGlobal::get_tex_fast(ssao_texVarId) : BAD_TEXTUREID;
    const TEXTUREID prevSsrTarget = ssr_targetVarId >= 0 ? ShaderGlobal::get_tex_fast(ssr_targetVarId) : BAD_TEXTUREID;
    const TEXTUREID prevDsFarDepthTex =
      downsampled_far_depth_texVarId >= 0 ? ShaderGlobal::get_tex_fast(downsampled_far_depth_texVarId) : BAD_TEXTUREID;
    const TEXTUREID prevDsPrevFarDepthTex =
      prev_downsampled_far_depth_texVarId >= 0 ? ShaderGlobal::get_tex_fast(prev_downsampled_far_depth_texVarId) : BAD_TEXTUREID;

    // CSM: renders cascades by calling back into renderCascadeShadowDepth(), which draws the heightmap.
    if (csm)
    {
      const CascadeShadows::ModeSettings mode;
      csm->prepareShadowCascades(mode, dirToSun, viewNow, cameraPos, projTmNow, Frustum(TMatrix4(viewNow) * projTmNow),
        Point2(CAMERA_ZNEAR, CAMERA_ZFAR), CAMERA_ZNEAR);
      // External strategy: CSM drives the render pass but we supply the depth target and
      // publish shadow_cascade_depth_tex ourselves (the Internal path at cascadeShadows.cpp
      // :503-506 does the barrier + setVar, External skips it).
      BaseTexture *extDepth = shadowCascadeDepthTex.getBaseTex();
      CascadeShadows *csmLocal = csm;
      csm->renderShadowsCascadesCb(
        [csmLocal, extDepth](int numToRender, bool clearPerView) {
          for (int i = 0; i < numToRender; ++i)
          {
            csmLocal->renderShadowCascadeDepth(i, clearPerView, extDepth);
          }
        },
        extDepth, false);
      d3d::resource_barrier({extDepth, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (shadow_cascade_depth_texVarId >= 0)
      {
        ShaderGlobal::set_texture(shadow_cascade_depth_texVarId, shadowCascadeDepthTex.getTexId());
      }
      csm->setCascadesToShader();
    }

    // G-buffer textures (normals, depth, material) as shader inputs. Must be
    // bound BEFORE SSAO/SSR so those passes read our G-buffer normals, not
    // stale bindings from the host's dynRenderSrv.
    target->setVar();

    // View vectors for depth -> world-position reconstruction. Used by SSAO,
    // SSR, combine_shadows and the resolve.
    ::set_viewvecs_to_shader(viewNow, projTmNow);

    // Depth pyramid: downsample the G-buffer depth. Required by combine_shadows'
    // contact-shadow raycast (which samples downsampled_far_depth_tex) AND by
    // SSAO. Ping-pong the two buffers so reprojection has the previous frame.
    currentDownsampledDepth ^= 1;
    BaseTexture *farDepth = farDownsampledDepth[currentDownsampledDepth].getTex2D();
    if (farDepth && target->getDepth())
    {
      downsample_depth::downsample(target->getDepth(), target->getWidth(), target->getHeight(), farDepth, nullptr, nullptr);
      if (downsampled_far_depth_texVarId >= 0)
      {
        ShaderGlobal::set_texture(downsampled_far_depth_texVarId, farDownsampledDepth[currentDownsampledDepth].getTexId());
      }
      if (prev_downsampled_far_depth_texVarId >= 0)
      {
        ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, farDownsampledDepth[1 - currentDownsampledDepth].getTexId());
      }
    }

    // SSAO: consumes farDepth + G-buffer normals, writes ssao_tex.
    if (ssao && farDepth)
    {
      ssao->render(viewNow, projTmNow, farDepth);
      if (ssao_texVarId >= 0)
      {
        ShaderGlobal::set_texture(ssao_texVarId, ssao->getSSAOTexId());
      }
    }
    else if (ssao_texVarId >= 0)
    {
      ShaderGlobal::set_texture(ssao_texVarId, BAD_TEXTUREID);
    }

    // SSR: consumes depth pyramid + G-buffer. Binds ssr_target internally.
    if (ssr)
    {
      ssr->render(viewNow, projTmNow);
    }
    else if (ssr_targetVarId >= 0)
    {
      ShaderGlobal::set_texture(ssr_targetVarId, BAD_TEXTUREID);
    }

    combineShadowsPass();

    // Bind const block and depth texture before resolve
    if (global_frame_const_blockid >= 0)
    {
      ShaderGlobal::setBlock(global_frame_const_blockid, ShaderGlobal::LAYER_GLOBAL_CONST);
    }

    static int intz_depth_texVarId = get_shader_variable_id("intz_depth_tex", true);
    if (intz_depth_texVarId >= 0)
    {
      ShaderGlobal::set_texture(intz_depth_texVarId, target->getDepthId());
    }

    // Resolve deferred -> frame
    target->resolve(frame.getBaseTex(), viewNow, projTmNow);

    // Sky: render the host's sky into our frame RT using the host's DaSkies
    // via ISkiesService. renderSky() reads d3d view/proj and ::grs_cur_view
    // from the current state, so we set those to our preview camera first.
    // NOTE: we do NOT call beforeRenderSky() here -- that would re-prepare
    // DaSkies' toroidal atmosphere/cloud cache for our camera and stomp on
    // the host main-viewport state (visible as "stair-step" cloud seams).
    // Sky atmosphere is prepared once per frame by the host's environment
    // plugin; we just render it from our view.
    if (skiesService)
    {
      d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::SampledRO, {{frame.getBaseTex(), 0, 0}});

      DagorCurView savedView = ::grs_cur_view;
      TMatrix itm = orthonormalized_inverse(viewTm);
      ::grs_cur_view.tm = viewTm;
      ::grs_cur_view.itm = itm;
      ::grs_cur_view.pos = cameraPos;

      skiesService->renderSky();

      ::grs_cur_view = savedView;
      d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::RW, {{frame.getBaseTex(), 0, 0}});
    }

    // Postfx: tonemap frame -> postfxOut
    {
      SCOPE_RENDER_TARGET_NAME(_postfx);
      d3d::set_render_target({}, DepthAccess::RW, {{postfxOut.getBaseTex(), 0, 0}});
      static int frame_texVarId = get_shader_variable_id("frame_tex", true);
      if (frame_texVarId >= 0)
      {
        ShaderGlobal::set_texture(frame_texVarId, frame.getTexId());
      }
      if (postfx.getMat())
      {
        postfx.render();
      }
      else
      {
        d3d::stretch_rect(frame.getBaseTex(), postfxOut.getBaseTex());
      }
    }

    // Restore host bindings for shader globals we overwrote. The host's
    // dynRenderSrv expects its SSAO/SSR/depth-pyramid textures to remain bound
    // across frames (see services/dynRenderSrv/dynRender.cpp).
    if (ssao_texVarId >= 0)
    {
      ShaderGlobal::set_texture(ssao_texVarId, prevSsaoTex);
    }
    if (ssr_targetVarId >= 0)
    {
      ShaderGlobal::set_texture(ssr_targetVarId, prevSsrTarget);
    }
    if (downsampled_far_depth_texVarId >= 0)
    {
      ShaderGlobal::set_texture(downsampled_far_depth_texVarId, prevDsFarDepthTex);
    }
    if (prev_downsampled_far_depth_texVarId >= 0)
    {
      ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, prevDsPrevFarDepthTex);
    }

    // Restore previous state
    d3d::set_render_target(prevRt);
    d3d::setview(prevVX, prevVY, prevVW, prevVH, prevVZN, prevVZF);
    d3d::settm(TM_VIEW, prevViewTm);
    if (hadPrevPersp)
    {
      d3d::setpersp(prevPersp);
    }
    else
    {
      d3d::settm(TM_PROJ, &prevProjTm);
    }
  }

public:
  BaseTexture *getFinalTexture() { return postfxOut.getBaseTex(); }

private:
  // Resources
  ISkiesService *skiesService = nullptr;
  ISceneLightService *lightService = nullptr;
  eastl::unique_ptr<DeferredRenderTarget> target;
  UniqueTex frame;
  UniqueTex postfxOut;
  UniqueTex combinedShadows;
  UniqueTex stubHeightmapTex;
  UniqueTexWithShaderVar preIntegratedGF;
  PostFxRenderer combineShadowsRenderer;
  PostFxRenderer postfx;
  HeightmapRenderer meshRenderer;
  CascadeShadows *csm = nullptr;
  // CSM runs with ResourceAccessStrategy::External so the preview service owns the depth
  // texture under a service-scoped name. The dynRenderSrv CSM uses Internal and hardcodes
  // "shadowCascadeDepthTex2D"; letting both instances use Internal collides in the managed
  // resource table (refCount>1) and asserts on teardown in UniqueRes::release.
  UniqueTex shadowCascadeDepthTex;
  d3d::SamplerHandle shadowCascadeDepthSampler = d3d::INVALID_SAMPLER_HANDLE;
  eastl::unique_ptr<SSAORenderer> ssao;
  eastl::unique_ptr<ScreenSpaceReflections> ssr;
  carray<UniqueTex, 2> farDownsampledDepth;
  int currentDownsampledDepth = 0;

  // State
  TMatrix viewTm = TMatrix::IDENT;
  Point3 cameraPos = Point3(0.0f, 200.0f, -400.0f);
  Point3 dirToSun;
  float heightmapScale = 1.0f;
  float heightmapMin = 0.0f;
  float heightmapCellSize = 1.0f;
  TEXTUREID heightmapTexId = BAD_TEXTUREID;
  bool availabilityChecked = false;
  bool shadersAvailable = false;
  eastl::string unavailableReason;

  // Previous-frame reprojection state (fed to set_reprojection each frame).
  DPoint3 prevWorldPos = DPoint3(0, 0, 0);
  TMatrix4 prevGlobTm = TMatrix4::IDENT;
  Point4 prevViewVecLT = Point4(0, 0, 0, 0);
  Point4 prevViewVecRT = Point4(0, 0, 0, 0);
  Point4 prevViewVecLB = Point4(0, 0, 0, 0);
  Point4 prevViewVecRB = Point4(0, 0, 0, 0);

  // Deferred render request (set in renderPreview/updateImgui, executed in renderService)
  bool renderRequested = false;
  float lastDt = 0.0f;

  // Cached shader var ids
  int world_view_posVarId = -1;
  int zn_zfarVarId = -1;
  int heightmap_scaleVarId = -1;
  int world_to_hmap_lowVarId = -1;
  int tex_hmap_inv_sizesVarId = -1;
  int tex_hmap_lowVarId = -1;
  int tex_hmap_highVarId = -1;
  int world_to_hmap_highVarId = -1;
  int heightmap_regionVarId = -1;
  int from_sun_directionVarId = -1;
  int sun_light_colorVarId = -1;
  int amb_light_colorVarId = -1;
  int combined_shadowsVarId = -1;
  int downsampled_far_depth_texVarId = -1;
  int prev_downsampled_far_depth_texVarId = -1;
  int ssao_texVarId = -1;
  int ssr_targetVarId = -1;
  int shadow_cascade_depth_texVarId = -1;
  int global_frame_const_blockid = -1;
};

void init_landscape_preview_service() { IDaEditor3Engine::get().registerService(new (inimem) LandscapePreviewServiceImpl); }
