// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_tex3d.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <util/dag_stlqsort.h>
#include <util/dag_convar.h>
#include <render/primitiveObjects.h>
#include <math/dag_viewMatrix.h>
#include <debug/dag_debug3d.h>
#include <render/lights/clusteredLights.h>
#include <render/lights/clusteredLightsGrid.h>
#include <render/lights/shadowSystem.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_overrideStates.h>
#include <render/lights/dstReadbackLights.h>
#include <math/dag_hlsl_floatx.h>
#include <render/lights/renderLights.hlsli>
#include <render/depthUtil.h>
#include <3d/dag_lockSbuffer.h>
#include <EASTL/numeric_limits.h>
#include <ioSys/dag_dataBlock.h>
#include <render/lights/lightsEncoding.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_log.h>

static const uint32_t MAX_SHADOWS_QUALITY = 8u;

const float ClusteredLights::MARK_SMALL_LIGHT_AS_FAR_LIMIT = 0.03;

static int omni_lightsVarId = -1;
static int spot_lightsVarId = -1;
static int common_lights_shadowsVarId = -1;

static int depthSliceScaleVarId = -1, depthSliceBiasVarId = -1;
static int shadowAtlasTexelVarId = -1;
static int shadowZBiasVarId = -1, shadowSlopeZBiasVarId = -1;

static int spot_lights_flagsVarId = -1, omni_lights_flagsVarId = -1;
static int spot_lights_ssss_shadow_descVarId = -1;

#define GLOBAL_VARS_OPT_LIST        \
  VAR(oof_clear_grid_size)          \
  VAR(out_of_frustum_grid_cull_box) \
  VAR(out_of_frustum_omni_box)      \
  VAR(out_of_frustum_spot_box)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPT_LIST
#undef VAR

void ClusteredLights::initClustered(int initial_light_density)
{
  lightsGrid = ClusteredLightsGrid(clusters, &lightsResMgr, initial_light_density);
  lightsGrid.init(dynLightsFeatures);
  dynLightsFeatures &=
    ~DynLightsFeatures::GpuClusteredLightsGrid | (lightsGrid.getRuntimeFeatures() & DynLightsFeatures::GpuClusteredLightsGrid);

  depthSliceScaleVarId = get_shader_variable_id("depthSliceScale");
  depthSliceBiasVarId = get_shader_variable_id("depthSliceBias");
  shadowAtlasTexelVarId = get_shader_variable_id("shadowAtlasTexel");
  shadowZBiasVarId = get_shader_variable_id("shadowZBias");
  shadowSlopeZBiasVarId = get_shader_variable_id("shadowSlopeZBias");
  spot_lights_flagsVarId = get_shader_variable_id("spot_lights_flags", true);
  omni_lights_flagsVarId = get_shader_variable_id("omni_lights_flags", true);
}

ClusteredLights::ClusteredLights(const char *name_suffix) :
  lightsResMgr(name_suffix),
  lightsRenderer(&lightsResMgr),
  dynamicLightShadows(&lightsResMgr, &omniLights, &spotLights),
  lightsPartition(omniLights, spotLights, &lightsResMgr),
  lightsCounter(&omniLights, &spotLights)
{
  spotOOFBox[0] = omniOOFBox[0] = Point4(0, 0, 0, 0);
  spotOOFBox[1] = omniOOFBox[1] = Point4(OOF_GRID_W * 2, OOF_GRID_VERT * 2, OOF_GRID_W * 2, 0);
  if (VariableMap::isVariablePresent(get_shader_variable_id("oof_lights_full_grid", true)))
  {
#define CS(a) a.reset(new_compute_shader(#a))
    CS(cull_out_of_frustum_lights_cs);
    CS(clear_out_of_frustum_grid_cs);
#undef CS
  }
}

ClusteredLights::~ClusteredLights() { close(); }

bool ClusteredLights::hasDeferredOmniLights() const { return getVisibleFarOmniCount() > 0; };
bool ClusteredLights::hasDeferredSpotLights() const { return getVisibleFarSpotsCount() > 0; };
bool ClusteredLights::hasClusteredOmniLights() const { return getVisibleClusteredOmniCount() > 0; };
bool ClusteredLights::hasClusteredSpotLights() const { return getVisibleClusteredSpotsCount() > 0; };

bool ClusteredLights::hasDeferredLights() const { return hasDeferredOmniLights() || hasDeferredSpotLights(); };
bool ClusteredLights::hasClusteredLights() const { return hasClusteredOmniLights() || hasClusteredSpotLights(); }

int ClusteredLights::getVisibleFarSpotsCount() const { return lightsCounter.getFarLightsCount(LightType::Spot); }
int ClusteredLights::getVisibleFarOmniCount() const { return lightsCounter.getFarLightsCount(LightType::Omni); }
int ClusteredLights::getVisibleClusteredSpotsCount() const { return lightsCounter.getClusteredLightsCount(LightType::Spot); };
int ClusteredLights::getVisibleClusteredOmniCount() const { return lightsCounter.getClusteredLightsCount(LightType::Omni); }
int ClusteredLights::getVisibleSpotsCount() const { return getVisibleClusteredSpotsCount() + getVisibleFarSpotsCount(); }
int ClusteredLights::getVisibleOmniCount() const { return getVisibleClusteredOmniCount() + getVisibleFarOmniCount(); }

DynLightsOptimizationMode ClusteredLights::getLightsCountInterval() const { return lightsCounter.getLightsCountInterval(); }

void ClusteredLights::prepareTiledLights(const bool clear_lights)
{
  if (tiledLights)
    tiledLights->computeTiledLigths(clear_lights);
}

void ClusteredLights::close() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  OSSpinlockScopedLock scopedLock{lightLock};
  lightsInitialized = false;
  dstReadbackLights.reset();
  resetShadows();
  lightsGrid.reset();
  lightsPartition.close();
  lightsCounter.reset();

  dynamicLightShadows.close();

  currentBufferSlot = LightBufferSlot::Main;
  buffersFilled.fill(false);
  lightsRenderer.close();
  shaders::overrides::destroy(depthBiasOverrideId);
  shaders::overrides::destroy(depthBiasTwoSidedOverrideId);
}

