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
#include <generic/dag_align.h>

static const uint32_t MAX_SHADOWS_QUALITY = 8u;

const float ClusteredLights::MARK_SMALL_LIGHT_AS_FAR_LIMIT = 0.03;

static int omni_lightsVarId = -1;
static int spot_lightsVarId = -1;
static int common_lights_shadowsVarId = -1;

static int depthSliceScaleVarId = -1, depthSliceBiasVarId = -1;
static int shadowAtlasTexelVarId = -1;
static int shadowDistScaleVarId = -1, shadowDistBiasVarId = -1;
static int shadowZBiasVarId = -1, shadowSlopeZBiasVarId = -1;

static int spot_lights_flagsVarId = -1, omni_lights_flagsVarId = -1;

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

  depthSliceScaleVarId = get_shader_variable_id("depthSliceScale");
  depthSliceBiasVarId = get_shader_variable_id("depthSliceBias");
  shadowAtlasTexelVarId = get_shader_variable_id("shadowAtlasTexel");
  shadowDistScaleVarId = get_shader_variable_id("shadowDistScale");
  shadowDistBiasVarId = get_shader_variable_id("shadowDistBias");
  shadowZBiasVarId = get_shader_variable_id("shadowZBias");
  shadowSlopeZBiasVarId = get_shader_variable_id("shadowSlopeZBias");
  spot_lights_flagsVarId = get_shader_variable_id("spot_lights_flags", true);
  omni_lights_flagsVarId = get_shader_variable_id("omni_lights_flags", true);
}

ClusteredLights::ClusteredLights(const char *name_suffix) :
  lightsResMgr(name_suffix), lightsRenderer(&lightsResMgr), lightsPartition(omniLights, spotLights, &lightsResMgr)
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

int ClusteredLights::getVisibleFarSpotsCount() const { return lightsPartition.getRenderSpotLightsFar().size(); }
int ClusteredLights::getVisibleFarOmniCount() const { return lightsPartition.getRenderOmniLightsFar().size(); }
int ClusteredLights::getVisibleClusteredSpotsCount() const { return lightsGrid.getSpotCount(); };
int ClusteredLights::getVisibleClusteredOmniCount() const { return lightsGrid.getOmniCount(); }
int ClusteredLights::getVisibleSpotsCount() const { return getVisibleClusteredSpotsCount() + getVisibleFarSpotsCount(); }
int ClusteredLights::getVisibleOmniCount() const { return getVisibleClusteredOmniCount() + getVisibleFarOmniCount(); }

DynLightsOptimizationMode ClusteredLights::getLightsCountInterval() const
{
  const uint32_t spotsCount = getVisibleClusteredSpotsCount();
  const uint32_t omniCount = getVisibleClusteredOmniCount();

  if (spotsCount == 0 && omniCount == 0)
    return DynLightsOptimizationMode::NO_LIGHTS;

  constexpr int THRESHOLD = LIGHTS_OPTIMIZATION_THRESHOLD;
  if (spotsCount == 0 && omniCount <= THRESHOLD)
    return DynLightsOptimizationMode::NO_SPOTS_FEW_OMNI;
  else if (spotsCount <= THRESHOLD && omniCount == 0)
    return DynLightsOptimizationMode::FEW_SPOTS_NO_OMNI;
  else if (spotsCount <= THRESHOLD && omniCount <= THRESHOLD)
    return DynLightsOptimizationMode::FEW_SPOTS_FEW_OMNI;
  return DynLightsOptimizationMode::FULL_CLUSTERED;
}

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

  spotLightSsssShadowDescBuffer.close();

  commonLightShadowsBufferCB.close();
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
  G_ASSERT(buffersFilled);
  if (hasDeferredOmniLights())
    lightsRenderer.renderFarOmniLights(lightsPartition.getVisibleFarOmniLightsCB());
  if (hasDeferredSpotLights())
    lightsRenderer.renderFarSpotLights(lightsPartition.getVisibleFarSpotLightsCB());
}