void ClusteredLights::setShadowBias(float z_bias, float slope_z_bias, float shader_z_bias, float shader_slope_z_bias)
{
  // todo: move depth bias depends to shader.
  // as, it depends on: wk, resolution, distance
  // however, distance can only be implemented in shader (and it is, but resolution independent)
  depthBiasOverrideState = shaders::OverrideState();
  depthBiasOverrideState.set(shaders::OverrideState::Z_BIAS);
  depthBiasOverrideState.zBias = z_bias;
  depthBiasOverrideState.slopeZBias = slope_z_bias;

  depthBiasOverrideId.reset(shaders::overrides::create(depthBiasOverrideState));
  if (lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    lightShadows->setOverrideState(depthBiasOverrideState);
  }

  shaderShadowZBias = shader_z_bias;
  shaderShadowSlopeZBias = shader_slope_z_bias;

  shaders::OverrideState depthBiasTwoSidedOverrideState = depthBiasOverrideState;
  depthBiasTwoSidedOverrideState.set(shaders::OverrideState::CULL_NONE);
  depthBiasTwoSidedOverrideId.reset(shaders::overrides::create(depthBiasTwoSidedOverrideState));
}

void ClusteredLights::getShadowBias(float &z_bias, float &slope_z_bias, float &shader_z_bias, float &shader_slope_z_bias) const
{
  z_bias = depthBiasOverrideState.zBias;
  slope_z_bias = depthBiasOverrideState.slopeZBias;
  shader_z_bias = shaderShadowZBias;
  shader_slope_z_bias = shaderShadowSlopeZBias;
}

void ClusteredLights::setRetainShadowSizeMul(float mul)
{
  if (lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    lightShadows->setRetainShadowSizeMul(mul);
  }
}

void ClusteredLights::renderOtherLights()
{
  G_ASSERT(buffersFilled[size_t(currentBufferSlot)]);
  if (hasDeferredOmniLights())
    lightsRenderer.renderFarOmniLights(lightsPartition.getVisibleFarOmniLightsCB(currentBufferSlot));
  if (hasDeferredSpotLights())
    lightsRenderer.renderFarSpotLights(lightsPartition.getVisibleFarSpotLightsCB(currentBufferSlot));
}

void ClusteredLights::setEmptyOutOfFrustumLights() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT(lightsInitialized);
  static Point4 c[2] = {Point4(0, 0, 0, 0), Point4(2 * OOF_GRID_W, 2 * OOF_GRID_VERT, 2 * OOF_GRID_W, 0)};
  ShaderGlobal::set_float4_array(out_of_frustum_omni_boxVarId, c, 2);
  ShaderGlobal::set_float4_array(out_of_frustum_spot_boxVarId, c, 2);
  outOfFrustumVisibleSpotLightsCB.reallocate(0, MAX_CLUSTERED_SPOT_LIGHTS, lightsResMgr.getResName("out_of_frustum_spot_lights"));
  outOfFrustumOmniLightsCB.reallocate(0, MAX_CLUSTERED_OMNI_LIGHTS, lightsResMgr.getResName("out_of_frustum_omni_lights"));

  dynamicLightShadows.setEmptyOutOfFrustumCommonLightShadowsCB();

  outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
  outOfFrustumOmniLightsCB.update(nullptr, 0);
  ShaderGlobal::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, dynamicLightShadows.getOutOfFrustumCommonLightShadowsCB().getId());
  ShaderGlobal::set_buffer(spot_lights_ssss_shadow_descVarId, BAD_D3DRESID);
}

bool ClusteredLights::cullOutOfFrustumLights(mat44f_cref globtm, SpotLightMaskType spot_light_mask, OmniLightMaskType omni_light_mask)
{
  G_ASSERT(lightsInitialized);
  Frustum frustum(globtm);
  vec4f unreachablePlane = v_make_vec4f(0, 0, 0, MAX_REAL);

  Tab<uint16_t> visibleFarOmniLightsId(framemem_ptr()), cVisibleOmniLightsId(framemem_ptr());
  Tab<uint16_t> visibleFarSpotLightsId(framemem_ptr()), cVisibleSpotLightsId(framemem_ptr());
  OSSpinlockScopedLock scopedLock{lightLock};

  LightsVisibilityChecker outOfFrustumVisibilityChecker(
    LightsVisibilityChecker::TestParameters{
      .frustum = frustum,
      .znearPlane = unreachablePlane,
      .omniRequireAnyMask = omni_light_mask,
      .spotRequireAnyMask = spot_light_mask,
    },
    LightsVisibilityChecker::Config{.omniLightsManager = &omniLights, .spotLightsManager = &spotLights});

  lightsPartition.executeLightsCPUPartition({
    .checker = outOfFrustumVisibilityChecker,
    .omniLightsFar = visibleFarOmniLightsId,
    .omniLightsClustered = cVisibleOmniLightsId,
    .spotLightsFar = visibleFarSpotLightsId,
    .spotLightsClustered = cVisibleSpotLightsId,
  });
  G_ASSERT(visibleFarOmniLightsId.size() == 0);
  cVisibleOmniLightsId.resize(min<int>(cVisibleOmniLightsId.size(), MAX_CLUSTERED_OMNI_LIGHTS));
  G_ASSERT(visibleFarSpotLightsId.size() == 0);
  cVisibleSpotLightsId.resize(min<int>(cVisibleSpotLightsId.size(), MAX_CLUSTERED_SPOT_LIGHTS));
  DA_PROFILE_TAG(outOfFrustumLights, "spots %d omnis %d", (int)cVisibleOmniLightsId.size(), (int)cVisibleSpotLightsId.size());

  const uint32_t spotWords = (cVisibleSpotLightsId.size() + 31) / 32, omniWords = (cVisibleOmniLightsId.size() + 31) / 32;

  dynamicLightShadows.updateOutOfFrustumCommonLightShadowsCB(cVisibleSpotLightsId, cVisibleOmniLightsId);

  outOfFrustumVisibleSpotLightsCB.reallocate(cVisibleSpotLightsId.size(), MAX_CLUSTERED_SPOT_LIGHTS,
    lightsResMgr.getResName("out_of_frustum_spot_lights"));
  bbox3f spotBox;
  v_bbox3_init_empty(spotBox);
  if (cVisibleSpotLightsId.size())
  {
    Tab<RenderSpotLight> outRenderSpotLights(framemem_ptr());
    outRenderSpotLights.resize(cVisibleSpotLightsId.size());
    for (int i = 0, ie = cVisibleSpotLightsId.size(); i < ie; ++i)
    {
      uint32_t id = cVisibleSpotLightsId[i];
      v_bbox3_add_box(spotBox, spotLights.getBoundingBox(id));
      outRenderSpotLights[i] = spotLights.getRenderLight(id);
    }
    outOfFrustumVisibleSpotLightsCB.update(outRenderSpotLights.data(), data_size(outRenderSpotLights));
  }
  else
  {
    outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
  }

  outOfFrustumOmniLightsCB.reallocate(cVisibleOmniLightsId.size(), MAX_CLUSTERED_OMNI_LIGHTS,
    lightsResMgr.getResName("out_of_frustum_omni_lights"));
  bbox3f omniBox;
  v_bbox3_init_empty(omniBox);
  if (cVisibleOmniLightsId.size())
  {
    Tab<RenderOmniLight> outRenderOmniLights(framemem_ptr());
    outRenderOmniLights.resize(cVisibleOmniLightsId.size());
    for (int i = 0, ie = cVisibleOmniLightsId.size(); i < ie; ++i)
    {
      outRenderOmniLights[i] = omniLights.getRenderLight(cVisibleOmniLightsId[i]);
      vec3f posAndRad = v_ldu(&outRenderOmniLights[i].posRadius.x);
      v_bbox3_add_pt(omniBox, v_add(posAndRad, v_splat_w(posAndRad)));
      v_bbox3_add_pt(omniBox, v_sub(posAndRad, v_splat_w(posAndRad)));
    }
    outOfFrustumOmniLightsCB.update(outRenderOmniLights.data(), data_size(outRenderOmniLights));
  }
  else
  {
    outOfFrustumOmniLightsCB.update(nullptr, 0);
  }

  const bool hasLights = !cVisibleSpotLightsId.empty() || !cVisibleOmniLightsId.empty();
  // todo: right now grid is of fixed size & fixed dimensions.
  // while there is may be some sense in make grid of fixed or at least capped size (to prevent reallocation)
  // but fixed dimensions doesn't make much sense! if we working with toroidal update, typical dimensions would be thin or narrow
  // so we'd better increase detalization over other dimensions
  // in order to do that:
  //  * calc bounding box from frustum (or directly pass box, not globtm)
  //  * intersect spot/omni boxes with this bounding box
  //  * calculate optimum dimensions (like ceil(box.width()/average light bounding radius)
  //  * clamp volume/adjust dimensions
  //  * pass dimensions to shader (rn it is hardcoded)
  const uint32_t omniGridOffset = 0, spotGridOffset = omniWords * OOF_GRID_SIZE;
  if (!cVisibleOmniLightsId.empty())
  {
    vec4f bmin = v_div(v_make_vec4f(OOF_GRID_W, OOF_GRID_VERT, OOF_GRID_W, 1), v_bbox3_size(omniBox));
    v_stu(&omniOOFBox[0].x, bmin);
    v_stu(&omniOOFBox[1].x, v_perm_xyzd(v_neg(v_mul(bmin, omniBox.bmin)), v_cast_vec4f(v_splatsi(omniGridOffset))));
  }
  else
  {
    omniOOFBox[0] = Point4(0, 0, 0, 0);
    omniOOFBox[1] = Point4(OOF_GRID_W * 2, OOF_GRID_VERT * 2, OOF_GRID_W * 2, 0);
  }

  if (!cVisibleSpotLightsId.empty())
  {
    vec4f bmin = v_div(v_make_vec4f(OOF_GRID_W, OOF_GRID_VERT, OOF_GRID_W, 1), v_bbox3_size(spotBox));
    v_stu(&spotOOFBox[0].x, bmin);
    v_stu(&spotOOFBox[1].x, v_perm_xyzd(v_neg(v_mul(bmin, spotBox.bmin)), v_cast_vec4f(v_splatsi(spotGridOffset))));
  }
  else
  {
    spotOOFBox[0] = Point4(0, 0, 0, 0);
    spotOOFBox[1] = Point4(OOF_GRID_W * 2, OOF_GRID_VERT * 2, OOF_GRID_W * 2, 0);
  }
  if (hasLights && cull_out_of_frustum_lights_cs && clear_out_of_frustum_grid_cs)
  {
    TIME_D3D_PROFILE(oof_clustering_lights);
    setOutOfFrustumLightsToShader();

    const uint32_t words = spotWords + omniWords;
    const uint32_t sz4 = (words * OOF_GRID_SIZE + 3) & ~3;
    if (!outOfFrustumLightsFullGridCB || outOfFrustumLightsFullGridCB.getBuf()->getSize() < sz4 * 4)
    {
      outOfFrustumLightsFullGridCB.close();
      outOfFrustumLightsFullGridCB = dag::create_sbuffer(sizeof(uint32_t), sz4, SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW, 0,
        "oof_lights_full_grid", RESTAG_LIGHTS);
    }
    ShaderGlobal::set_int(oof_clear_grid_sizeVarId, sz4 / 4);
    d3d::set_rwbuffer(STAGE_CS, 0, outOfFrustumLightsFullGridCB.getBuf());
    clear_out_of_frustum_grid_cs->dispatchThreads(sz4, 1, 1);
    d3d::resource_barrier({outOfFrustumLightsFullGridCB.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    const uint32_t totalLights = cVisibleOmniLightsId.size() + cVisibleSpotLightsId.size();
    {
      vec4f halfOmniCellSz = v_div(v_bbox3_size(omniBox), v_make_vec4f(2 * OOF_GRID_W, 2 * OOF_GRID_VERT, 2 * OOF_GRID_W, 1));
      halfOmniCellSz = v_perm_xyzd(halfOmniCellSz, v_cast_vec4f(v_splatsi(omniGridOffset)));
      vec4f halfSpotCellSz = v_div(v_bbox3_size(spotBox), v_make_vec4f(2 * OOF_GRID_W, 2 * OOF_GRID_VERT, 2 * OOF_GRID_W, 1));
      halfSpotCellSz = v_perm_xyzd(halfSpotCellSz, v_cast_vec4f(v_splatsi(spotGridOffset)));
      vec4f cullBox[4] = {v_add(omniBox.bmin, halfOmniCellSz), halfOmniCellSz, v_add(spotBox.bmin, halfSpotCellSz), halfSpotCellSz};
      Point4 cb[4];
      memcpy(cb, cullBox, sizeof(cullBox));
      ShaderGlobal::set_float4_array(out_of_frustum_grid_cull_boxVarId, cb, 4);
      cull_out_of_frustum_lights_cs->dispatchThreads(OOF_GRID_W, OOF_GRID_W, OOF_GRID_VERT * totalLights);
    }
    d3d::resource_barrier({outOfFrustumLightsFullGridCB.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  }

  return hasLights;
}

void ClusteredLights::cullFrustumLights(vec4f cur_view_pos, mat44f_cref globtm, mat44f_cref view, mat44f_cref proj, float znear,
  float zfar, Occlusion *occlusion, SpotLightMaskType spot_light_require_any_mask, OmniLightMaskType omni_light_require_any_mask,
  float light_cutoff_dist_sq)
{
  TIME_PROFILE(cullFrustumLights);
  buffersFilled.fill(false);
  Frustum frustum(globtm);
  plane3f clusteredLastPlane = shrink_zfar_plane(frustum.camPlanes[4], cur_view_pos, v_splats(maxClusteredDist));

  OSSpinlockScopedLock scopedLock{lightLock};

  LightsVisibilityChecker::TestParameters testParams{
    .frustum = frustum,
    .occlusion = occlusion,
    .znearPlane = clusteredLastPlane,
    .markSmallLightsAsFarLimit = MARK_SMALL_LIGHT_AS_FAR_LIMIT,
    .cameraPos = cur_view_pos,
    .omniRequireAnyMask = omni_light_require_any_mask,
    .spotRequireAnyMask = spot_light_require_any_mask,
    .cutoffDistSq = light_cutoff_dist_sq,
  };

  if (lightVisibilityChecker.has_value())
    lightVisibilityChecker = LightsVisibilityChecker(eastl::move(testParams), eastl::move(lightVisibilityChecker.value()));
  else
    lightVisibilityChecker.emplace(eastl::move(testParams),
      LightsVisibilityChecker::Config{.omniLightsManager = &omniLights, .spotLightsManager = &spotLights, .cacheResults = true});

  lightsCounter.update(lightVisibilityChecker.value());
  lightsPartition.prepareClusteredAndFarLightBuffersCPU(lightVisibilityChecker.value());

  const Tab<vec4f> &visibleOmniLightsBounds = lightsPartition.getVisibleClusteredOmniLightsBounds();
  const Tab<vec4f> &visibleSpotLightsBounds = lightsPartition.getVisibleClusteredSpotLightsBounds();

  if (lightsGrid.isGpu())
    lightsGrid.prepareGPUCulling(view, proj, znear, zfar, closeSliceDist, maxClusteredDist, visibleOmniLightsBounds.size(),
      visibleSpotLightsBounds.size());
  else
  {
    Tab<FrustumClusters::SpotsCullingData> visibleClusteredSpotLightsCullingData(framemem_ptr());
    const Tab<uint16_t> &visibleSpotLightsId = lightsPartition.getVisibleClusteredSpotLightsIds();
    visibleClusteredSpotLightsCullingData.resize(visibleSpotLightsId.size());

    for (int i = 0, e = visibleSpotLightsId.size(); i < e; ++i)
    {
      uint32_t id = visibleSpotLightsId[i];
      const SpotLight &light = spotLights.getLight(id);
      visibleClusteredSpotLightsCullingData[i].pos_radius = light.pos_radius;
      visibleClusteredSpotLightsCullingData[i].dir_tanHalfAngle = light.dir_tanHalfAngle;
    }

    lightsGrid.cullCPU(view, proj, znear, closeSliceDist, maxClusteredDist, visibleOmniLightsBounds,
      visibleClusteredSpotLightsCullingData, visibleSpotLightsBounds, occlusion);
  }

  if (tiledLights)
  {
    mat44f invView;
    v_mat44_orthonormal_inverse43(invView, view);
    vec4f cur_view_dir = invView.col2;
    tiledLights->prepare(visibleOmniLightsBounds, visibleSpotLightsBounds, cur_view_pos, cur_view_dir);
  }
}

void ClusteredLights::fillAndSetInsideOfFrustumLightsBuffers() DAG_TS_NO_THREAD_SAFETY_ANALYSIS /* read only lightShadows atlas size */
{
  if (buffersFilled[size_t(currentBufferSlot)])
    return;
  buffersFilled[size_t(currentBufferSlot)] = true;

  ShaderGlobal::set_float(depthSliceScaleVarId, clusters.depthSliceScale);
  ShaderGlobal::set_float(depthSliceBiasVarId, clusters.depthSliceBias);
  ShaderGlobal::set_float4(shadowAtlasTexelVarId,
    Color4(lightShadows ? 1.f / lightShadows->getAtlasWidth() : 1, lightShadows ? 1.f / lightShadows->getAtlasHeight() : 1, 0.f, 0.f));

  ShaderGlobal::set_float(shadowZBiasVarId, shaderShadowZBias);
  ShaderGlobal::set_float(shadowSlopeZBiasVarId, shaderShadowSlopeZBias);

  lightsPartition.updateBuffersForVisibleLightsCPU(currentBufferSlot);

  const auto &visibleFarSpotLightsCB = lightsPartition.getVisibleFarSpotLightsCB(currentBufferSlot);
  const auto &visibleFarOmniLightsCB = lightsPartition.getVisibleFarOmniLightsCB(currentBufferSlot);

  const auto &visibleClusteredSpotLightsCB = lightsPartition.getVisibleClusteredSpotLightsCB(currentBufferSlot);
  const auto &visibleClusteredOmniLightsCB = lightsPartition.getVisibleClusteredOmniLightsCB(currentBufferSlot);

  {
    Sbuffer *lightsCBs[] = {visibleClusteredSpotLightsCB.getBuf(), visibleClusteredOmniLightsCB.getBuf(),
      visibleFarSpotLightsCB.getBuf(), visibleFarOmniLightsCB.getBuf()};
    ResourceBarrier copySourceStates[] = {RB_RO_COPY_SOURCE, RB_RO_COPY_SOURCE, RB_RO_COPY_SOURCE, RB_RO_COPY_SOURCE};
    d3d::resource_barrier({lightsCBs, copySourceStates});
    lightsRenderer.copyInstanceCountsToIndirectArgs(
      LightsRenderer::OmniLightsCBs{.far = &visibleFarOmniLightsCB, .clustered = &visibleClusteredOmniLightsCB},
      LightsRenderer::SpotLightsCBs{.far = &visibleFarSpotLightsCB, .clustered = &visibleClusteredSpotLightsCB});
    const ResourceBarrier constantBufferState = RB_RO_CONSTANT_BUFFER | RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE;
    ResourceBarrier constantBufferStates[] = {constantBufferState, constantBufferState, constantBufferState, constantBufferState};
    d3d::resource_barrier({lightsCBs, constantBufferStates});
  }

  ShaderGlobal::set_buffer(omni_lightsVarId, visibleClusteredOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, visibleClusteredSpotLightsCB.getId());

  if (const UniqueBuf &omniMasksSB = lightsPartition.getVisibleClusteredOmniLightsMasksSB())
  {
    if (lightsPartition.isGpu())
      d3d::resource_barrier({omniMasksSB.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    ShaderGlobal::set_buffer(omni_lights_flagsVarId, omniMasksSB);
  }
  if (const UniqueBuf &spotMasksSB = lightsPartition.getVisibleClusteredSpotLightsMasksSB())
  {
    if (lightsPartition.isGpu())
      d3d::resource_barrier({spotMasksSB.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    ShaderGlobal::set_buffer(spot_lights_flagsVarId, spotMasksSB);
  }

  lightsGrid.fill();

  if (tiledLights)
    tiledLights->applyBinning();

  if (!lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    updateShadowBuffers();
  }
}

void ClusteredLights::setResolution(uint32_t width, uint32_t height)
{
  if (tiledLights)
    tiledLights->setResolution(width, height);
}

void ClusteredLights::changeResolution(uint32_t width, uint32_t height)
{
  if (tiledLights)
    tiledLights->changeResolution(width, height);
}

void ClusteredLights::changeShadowResolutionByQuality(uint32_t shadow_quality, bool dynamic_shadow_32bit)
{
  auto &drvDesc = d3d::get_driver_desc();
  const uint32_t res = min<uint32_t>(1024 * shadow_quality, min(drvDesc.maxtexw, drvDesc.maxtexh));
  const uint32_t qMul = clamp<uint32_t>(res / 1024, 1, MAX_SHADOWS_QUALITY);

  lightShadows->changeResolution(res, 256 * qMul, 64 * qMul, 64 * qMul, dynamic_shadow_32bit);
}

void ClusteredLights::resetShadows()
{
  dynamicLightShadows.resetShadowVolumeUpdateFlags();
  dynamicLightShadows.setShadowSystem(nullptr);
  omniLights.closeShadows();
  spotLights.closeShadows();
  lightShadows.reset();
}

void ClusteredLights::changeShadowResolution(uint32_t shadow_quality, bool dynamic_shadow_32bit)
{
  OSSpinlockScopedLock scopedLock{lightLock};
  if (!lightShadows && shadow_quality > 0)
  {
    dstReadbackLights.reset();
    lightShadows.reset();
    lightShadows = eastl::make_unique<ShadowSystem>(&lightsResMgr);
    lightShadows->setOverrideState(depthBiasOverrideState);
    dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights, &lightsResMgr);
  }

  omniLights.setShadowSystem(lightShadows.get());
  spotLights.setShadowSystem(lightShadows.get());
  dynamicLightShadows.setShadowSystem(lightShadows.get());

  if (lightShadows)
  {
    changeShadowResolutionByQuality(shadow_quality, dynamic_shadow_32bit);
    invalidateAllShadows();
  }
}

void ClusteredLights::toggleTiledLights(bool use_tiled)
{
  if (!use_tiled)
    tiledLights.reset();
  else if (!tiledLights)
    initTiledLights();
}

void ClusteredLights::init(int frame_initial_lights_count, uint32_t shadow_quality, bool use_tiled_lights,
  const char *name_suffix) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  lightsResMgr = LightsResourcesManager(name_suffix);
  lightsInitialized = true;
  if (shadow_quality)
  {
    lightShadows = eastl::make_unique<ShadowSystem>(&lightsResMgr);
    lightShadows->setOverrideState(depthBiasOverrideState);
    changeShadowResolutionByQuality(shadow_quality, false);
  }

  const auto defaultDynLightsFeatures = DynLightsFeatures::GpuClusteredLightsGrid | DynLightsFeatures::GpuTiledLightsZBinning;
  dynLightsFeatures = static_cast<DynLightsFeatures>(
    dgs_get_settings()->getBlockByNameEx("graphics")->getInt("dynLightsFeatures", static_cast<int>(defaultDynLightsFeatures)));

  const auto gpuLightsCullingValues = dynLightsFeatures & DynLightsFeatures::GpuLightsCulling;
  if (gpuLightsCullingValues != DynLightsFeatures::CpuOnly && gpuLightsCullingValues != DynLightsFeatures::GpuLightsCulling)
  {
    logerr("ClusteredLights::init - invalid combination of dyn lights features : %d", static_cast<int>(gpuLightsCullingValues));
    dynLightsFeatures &= ~DynLightsFeatures::GpuLightsCulling;
  }

  // TODO: remove temporary force disable gpu lights culling
  dynLightsFeatures &= ~DynLightsFeatures::GpuLightsCulling;

  omniLights.init(dynLightsFeatures);
  spotLights.init(dynLightsFeatures);
  omniLights.setShadowSystem(lightShadows.get());
  spotLights.setShadowSystem(lightShadows.get());

  const DynLightsFeatures managersFeatures = omniLights.getRuntimeFeatures() & spotLights.getRuntimeFeatures();
  dynLightsFeatures &=
    ~DynLightsFeatures::GpuLightsManagerPersistentStorage | (managersFeatures & DynLightsFeatures::GpuLightsManagerPersistentStorage);

  const DynLightsFeatures partitionBits = DynLightsFeatures::GpuLightsPartition | DynLightsFeatures::GpuLightsSort;
  if (!static_cast<bool>(dynLightsFeatures & DynLightsFeatures::GpuLightsManagerPersistentStorage))
  {
    dynLightsFeatures &= ~partitionBits;
  }

  lightsPartition.init(dynLightsFeatures);
  dynLightsFeatures &= ~partitionBits | (lightsPartition.getRuntimeFeatures() & partitionBits);

  if ((dynLightsFeatures & partitionBits) != partitionBits)
  {
    dynLightsFeatures &= ~(DynLightsFeatures::GpuLightShadowsPersistentStorage);
  }

  dynamicLightShadows.init(dynLightsFeatures);
  dynLightsFeatures &= ~DynLightsFeatures::GpuLightShadowsPersistentStorage |
                       (dynamicLightShadows.getRuntimeFeatures() & DynLightsFeatures::GpuLightShadowsPersistentStorage);
  dynamicLightShadows.setShadowSystem(lightShadows.get());

  initClustered(frame_initial_lights_count);
  lightsRenderer.init();

  if (lightShadows)
    dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights, &lightsResMgr);

  omni_lightsVarId = ::get_shader_variable_id("omni_lights", false);
  spot_lightsVarId = ::get_shader_variable_id("spot_lights", false);
  common_lights_shadowsVarId = ::get_shader_variable_id("common_lights_shadows", false);
  spot_lights_ssss_shadow_descVarId = ::get_shader_variable_id("spot_lights_ssss_shadow_desc", true);
  tiledLights.reset();
  if (use_tiled_lights)
    initTiledLights();
}

void ClusteredLights::initTiledLights()
{
  tiledLights = eastl::make_unique<TiledLights>(maxClusteredDist);
  tiledLights->init(dynLightsFeatures);
  dynLightsFeatures &=
    ~DynLightsFeatures::GpuTiledLightsZBinning | (tiledLights->getRuntimeFeatures() & DynLightsFeatures::GpuTiledLightsZBinning);
}

void ClusteredLights::setMaxClusteredDist(const float max_clustered_dist)
{
  maxClusteredDist = max_clustered_dist;
  if (tiledLights)
    tiledLights->setMaxLightsDist(maxClusteredDist);
}

void ClusteredLights::setMaxShadowDist(const float max_shadow_dist) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  dynamicLightShadows.setMaxShadowDist(max_shadow_dist);
}

void ClusteredLights::renderDebugOmniLights()
{
  if (hasClusteredOmniLights() || hasDeferredOmniLights())
  {
    G_ASSERT(buffersFilled[size_t(currentBufferSlot)]);
    lightsRenderer.renderDebugOmniLights(
      LightsRenderer::OmniLightsCBs{.far = &lightsPartition.getVisibleFarOmniLightsCB(currentBufferSlot),
        .clustered = &lightsPartition.getVisibleClusteredOmniLightsCB(currentBufferSlot)});
  }
}

void ClusteredLights::renderDebugSpotLights()
{
  if (hasClusteredSpotLights() || hasDeferredSpotLights())
  {
    G_ASSERT(buffersFilled[size_t(currentBufferSlot)]);
    lightsRenderer.renderDebugSpotLights(
      LightsRenderer::SpotLightsCBs{.far = &lightsPartition.getVisibleFarSpotLightsCB(currentBufferSlot),
        .clustered = &lightsPartition.getVisibleClusteredSpotLightsCB(currentBufferSlot)});
  }
}

void ClusteredLights::renderDebugLights()
{
  OSSpinlockScopedLock scopedLock{lightLock};
  renderDebugSpotLights();
  renderDebugOmniLights();
}

void ClusteredLights::renderDebugLightsBboxes()
{
  OSSpinlockScopedLock scopedLock{lightLock};
  spotLights.renderDebugBboxes();
  omniLights.renderDebugBboxes();
}

void ClusteredLights::destroyLight(uint32_t id)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  OSSpinlockScopedLock scopedLock{lightLock};
  switch (typeId.type)
  {
    case LightType::Spot: spotLights.destroyLight(typeId.id); break;
    case LightType::Omni: omniLights.destroyLight(typeId.id); break;
    case LightType::Invalid: return;
    default: G_ASSERT_FAIL("unknown light type");
  }

  if (!lightShadows)
    return;

  switch (typeId.type)
  {
    case LightType::Invalid: return;
    case LightType::Spot:
      if (spotLights.isShadowVolumeAllocated(typeId.id))
        spotLights.destroyShadowVolume(typeId.id);
      break;
    case LightType::Omni:
      if (omniLights.isShadowVolumeAllocated(typeId.id))
        omniLights.destroyShadowVolume(typeId.id);
      break;
    default: break;
  }
}

uint32_t ClusteredLights::addOmniLight(const OmniLight &light, OmniLightMaskType mask)
{
  OSSpinlockScopedLock scopedLock{lightLock};
  int id = omniLights.addLight(light);
  if (id < 0)
    return LightsEncoder::INVALID_LIGHT;
  omniLights.setLightMask(id, mask);
  return LightsEncoder::encodeLightId(LightType::Omni, id);
}

// keep mask
void ClusteredLights::setLightNoLock(uint32_t id, const OmniLight &light, bool invalidate_shadow)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return,
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());

  if (invalidate_shadow && omniLights.tryInvalidateShadowsIfNeed(typeId.id, light))
  {
    dynamicLightShadows.resetShadowVolumeUpdateFlag(omniLights.getShadowId(typeId.id));
  }
  omniLights.setLight(typeId.id, light);
}

void ClusteredLights::setLight(uint32_t id, const OmniLight &light, bool invalidate_shadow)
{
  OSSpinlockScopedLock scopedLock{lightLock};
  return setLightNoLock(id, light, invalidate_shadow);
}

void ClusteredLights::setLightWithMask(uint32_t id, const OmniLight &light, OmniLightMaskType mask, bool invalidate_shadow)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  OSSpinlockScopedLock scopedLock{lightLock};
  G_ASSERTF_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return,
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());

  if (invalidate_shadow && omniLights.tryInvalidateShadowsIfNeed(typeId.id, light))
  {
    dynamicLightShadows.resetShadowVolumeUpdateFlag(omniLights.getShadowId(typeId.id));
  }

  omniLights.setLight(typeId.id, light);
  omniLights.setLightMask(typeId.id, mask);
}