void ClusteredLights::setEmptyOutOfFrustumLights()
{
  G_ASSERT(lightsInitialized);
  static Point4 c[2] = {Point4(0, 0, 0, 0), Point4(2 * OOF_GRID_W, 2 * OOF_GRID_VERT, 2 * OOF_GRID_W, 0)};
  ShaderGlobal::set_float4_array(out_of_frustum_omni_boxVarId, c, 2);
  ShaderGlobal::set_float4_array(out_of_frustum_spot_boxVarId, c, 2);
  outOfFrustumVisibleSpotLightsCB.reallocate(0, MAX_CLUSTERED_SPOT_LIGHTS, lightsResMgr.getResName("out_of_frustum_spot_lights"));
  outOfFrustumOmniLightsCB.reallocate(0, MAX_CLUSTERED_OMNI_LIGHTS, lightsResMgr.getResName("out_of_frustum_omni_lights"));

  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  outOfFrustumCommonLightsShadowsCB.reallocate(1, 1 + MAX_CLUSTERED_SPOT_LIGHTS * 5 + MAX_CLUSTERED_OMNI_LIGHTS,
    lightsResMgr.getResName("out_of_frustum_common_lights_shadow_data"), true /*persistent*/);

  outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
  outOfFrustumOmniLightsCB.update(nullptr, 0);
  if (commonLightsShadowsAreEmpty == false)
    outOfFrustumCommonLightsShadowsCB.update(nullptr, 0);
  commonLightsShadowsAreEmpty = true;
  ShaderGlobal::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, outOfFrustumCommonLightsShadowsCB.getId());
}