const ClusteredLights::OmniLight &ClusteredLights::getOmniLightNoLock(uint32_t id) const
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  static const OmniLight emptyLight{};
  G_ASSERTF_RETURN(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), emptyLight,
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());
  return omniLights.getLight(id);
}

ClusteredLights::OmniLight ClusteredLights::getOmniLight(uint32_t id) const
{
  OSSpinlockScopedLock scopedLock{lightLock};
  return getOmniLightNoLock(id);
}

OmniLightMaskType ClusteredLights::getOmniLightMask(uint32_t id) const
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  OSSpinlockScopedLock scopedLock{lightLock};
  G_ASSERTF_RETURN(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), OmniLightMaskType::OMNI_LIGHT_MASK_NONE,
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());
  return omniLights.getLightMask(typeId.id);
}

void ClusteredLights::setLightNoLock(uint32_t id, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), return,
    "(%s) light %d is invalid (maxIndex= %d)", typeId.type == LightType::Spot ? "spot" : "omni", typeId.id, spotLights.maxIndex());

  if (invalidate_shadow && spotLights.tryInvalidateShadowsIfNeed(typeId.id, light))
  {
    dynamicLightShadows.resetShadowVolumeUpdateFlag(spotLights.getShadowId(typeId.id));
  }

  spotLights.setLight(typeId.id, light);
  spotLights.setLightMask(typeId.id, mask);
}

void ClusteredLights::setLight(uint32_t id, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow)
{
  OSSpinlockScopedLock scopedLock{lightLock};
  return setLightNoLock(id, light, mask, invalidate_shadow);
}

const ClusteredLights::SpotLight &ClusteredLights::getSpotLightNoLock(uint32_t id) const
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  static const SpotLight emptyLight{};
  G_ASSERTF_RETURN(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), emptyLight,
    "(%s) light %d is invalid (maxIndex= %d)", typeId.type == LightType::Spot ? "spot" : "omni", id, spotLights.maxIndex());
  return spotLights.getLight(typeId.id);
}

ClusteredLights::SpotLight ClusteredLights::getSpotLight(uint32_t id) const
{
  OSSpinlockScopedLock scopedLock{lightLock};
  return getSpotLightNoLock(id);
}

SpotLightMaskType ClusteredLights::getSpotLightMask(uint32_t id) const
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  OSSpinlockScopedLock scopedLock{lightLock};
  G_ASSERTF_RETURN(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), SpotLightMaskType::SPOT_LIGHT_MASK_NONE,
    "spot light %d is invalid (maxIndex= %d)", typeId.id, spotLights.maxIndex());
  return spotLights.getLightMask(typeId.id);
}

void ClusteredLights::getSpotLightShadowViewProj(uint32_t id, mat44f &view_itm, mat44f &proj)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  G_ASSERT_RETURN(typeId.type == LightType::Spot, );
  OSSpinlockScopedLock scopedLock{lightLock};
  spotLights.getLightView(typeId.id, view_itm);
  spotLights.getLightPersp(typeId.id, proj);
}