bool ClusteredLights::cullOutOfFrustumLights(mat44f_cref globtm, SpotLightMaskType spot_light_mask, OmniLightMaskType omni_light_mask)
{
  G_ASSERT(lightsInitialized);
  Frustum frustum(globtm);
  vec4f unreachablePlane = v_make_vec4f(0, 0, 0, MAX_REAL);

  Tab<uint16_t> visibleFarOmniLightsId(framemem_ptr()), cVisibleOmniLightsId(framemem_ptr());
  OSSpinlockScopedLock scopedLock{lightLock};
  lightsPartition.executeOmniLightsCPUPartition(frustum, visibleFarOmniLightsId, cVisibleOmniLightsId, nullptr, unreachablePlane, 0,
    v_zero(), omni_light_mask);
  G_ASSERT(visibleFarOmniLightsId.size() == 0);
  cVisibleOmniLightsId.resize(min<int>(cVisibleOmniLightsId.size(), MAX_CLUSTERED_OMNI_LIGHTS));


  Tab<uint16_t> visibleFarSpotLightsId(framemem_ptr()), cVisibleSpotLightsId(framemem_ptr());
  lightsPartition.executeSpotLightsCPUPartition(frustum, visibleFarSpotLightsId, cVisibleSpotLightsId, nullptr, nullptr,
    unreachablePlane, spot_light_mask);
  G_ASSERT(visibleFarSpotLightsId.size() == 0);
  cVisibleSpotLightsId.resize(min<int>(cVisibleSpotLightsId.size(), MAX_CLUSTERED_SPOT_LIGHTS));
  DA_PROFILE_TAG(outOfFrustumLights, "spots %d omnis %d", (int)cVisibleOmniLightsId.size(), (int)cVisibleSpotLightsId.size());

  const uint32_t spotWords = (cVisibleSpotLightsId.size() + 31) / 32, omniWords = (cVisibleOmniLightsId.size() + 31) / 32;


  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  outOfFrustumCommonLightsShadowsCB.reallocate(1 + cVisibleSpotLightsId.size() * 5 + cVisibleOmniLightsId.size(),
    1 + MAX_CLUSTERED_SPOT_LIGHTS * 5 + MAX_CLUSTERED_OMNI_LIGHTS, lightsResMgr.getResName("out_of_frustum_common_lights_shadow_data"),
    true /*persistent*/);

  // Per spot: 4 float4 tex matrix rows + 1 float4 atlas-UV bounds (rectMin.xy, rectMax.xy).
  StaticTab<Point4, 1 + MAX_CLUSTERED_SPOT_LIGHTS * 5 + MAX_CLUSTERED_OMNI_LIGHTS> commonShadowData;
  commonShadowData.resize(1 + cVisibleSpotLightsId.size() * 5 + cVisibleOmniLightsId.size());
  commonShadowData[0] = Point4(cVisibleSpotLightsId.size(), cVisibleOmniLightsId.size(), 5 * cVisibleSpotLightsId.size(), 0);

  outOfFrustumVisibleSpotLightsCB.reallocate(cVisibleSpotLightsId.size(), MAX_CLUSTERED_SPOT_LIGHTS,
    lightsResMgr.getResName("out_of_frustum_spot_lights"));
  int baseIndex = 1;
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
    for (int i = 0, ie = cVisibleSpotLightsId.size(); i < ie; ++i)
    {
      const auto shadowId = spotLights.getShadowId(cVisibleSpotLightsId[i]);
      if (shadowId != INVALID_SHADOW_VOLUME_ID && lightShadows->hasVolumeEverBeenRendered(shadowId))
      {
        memcpy(&commonShadowData[baseIndex + i * 5], &lightShadows->getVolumeTexMatrix(shadowId), 4 * sizeof(Point4));
        commonShadowData[baseIndex + i * 5 + 4] = lightShadows->getShadowUvMinMax(shadowId);
      }
      else
      {
        memset(&commonShadowData[baseIndex + i * 5], 0, 4 * sizeof(Point4));
        commonShadowData[baseIndex + i * 5 + 4] = Point4(0, 0, 1, 1);
      }
    }
  }
  else
  {
    outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
  }

  outOfFrustumOmniLightsCB.reallocate(cVisibleOmniLightsId.size(), MAX_CLUSTERED_OMNI_LIGHTS,
    lightsResMgr.getResName("out_of_frustum_omni_lights"));
  baseIndex += cVisibleSpotLightsId.size() * 5;
  bbox3f omniBox;
  v_bbox3_init_empty(omniBox);
  if (cVisibleOmniLightsId.size())
  {
    Tab<OmniLightsManager::RawLight> outRenderOmniLights(framemem_ptr());
    outRenderOmniLights.resize(cVisibleOmniLightsId.size());
    for (int i = 0, ie = cVisibleOmniLightsId.size(); i < ie; ++i)
    {
      auto &l = omniLights.getLight(cVisibleOmniLightsId[i]);
      vec3f posAndRad = v_ld(&l.pos_radius.x);
      v_bbox3_add_pt(omniBox, v_add(posAndRad, v_splat_w(posAndRad)));
      v_bbox3_add_pt(omniBox, v_sub(posAndRad, v_splat_w(posAndRad)));
      outRenderOmniLights[i] = l;
      uint16_t shadowId = omniLights.getShadowId(cVisibleOmniLightsId[i]);
      if (shadowId != INVALID_SHADOW_VOLUME_ID)
        commonShadowData[baseIndex + i] = lightShadows->getOctahedralVolumeTexData(shadowId);
      else
        memset(&commonShadowData[baseIndex + i], 0, sizeof(Point4));
    }
    outOfFrustumOmniLightsCB.update(outRenderOmniLights.data(), data_size(outRenderOmniLights));
  }
  else
  {
    outOfFrustumOmniLightsCB.update(nullptr, 0);
  }

  const bool hasLights = !cVisibleSpotLightsId.empty() || !cVisibleOmniLightsId.empty();
  if (hasLights)
  {
    outOfFrustumCommonLightsShadowsCB.update(commonShadowData.data(), data_size(commonShadowData));
    commonLightsShadowsAreEmpty = false;
  }
  else
  {
    if (commonLightsShadowsAreEmpty == false)
      outOfFrustumCommonLightsShadowsCB.update(nullptr, 0);
    commonLightsShadowsAreEmpty = true;
  }
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
  buffersFilled = false;
  Frustum frustum(globtm);
  plane3f clusteredLastPlane = shrink_zfar_plane(frustum.camPlanes[4], cur_view_pos, v_splats(maxClusteredDist));

  OSSpinlockScopedLock scopedLock{lightLock};

  lightsPartition.prepareClusteredAndFarOmniLightBuffersCPU(frustum, occlusion, clusteredLastPlane, MARK_SMALL_LIGHT_AS_FAR_LIMIT,
    cur_view_pos, omni_light_require_any_mask, light_cutoff_dist_sq);

  lightsPartition.prepareClusteredAndFarSpotLightBuffersCPU(frustum, occlusion, clusteredLastPlane, MARK_SMALL_LIGHT_AS_FAR_LIMIT,
    cur_view_pos, spot_light_require_any_mask, light_cutoff_dist_sq);

  const Tab<vec4f> &visibleOmniLightsBounds = lightsPartition.getVisibleClusteredOmniLightsBounds();
  const Tab<vec4f> &visibleSpotLightsBounds = lightsPartition.getVisibleClusteredSpotLightsBounds();

  if (lightsGrid.isGPU())
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
  if (buffersFilled)
    return;
  buffersFilled = true;
  const uint32_t omniWords = lightsGrid.getOmniWords();
  const uint32_t spotWords = lightsGrid.getSpotWords();
  const bool hasLights = lightsGrid.newFrameHasLights();
  if (hasLights || lightsGrid.lastFrameHasLights()) // todo: only update if something changed (which won't happen very often)
  {
    G_ASSERT(omniWords == dag::divide_align_up(lightsGrid.getOmniCount(), 32));
    G_ASSERT(spotWords == dag::divide_align_up(lightsGrid.getSpotCount(), 32));

    ShaderGlobal::set_float(depthSliceScaleVarId, clusters.depthSliceScale);
    ShaderGlobal::set_float(depthSliceBiasVarId, clusters.depthSliceBias);
    ShaderGlobal::set_float4(shadowAtlasTexelVarId, Color4(lightShadows ? 1.f / lightShadows->getAtlasWidth() : 1,
                                                      lightShadows ? 1.f / lightShadows->getAtlasHeight() : 1, 0.f, 0.f));
    const float maxShadowDistUse = min(maxShadowDist, maxClusteredDist * 0.9f);
    const float shadowScale = 1 / (maxShadowDistUse * 0.95 - maxShadowDistUse); // last 5% of distance are used for disappearing of
                                                                                // shadows
    const float shadowBias = -shadowScale * maxShadowDistUse;
    ShaderGlobal::set_float(shadowDistScaleVarId, shadowScale);
    ShaderGlobal::set_float(shadowDistBiasVarId, shadowBias);
    ShaderGlobal::set_float(shadowZBiasVarId, shaderShadowZBias);
    ShaderGlobal::set_float(shadowSlopeZBiasVarId, shaderShadowSlopeZBias);
  }
  lightsGrid.advanceFrameState();

  lightsPartition.updateBuffersForVisibleClusteredLights(lightsGrid.getOmniCount(), lightsGrid.getSpotCount());
  lightsPartition.updateBuffersForVisibleFarLights();

  const auto &visibleFarSpotLightsCB = lightsPartition.getVisibleFarSpotLightsCB();
  const auto &visibleFarOmniLightsCB = lightsPartition.getVisibleFarOmniLightsCB();

  const auto &visibleClusteredSpotLightsCB = lightsPartition.getVisibleClusteredSpotLightsCB();
  const auto &visibleClusteredOmniLightsCB = lightsPartition.getVisibleClusteredOmniLightsCB();

  {
    d3d::resource_barrier({visibleClusteredSpotLightsCB.getBuf(), RB_RO_COPY_SOURCE});
    d3d::resource_barrier({visibleClusteredOmniLightsCB.getBuf(), RB_RO_COPY_SOURCE});
    d3d::resource_barrier({visibleFarSpotLightsCB.getBuf(), RB_RO_COPY_SOURCE});
    d3d::resource_barrier({visibleFarOmniLightsCB.getBuf(), RB_RO_COPY_SOURCE});

    lightsRenderer.copyInstanceCountsToIndirectArgs(
      LightsRenderer::OmniLightsCBs{.far = &visibleFarOmniLightsCB, .clustered = &visibleClusteredOmniLightsCB},
      LightsRenderer::SpotLightsCBs{.far = &visibleFarSpotLightsCB, .clustered = &visibleClusteredSpotLightsCB});

    d3d::resource_barrier(
      {visibleClusteredSpotLightsCB.getBuf(), RB_RO_CONSTANT_BUFFER | RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    d3d::resource_barrier(
      {visibleClusteredOmniLightsCB.getBuf(), RB_RO_CONSTANT_BUFFER | RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    d3d::resource_barrier(
      {visibleFarSpotLightsCB.getBuf(), RB_RO_CONSTANT_BUFFER | RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    d3d::resource_barrier(
      {visibleFarOmniLightsCB.getBuf(), RB_RO_CONSTANT_BUFFER | RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
  }


  ShaderGlobal::set_buffer(omni_lightsVarId, visibleClusteredOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, visibleClusteredSpotLightsCB.getId());

  if (lightsPartition.getVisibleClusteredOmniLightsMasksSB())
    ShaderGlobal::set_buffer(omni_lights_flagsVarId, lightsPartition.getVisibleClusteredOmniLightsMasksSB());
  if (lightsPartition.getVisibleClusteredSpotLightsMasksSB())
    ShaderGlobal::set_buffer(spot_lights_flagsVarId, lightsPartition.getVisibleClusteredSpotLightsMasksSB());

  // GPU path: both omni_lights_cb and spot_lights_cb are now uploaded; dispatch
  // the compute shader that clears and fills lights_full_grid directly.
  if (hasLights)
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
  dynamicLightsShadowsVolumeSet.reset();
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
    tiledLights = eastl::make_unique<TiledLights>(maxClusteredDist);
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
  else
  {
    // We should default dynamic_light_shadows sampler to a comparison one in order to prevent D3D11 ERROR.
    ShaderGlobal::set_sampler(::get_shader_variable_id("dynamic_light_shadows_samplerstate"),
      d3d::request_sampler({.filter_mode = d3d::FilterMode::Compare}));
  }

  omniLights.setShadowSystem(lightShadows.get());
  spotLights.setShadowSystem(lightShadows.get());

  initClustered(frame_initial_lights_count);
  lightsPartition.init(false); // now gpu partition is not integrated: so cpu-path is selected always
  lightsRenderer.init();

  if (lightShadows)
    dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights, &lightsResMgr);

  omni_lightsVarId = ::get_shader_variable_id("omni_lights", false);
  spot_lightsVarId = ::get_shader_variable_id("spot_lights", false);
  common_lights_shadowsVarId = ::get_shader_variable_id("common_lights_shadows", false);
  tiledLights.reset();
  if (use_tiled_lights)
    tiledLights = eastl::make_unique<TiledLights>(maxClusteredDist);
}

void ClusteredLights::setMaxClusteredDist(const float max_clustered_dist)
{
  maxClusteredDist = max_clustered_dist;
  if (tiledLights)
    tiledLights->setMaxLightsDist(maxClusteredDist);
}

void ClusteredLights::renderDebugOmniLights()
{
  if (hasClusteredOmniLights() || hasDeferredOmniLights())
  {
    G_ASSERT(buffersFilled);
    lightsRenderer.renderDebugOmniLights(LightsRenderer::OmniLightsCBs{
      .far = &lightsPartition.getVisibleFarOmniLightsCB(), .clustered = &lightsPartition.getVisibleClusteredOmniLightsCB()});
  }
}

void ClusteredLights::renderDebugSpotLights()
{
  if (hasClusteredSpotLights() || hasDeferredSpotLights())
  {
    G_ASSERT(buffersFilled);
    lightsRenderer.renderDebugSpotLights(LightsRenderer::SpotLightsCBs{
      .far = &lightsPartition.getVisibleFarSpotLightsCB(), .clustered = &lightsPartition.getVisibleClusteredSpotLightsCB()});
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
    dynamicLightsShadowsVolumeSet.reset(omniLights.getShadowId(typeId.id));
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
    dynamicLightsShadowsVolumeSet.reset(omniLights.getShadowId(typeId.id));
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

void ClusteredLights::setLightNoLock(uint32_t id, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow)
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), return,
    "(%s) light %d is invalid (maxIndex= %d)", typeId.type == LightType::Spot ? "spot" : "omni", typeId.id, spotLights.maxIndex());

  if (invalidate_shadow && spotLights.tryInvalidateShadowsIfNeed(typeId.id, light))
  {
    dynamicLightsShadowsVolumeSet.reset(spotLights.getShadowId(typeId.id));
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
  return lightsPartition.isLightVisible(id);
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
      dynamicLightsShadowsVolumeSet.reset(shadowId);
    }
    break;
    case LightType::Omni:
    {
      const auto shadowId =
        omniLights.allocateShadowVolume(typeId.id, casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
      if (shadowId == INVALID_SHADOW_VOLUME_ID)
        return false;
      dynamicLightsShadowsVolumeSet.reset(shadowId);
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

dynamic_shadow_render::QualityParams ClusteredLights::getQualityParams() const
{
  dynamic_shadow_render::QualityParams result;
  result.maxShadowsToUpdateOnFrame = maxShadowsToUpdateOnFrame;
  result.maxShadowDist = maxShadowDist;
  return result;
}

void ClusteredLights::framePrepareShadows(dynamic_shadow_render::VolumesVector &volumesToRender, const Point3 &viewPos,
  mat44f_cref globtm, float hk, dag::ConstSpan<bbox3f> dynamicBoxes, dynamic_shadow_render::FrameUpdates *frameUpdates)
{
  const Tab<uint16_t> &visibleSpotLightsId = lightsPartition.getVisibleClusteredSpotLightsIds();
  const Tab<uint16_t> &visibleOmniLightsId = lightsPartition.getVisibleClusteredOmniLightsIds();
  if ((visibleSpotLightsId.empty() && visibleOmniLightsId.empty()) || !lightShadows)
    return;
  TIME_D3D_PROFILE(spotAndOmniShadows);

  OSSpinlockScopedLock scopedLock{lightLock};
  lightShadows->startPrepareShadows();

  for (auto spotId : visibleSpotLightsId)
  {
    uint32_t shadowId = spotLights.getShadowId(spotId);
    if (shadowId != INVALID_SHADOW_VOLUME_ID)
    {
      setSpotLightShadowVolume(spotId);
      if (spotLights.isShadowClose(spotId, viewPos, maxShadowDist))
        lightShadows->useShadowOnFrame(shadowId);
    }
  }

  for (auto omniId : visibleOmniLightsId)
  {
    const auto shadowId = omniLights.getShadowId(omniId);
    if (shadowId != INVALID_SHADOW_VOLUME_ID)
    {
      setOmniLightShadowVolume(omniId);
      if (omniLights.isShadowClose(omniId, viewPos, maxShadowDist))
        lightShadows->useShadowOnFrame(shadowId);
    }
  }

  lightShadows->setDynamicObjectsContent(dynamicBoxes.data(), dynamicBoxes.size()); // dynamic content within those boxes

  float maxAreaToUpdate = max((float)maxShadowsToUpdateOnFrame / DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME, 1.0f) * 0.25f;
  lightShadows->endPrepareShadows(volumesToRender, maxShadowsToUpdateOnFrame, maxShadowViewsToUpdateOnFrame, maxAreaToUpdate, viewPos,
    hk, globtm);

  if (frameUpdates)
  {
    for (int i = volumesToRender.size() - 1; i >= 0; --i)
    {
      const int id = volumesToRender[i];
      const auto renderFlags = lightShadows->getVolumeRenderFlags(id);

      if (renderFlags & ShadowSystem::RENDER_STATIC)
      {
        // Note: indexing must match frameRenderShadows!
        dynamic_shadow_render::FrameUpdate &result = frameUpdates->emplace_back();
        lightShadows->getVolumeUpdateData(id, result);
      }
    }
  }
}

void ClusteredLights::frameRenderShadows(const dag::ConstSpan<uint16_t> &volumesToRender,
  eastl::fixed_function<sizeof(void *) * 2, StaticRenderCallback> renderStatic,
  eastl::fixed_function<sizeof(void *) * 2, DynamicRenderCallback> renderDynamic)
{
  if ((lightsPartition.getVisibleClusteredSpotLightsIds().empty() && lightsPartition.getVisibleClusteredOmniLightsIds().empty()) ||
      !lightShadows)
    return;

  SCOPE_VIEW_PROJ_MATRIX;
  SCOPE_RENDER_TARGET;
  OSSpinlockUniqueLock scopedLock{lightLock};
  if (!volumesToRender.empty())
  {
    // debug("render %d / %d", lightShadows->getShadowVolumesToRender().size(), visibleSpotLightsId.size());
    lightShadows->startRenderVolumes(volumesToRender);
    int staticUpdateIndex = 0;
    bool staticOverrideState = false;
    shaders::OverrideStateId originalState = shaders::overrides::get_current();
    for (int i = volumesToRender.size() - 1; i >= 0; --i)
    {
      shaders::overrides::set(depthBiasOverrideId);
      mat44f view, proj, viewItm;
      const int id = volumesToRender[i];
      ShadowSystem::RenderFlags renderFlags;
      uint32_t numViews = lightShadows->startRenderVolume(id, proj, renderFlags);
      if (renderFlags & ShadowSystem::RENDER_STATIC)
      {
        TIME_D3D_PROFILE(staticShadow);
        if (!staticOverrideState)
        {
          shaders::overrides::reset();
          shaders::overrides::set(depthBiasOverrideId);
          staticOverrideState = true;
        }

        for (uint32_t viewId = 0; viewId < numViews; ++viewId)
        {
          lightShadows->startRenderVolumeView(id, viewId, viewItm, view, renderFlags, ShadowSystem::RENDER_STATIC);
          alignas(16) TMatrix viewItmS;
          v_mat_43ca_from_mat44(viewItmS[0], viewItm);

          d3d::settm(TM_VIEW, view);
          d3d::settm(TM_PROJ, proj);
          mat44f globTm;
          v_mat44_mul(globTm, proj, view);

          bool hint_dynamic;
          ShadowCastersFlag casters;
          uint8_t priority, shadow_size_srl;
          uint16_t quality;
          DynamicShadowRenderGPUObjects render_gpu_objects;
          lightShadows->getShadowProperties(id, casters, hint_dynamic, quality, priority, shadow_size_srl, render_gpu_objects);

          // Note: indexing must match frameUpdateShadows!
          renderStatic(globTm, proj, viewItmS, staticUpdateIndex, viewId, render_gpu_objects);
          lightShadows->endRenderVolumeView(id, viewId);
        }
        ++staticUpdateIndex;
        lightShadows->endRenderStatic(id);
      }
      if (renderFlags & ShadowSystem::RENDER_DYNAMIC)
      {
        TIME_D3D_PROFILE(dynamicShadow);
        staticOverrideState = false;
        shaders::overrides::reset(); // startRenderDynamic uses an other state
        lightShadows->startRenderDynamic(id);
        shaders::overrides::set(lightShadows->isShadowTwoSided(id) ? depthBiasTwoSidedOverrideId : depthBiasOverrideId);
        for (uint32_t viewId = 0; viewId < numViews; ++viewId)
        {
          lightShadows->startRenderVolumeView(id, viewId, viewItm, view, renderFlags, ShadowSystem::RENDER_DYNAMIC);
          alignas(16) TMatrix viewItmS;
          v_mat_43ca_from_mat44(viewItmS[0], viewItm);

          d3d::settm(TM_VIEW, view);
          d3d::settm(TM_PROJ, proj);

          renderDynamic(viewItmS, view, proj);
          lightShadows->endRenderVolumeView(id, viewId);
        }
        shaders::overrides::reset();
      }
      lightShadows->endRenderVolume(id);
      shaders::overrides::reset();
    }
    shaders::overrides::reset();
    shaders::overrides::set(originalState);
    lightShadows->endRenderVolumes();
  }

  updateShadowBuffers();
  scopedLock.unlock();

  shaders::overrides::set(depthBiasOverrideId);
  dstReadbackLights->update(renderStatic);
  shaders::overrides::reset();
}

void ClusteredLights::updateShadowBuffers()
{
  const Tab<uint16_t> &visibleSpotLightsId = lightsPartition.getVisibleClusteredSpotLightsIds();
  const Tab<uint16_t> &visibleOmniLightsId = lightsPartition.getVisibleClusteredOmniLightsIds();
  // Per spot: 4 float4 tex matrix rows + 1 float4 atlas-UV bounds (rectMin.xy, rectMax.xy).
  StaticTab<Point4, 1 + MAX_CLUSTERED_SPOT_LIGHTS * 5 + MAX_CLUSTERED_OMNI_LIGHTS> commonLightShadowData;
  int numSpotShadows = min<int>(visibleSpotLightsId.size(), MAX_CLUSTERED_SPOT_LIGHTS);
  int numOmniShadows = min<int>(visibleOmniLightsId.size(), MAX_CLUSTERED_OMNI_LIGHTS);
  commonLightShadowData.resize(1 + numSpotShadows * 5 + numOmniShadows);
  commonLightShadowData[0] = Point4(numSpotShadows, numOmniShadows, 5 * numSpotShadows, 0);
  int baseIndex = 1;
  for (int i = 0; i < visibleSpotLightsId.size(); ++i)
  {
    uint16_t shadowId = lightShadows ? spotLights.getShadowId(visibleSpotLightsId[i]) : INVALID_SHADOW_VOLUME_ID;
    if (shadowId != INVALID_SHADOW_VOLUME_ID && lightShadows->hasVolumeEverBeenRendered(shadowId))
    {
      memcpy(&commonLightShadowData[baseIndex + i * 5], &lightShadows->getVolumeTexMatrix(shadowId), 4 * sizeof(Point4));
      commonLightShadowData[baseIndex + i * 5 + 4] = lightShadows->getShadowUvMinMax(shadowId);
    }
    else
    {
      memset(&commonLightShadowData[baseIndex + i * 5], 0, 4 * sizeof(Point4));
      commonLightShadowData[baseIndex + i * 5 + 4] = Point4(0, 0, 1, 1);
    }
  }
  baseIndex += visibleSpotLightsId.size() * 5;
  for (int i = 0; i < visibleOmniLightsId.size(); ++i)
  {
    uint16_t shadowId = lightShadows ? omniLights.getShadowId(visibleOmniLightsId[i]) : INVALID_SHADOW_VOLUME_ID;
    if (shadowId != INVALID_SHADOW_VOLUME_ID)
    {
      commonLightShadowData[baseIndex + i] = lightShadows->getOctahedralVolumeTexData(shadowId);
    }
    else
    {
      memset(&commonLightShadowData[baseIndex + i], 0, sizeof(Point4));
    }
  }

  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  commonLightShadowsBufferCB.reallocate(1 + visibleSpotLightsId.size() * 5 + numOmniShadows,
    1 + MAX_CLUSTERED_SPOT_LIGHTS * 5 + MAX_CLUSTERED_OMNI_LIGHTS, lightsResMgr.getResName("common_lights_shadows"),
    true /* persistent */);
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, commonLightShadowsBufferCB.getId());

  commonLightShadowsBufferCB.update(commonLightShadowData.data(), data_size(commonLightShadowData));

  if (spotLightSsssShadowDescBuffer && numSpotShadows > 0 && lightShadows)
  {
    StaticTab<SpotlightShadowDescriptor, MAX_CLUSTERED_SPOT_LIGHTS> spotLightSsssShadowDesc;
    spotLightSsssShadowDesc.resize(numSpotShadows);
    for (int i = 0; i < visibleSpotLightsId.size(); ++i)
    {
      uint16_t shadowId = spotLights.getShadowId(visibleSpotLightsId[i]);
      if (shadowId != INVALID_SHADOW_VOLUME_ID)
      {
        SpotlightShadowDescriptor &shadowDesc = spotLightSsssShadowDesc[i];

        float wk;
        Point2 zn_zfar;
        lightShadows->getVolumeInfo(shadowId, wk, zn_zfar.x, zn_zfar.y);
        shadowDesc.decodeDepth = get_decode_depth(zn_zfar);

        Point2 shadowUvSize = lightShadows->getShadowUvSize(shadowId);
        shadowDesc.meterToUvAtZfar = max(shadowUvSize.x, shadowUvSize.y) / (2 * wk);
        Point4 shadowUvMinMax = lightShadows->getShadowUvMinMax(shadowId);
        shadowDesc.uvMinMax = shadowUvMinMax;

        bool hintDynamic;
        ShadowCastersFlag casters;
        uint16_t quality;
        uint8_t priority, size;
        DynamicShadowRenderGPUObjects renderGPUObjects;
        lightShadows->getShadowProperties(shadowId, casters, hintDynamic, quality, priority, size, renderGPUObjects);
        shadowDesc.hasDynamic = static_cast<float>((casters & ShadowCastersFlag::Dynamic) != ShadowCastersFlag::None);
      }
      else
      {
        spotLightSsssShadowDesc[i] = {};
      }
    }

    spotLightSsssShadowDescBuffer.getBuf()->updateData(0, numSpotShadows * sizeof(SpotlightShadowDescriptor),
      static_cast<const void *>(spotLightSsssShadowDesc.data()), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }
}


void ClusteredLights::setSpotLightShadowVolume(int spot_light_id)
{
  uint32_t shadowId = spotLights.getShadowId(spot_light_id);
  if (shadowId == INVALID_SHADOW_VOLUME_ID)
    return;
  if (dynamicLightsShadowsVolumeSet.test(shadowId))
    return;

  spotLights.updateShadowVolume(spot_light_id);
  dynamicLightsShadowsVolumeSet.set(shadowId);
}

void ClusteredLights::setOmniLightShadowVolume(int omni_light_id)
{
  uint32_t shadowId = omniLights.getShadowId(omni_light_id);
  if (shadowId == INVALID_SHADOW_VOLUME_ID)
    return;
  if (dynamicLightsShadowsVolumeSet.test(shadowId))
    return;

  omniLights.updateShadowVolume(omni_light_id);
  dynamicLightsShadowsVolumeSet.set(shadowId);
}

void ClusteredLights::setOutOfFrustumLightsToShader()
{
  G_ASSERT(lightsInitialized);
  ShaderGlobal::set_float4_array(out_of_frustum_omni_boxVarId, omniOOFBox, 2);
  ShaderGlobal::set_float4_array(out_of_frustum_spot_boxVarId, spotOOFBox, 2);
  ShaderGlobal::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, outOfFrustumCommonLightsShadowsCB.getId());
}

void ClusteredLights::setInsideOfFrustumLightsToShader() const
{
  G_ASSERT(lightsInitialized);
  ShaderGlobal::set_buffer(omni_lightsVarId, lightsPartition.getVisibleClusteredOmniLightsCB().getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, lightsPartition.getVisibleClusteredSpotLightsCB().getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, commonLightShadowsBufferCB.getId());
}

void ClusteredLights::beforeResetDevice()
{
  lightsRenderer.beforeResetDevice();
  if (tiledLights)
    tiledLights->beforeResetDevice();
}

void ClusteredLights::afterResetDevice()
{
  lightsRenderer.afterResetDevice();
  if (lightShadows)
  {
    OSSpinlockScopedLock scopedLock{lightLock};
    lightShadows->afterReset();
  }

  if (tiledLights)
    tiledLights->afterResetDevice();

  if (dstReadbackLights)
    dstReadbackLights->afterResetDevice();

  if (commonLightsShadowsAreEmpty)
    outOfFrustumCommonLightsShadowsCB.update(nullptr, 0);
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

void ClusteredLights::setNeedSsss(bool need_ssss)
{
  spotLightSsssShadowDescBuffer.close();
  if (need_ssss)
    spotLightSsssShadowDescBuffer = dag::create_sbuffer(sizeof(SpotlightShadowDescriptor), MAX_CLUSTERED_SPOT_LIGHTS,
      SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, "spot_lights_ssss_shadow_desc",
      RESTAG_LIGHTS);
}