bool ClusteredLights::isLightVisible(uint32_t id) const
{
  OSSpinlockScopedLock scopedLock{lightLock};
  const auto typeId = LightsEncoder::decodeLightId(id);
  if (typeId.type == LightType::Invalid)
    return false;
  if (lightVisibilityChecker.has_value())
    return lightVisibilityChecker->testVisibility(typeId.type, typeId.id);
  return false;
}

uint32_t ClusteredLights::addSpotLight(const SpotLight &light, SpotLightMaskType mask)
{
  OSSpinlockScopedLock scopedLock{lightLock};
  int id = spotLights.addLight(light);
  if (id < 0)
    return LightsEncoder::INVALID_LIGHT;
  spotLights.setLightMask(id, mask);
  return LightsEncoder::encodeLightId(LightType::Spot, id);
}

bool ClusteredLights::addShadowToLight(uint32_t id, ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
  uint8_t max_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  OSSpinlockScopedLock scopedLock{lightLock};

  if (!lightShadows)
    return false;

  switch (typeId.type)
  {
    case LightType::Spot:
    {
      const auto shadowId =
        spotLights.allocateShadowVolume(typeId.id, casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
      if (shadowId == INVALID_SHADOW_VOLUME_ID)
        return false;
      spotLights.setLightShadows(typeId.id, true);
      dynamicLightShadows.resetShadowVolumeUpdateFlag(shadowId);
    }
    break;
    case LightType::Omni:
    {
      const auto shadowId =
        omniLights.allocateShadowVolume(typeId.id, casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
      if (shadowId == INVALID_SHADOW_VOLUME_ID)
        return false;
      dynamicLightShadows.resetShadowVolumeUpdateFlag(shadowId);
    }
    break;
    case LightType::Invalid: return false;
    default: G_ASSERT_FAIL("unknown light type");
  }
  return true;
}

bool ClusteredLights::getShadowProperties(uint32_t id, ShadowCastersFlag &casters, bool &hint_dynamic, uint16_t &quality,
  uint8_t &priority, uint8_t &shadow_size_srl, DynamicShadowRenderGPUObjects &render_gpu_objects) const
{
  if (!lightShadows)
    return false;

  const auto typeId = LightsEncoder::decodeLightId(id);
  if (typeId.type == LightType::Invalid)
    return false;

  OSSpinlockScopedLock scopedLock{lightLock};
  const auto lightShadow = typeId.type == LightType::Spot ? spotLights.getShadowId(typeId.id) : omniLights.getShadowId(typeId.id);
  if (lightShadow == INVALID_SHADOW_VOLUME_ID)
    return false;

  return lightShadows->getShadowProperties(lightShadow, casters, hint_dynamic, quality, priority, shadow_size_srl, render_gpu_objects);
}

void ClusteredLights::removeShadow(uint32_t id)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  OSSpinlockScopedLock scopedLock{lightLock};
  switch (typeId.type)
  {
    case LightType::Invalid: return;
    case LightType::Spot: spotLights.setLightShadows(typeId.id, false); break;
    default: break;
  }

  if (!lightShadows)
    return;

  switch (typeId.type)
  {
    case LightType::Invalid: return;
    case LightType::Spot:
      if (spotLights.isShadowVolumeAllocated(typeId.id))
        spotLights.destroyShadowVolume(typeId.id);
      break;
    case LightType::Omni:
      if (omniLights.isShadowVolumeAllocated(typeId.id))
        omniLights.destroyShadowVolume(typeId.id);
      break;
    default: break;
  }
}

void ClusteredLights::invalidateAllShadows()
{
  if (lightShadows)
    lightShadows->invalidateAllVolumes();
}
void ClusteredLights::invalidateStaticObjects(bbox3f_cref box)
{
  if (lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    lightShadows->invalidateStaticObjects(box);
  }
}
void ClusteredLights::shrinkShadowVolumes()
{
  if (lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    lightShadows->shrink();
  }
}

dynamic_shadow_render::QualityParams ClusteredLights::getQualityParams() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  return dynamicLightShadows.getQualityParams();
}

void ClusteredLights::framePrepareShadows(dynamic_shadow_render::FrameVolumeData &volume_data, const Point3 &viewPos,
  mat44f_cref globtm, float camera_focal, dag::ConstSpan<bbox3f> dynamicBoxes, bool collect_updates)
{
  OSSpinlockScopedLock scopedLock{lightLock};

  const LightsVisibilityChecker *checker = nullptr;
  if (lightVisibilityChecker.has_value())
    checker = &lightVisibilityChecker.value();

  dynamicLightShadows.framePrepareShadows(volume_data, {
                                                         .lightVisibilityChecker = checker,
                                                         .viewPos = viewPos,
                                                         .globtm = globtm,
                                                         .cameraFocal = camera_focal,
                                                         .dynamicBoxes = dynamicBoxes,
                                                         .collectUpdates = collect_updates,
                                                       });
}

void ClusteredLights::frameRenderShadows(const dynamic_shadow_render::FrameVolumeData &volume_data, StaticRenderCallback renderStatic,
  DynamicRenderCallback renderDynamic)
{
  if ((lightsPartition.getVisibleClusteredSpotLightsIds().empty() && lightsPartition.getVisibleClusteredOmniLightsIds().empty()) ||
      !lightShadows)
    return;

  SCOPE_VIEW_PROJ_MATRIX;
  SCOPE_RENDER_TARGET;
  OSSpinlockUniqueLock scopedLock{lightLock};
  dynamicLightShadows.frameRenderShadows(volume_data, renderStatic, renderDynamic, depthBiasOverrideId.get(),
    depthBiasTwoSidedOverrideId.get());

  updateShadowBuffers();
  scopedLock.unlock();

  shaders::overrides::set(depthBiasOverrideId);
  dstReadbackLights->update(renderStatic);
  shaders::overrides::reset();
}

void ClusteredLights::updateShadowBuffers()
{
  dynamicLightShadows.updateVisibleLightShadowBuffersCPU(currentBufferSlot, lightsPartition.getVisibleClusteredSpotLightsIds(),
    lightsPartition.getVisibleClusteredOmniLightsIds());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId,
    dynamicLightShadows.getInFrustumCommonLightShadowsCB(currentBufferSlot).getId());
  ShaderGlobal::set_buffer(spot_lights_ssss_shadow_descVarId, dynamicLightShadows.getSpotLightSsssShadowDescBufferId());
}

void ClusteredLights::setOutOfFrustumLightsToShader() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT(lightsInitialized);
  ShaderGlobal::set_float4_array(out_of_frustum_omni_boxVarId, omniOOFBox, 2);
  ShaderGlobal::set_float4_array(out_of_frustum_spot_boxVarId, spotOOFBox, 2);
  ShaderGlobal::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, dynamicLightShadows.getOutOfFrustumCommonLightShadowsCB().getId());
  ShaderGlobal::set_buffer(spot_lights_ssss_shadow_descVarId, BAD_D3DRESID);
}

void ClusteredLights::setInsideOfFrustumLightsToShader() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT(lightsInitialized);
  ShaderGlobal::set_buffer(omni_lightsVarId, lightsPartition.getVisibleClusteredOmniLightsCB(currentBufferSlot).getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, lightsPartition.getVisibleClusteredSpotLightsCB(currentBufferSlot).getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId,
    dynamicLightShadows.getInFrustumCommonLightShadowsCB(currentBufferSlot).getId());
  ShaderGlobal::set_buffer(spot_lights_ssss_shadow_descVarId, dynamicLightShadows.getSpotLightSsssShadowDescBufferId());
}

void ClusteredLights::selectBufferSlot(LightBufferSlot slot) { currentBufferSlot = slot; }

void ClusteredLights::beforeResetDevice()
{
  lightsRenderer.beforeResetDevice();
  if (tiledLights)
    tiledLights->beforeResetDevice();
}

void ClusteredLights::afterResetDevice()
{
  lightsRenderer.afterResetDevice();

  {
    OSSpinlockScopedLock scopedLock{lightLock};
    omniLights.afterResetDevice();
    spotLights.afterResetDevice();
  }

  if (lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    lightShadows->afterReset();
  }

  if (tiledLights)
    tiledLights->afterResetDevice();

  if (dstReadbackLights)
    dstReadbackLights->afterResetDevice();

  {
    OSSpinlockScopedLock scopedLock{lightLock};
    dynamicLightShadows.afterResetDevice();
  }
}

bbox3f ClusteredLights::getActiveShadowVolume() const
{
  if (!lightShadows)
  {
    bbox3f ret;
    v_bbox3_init_empty(ret);
    return ret;
  }
  OSSpinlockScopedLock scopedLock{lightLock};
  return lightShadows->getActiveShadowVolume();
}

void ClusteredLights::setNeedSsss(bool need_ssss) DAG_TS_NO_THREAD_SAFETY_ANALYSIS { dynamicLightShadows.setNeedSsss(need_ssss); }

void ClusteredLights::setMaxShadowsToUpdateOnFrame(int max_shadows) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  dynamicLightShadows.setMaxShadowsToUpdateOnFrame(max_shadows);
}

void ClusteredLights::setMaxShadowViewsToUpdateOnFrame(int max_views) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  dynamicLightShadows.setMaxShadowViewsToUpdateOnFrame(max_views);
}
