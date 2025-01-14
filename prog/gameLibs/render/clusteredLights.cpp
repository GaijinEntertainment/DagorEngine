// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <util/dag_stlqsort.h>
#include <util/dag_convar.h>
#include <render/primitiveObjects.h>
#include <math/dag_viewMatrix.h>
#include <debug/dag_debug3d.h>
#include <render/clusteredLights.h>
#include <render/shadowSystem.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_overrideStates.h>
#include <render/dstReadbackLights.h>
#include <math/dag_hlsl_floatx.h>
#include <render/renderLights.hlsli>
#include <render/depthUtil.h>
#include <3d/dag_lockSbuffer.h>
#include <EASTL/numeric_limits.h>
#include <ioSys/dag_dataBlock.h>

static constexpr int CLUSTERS_PER_GRID = CLUSTERS_W * CLUSTERS_H * (CLUSTERS_D + 1); // one more slice so we can sample zero for it,
                                                                                     // instead of branch in shader
static const uint32_t MAX_SHADOWS_QUALITY = 8u;

const float ClusteredLights::MARK_SMALL_LIGHT_AS_FAR_LIMIT = 0.03;

static int lights_full_gridVarId = -1;
static int omni_lightsVarId = -1;
static int spot_lightsVarId = -1;
static int common_lights_shadowsVarId = -1;

static int omniLightsCountVarId = -1, omniLightsWordCountVarId = -1;
static int spotLightsCountVarId = -1, spotLightsWordCountVarId = -1;
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

bool equalWithEps(const Point4 &a, const Point4 &b, float eps)
{
  const Point4 diff = abs(a - b);
  return (diff.x < eps) && (diff.y < eps) && (diff.z < eps) && (diff.w < eps);
}

bool isInvaliatingShadowsNeeded(const OmniLight &oldLight, const OmniLight &newLight)
{
  return !equalWithEps(oldLight.pos_radius, newLight.pos_radius, eastl::numeric_limits<float>::epsilon());
}

bool isInvaliatingShadowsNeeded(const SpotLight &oldLight, const SpotLight &newLight)
{
  return !equalWithEps(oldLight.pos_radius, newLight.pos_radius, eastl::numeric_limits<float>::epsilon()) ||
         !equalWithEps(oldLight.dir_tanHalfAngle, newLight.dir_tanHalfAngle, eastl::numeric_limits<float>::epsilon());
}

void ClusteredLights::validateDensity(uint32_t words)
{
  if (words <= allocatedWords)
    return;
  allocatedWords = words;
  for (int i = 0; i < lightsFullGridCB.size(); ++i)
  {
    String name(128, "lights_full_grid_%d%s", i, nameSuffix.c_str());
    lightsFullGridCB[i].close();
    lightsFullGridCB[i] = dag::create_sbuffer(sizeof(uint32_t), CLUSTERS_PER_GRID * allocatedWords,
      SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, name);
  }
}

void ClusteredLights::initClustered(int initial_light_density)
{
  uint32_t words = clamp((initial_light_density + 31) / 32, (int)2, (int)(MAX_SPOT_LIGHTS + MAX_OMNI_LIGHTS + 31) / 32);
  validateDensity(words);
  gridFrameHasLights = NOT_INITED;
  lightsGridFrame = 0;

  omniLightsCountVarId = get_shader_variable_id("omniLightsCount");
  omniLightsWordCountVarId = get_shader_variable_id("omniLightsWordCount");
  spotLightsCountVarId = get_shader_variable_id("spotLightsCount");
  spotLightsWordCountVarId = get_shader_variable_id("spotLightsWordCount");
  depthSliceScaleVarId = get_shader_variable_id("depthSliceScale");
  depthSliceBiasVarId = get_shader_variable_id("depthSliceBias");
  shadowAtlasTexelVarId = get_shader_variable_id("shadowAtlasTexel");
  shadowDistScaleVarId = get_shader_variable_id("shadowDistScale");
  shadowDistBiasVarId = get_shader_variable_id("shadowDistBias");
  shadowZBiasVarId = get_shader_variable_id("shadowZBias");
  shadowSlopeZBiasVarId = get_shader_variable_id("shadowSlopeZBias");
  spot_lights_flagsVarId = get_shader_variable_id("spot_lights_flags", true);
  omni_lights_flagsVarId = get_shader_variable_id("omni_lights_flags", true);

  // TODO: maybe use texture with R8 format instead of custom byte packing
  if (VariableMap::isVariablePresent(VariableMap::getVariableId("spot_lights_flags")))
  {
    int spotMaskSizeInBytes = (MAX_SPOT_LIGHTS + 3) / 4;
    visibleSpotLightsMasksSB =
      dag::buffers::create_one_frame_sr_structured(sizeof(uint), spotMaskSizeInBytes, getResName("spot_lights_flags"));
    ShaderGlobal::set_buffer(spot_lights_flagsVarId, visibleSpotLightsMasksSB);
  }

  if (VariableMap::isVariablePresent(VariableMap::getVariableId("omni_lights_flags")))
  {
    int omniMaskSizeInBytes = (MAX_OMNI_LIGHTS + 3) / 4;
    visibleOmniLightsMasksSB =
      dag::buffers::create_one_frame_sr_structured(sizeof(uint), omniMaskSizeInBytes, getResName("omni_lights_flags"));
    ShaderGlobal::set_buffer(omni_lights_flagsVarId, visibleOmniLightsMasksSB);
  }
}

ClusteredLights::ClusteredLights(const char *name_suffix) : nameSuffix(name_suffix)
{
  spotOOFBox[0] = omniOOFBox[0] = Point4(0, 0, 0, 0);
  spotOOFBox[1] = omniOOFBox[1] = Point4(OOF_GRID_W * 2, OOF_GRID_VERT * 2, OOF_GRID_W * 2, 0);
  mem_set_0(currentIndicesSize);
  if (VariableMap::isVariablePresent(get_shader_variable_id("oof_lights_full_grid", true)))
  {
#define CS(a) a.reset(new_compute_shader(#a))
    CS(cull_out_of_frustum_lights_cs);
    CS(clear_out_of_frustum_grid_cs);
#undef CS
  }
}

ClusteredLights::~ClusteredLights() { close(); }

void ClusteredLights::close()
{
  lightsInitialized = false;
  dstReadbackLights.reset();
  closeOmniShadows();
  closeSpotShadows();
  lightShadows.reset();
  allocatedWords = 0;

  spotLightSsssShadowDescBuffer.close();
  visibleSpotLightsMasksSB.close();
  visibleOmniLightsMasksSB.close();

  coneSphereVb.close();
  coneSphereIb.close();

  closeOmni();
  closeSpot();
  closeDebugOmni();
  closeDebugSpot();
  shaders::overrides::destroy(depthBiasOverrideId);
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
    lightShadows->setOverrideState(depthBiasOverrideState);

  shaderShadowZBias = shader_z_bias;
  shaderShadowSlopeZBias = shader_slope_z_bias;
}

void ClusteredLights::getShadowBias(float &z_bias, float &slope_z_bias, float &shader_z_bias, float &shader_slope_z_bias) const
{
  z_bias = depthBiasOverrideState.zBias;
  slope_z_bias = depthBiasOverrideState.slopeZBias;
  shader_z_bias = shaderShadowZBias;
  shader_slope_z_bias = shaderShadowSlopeZBias;
}

void ClusteredLights::renderOtherLights() // render regular deferred way (currently - with no shadows)
{
  if (!hasDeferredLights())
    return;
  TIME_D3D_PROFILE(deferred_lights);
  setBuffers();
  renderPrims(pointLightsElem, omni_lightsVarId, visibleFarOmniLightsCB.getId(), renderFarOmniLights.size(), 0, v_count, 0, f_count);
  renderPrims(spotLightsElem, spot_lightsVarId, visibleFarSpotLightsCB.getId(), renderFarSpotLights.size(), v_count, 5, f_count * 3,
    6);
  resetBuffers();
}

void ClusteredLights::setEmptyOutOfFrustumLights()
{
  G_ASSERT(lightsInitialized);
  static Point4 c[2] = {Point4(0, 0, 0, 0), Point4(2 * OOF_GRID_W, 2 * OOF_GRID_VERT, 2 * OOF_GRID_W, 0)};
  ShaderGlobal::set_color4_array(out_of_frustum_omni_boxVarId, c, 2);
  ShaderGlobal::set_color4_array(out_of_frustum_spot_boxVarId, c, 2);
  outOfFrustumVisibleSpotLightsCB.reallocate(0, MAX_SPOT_LIGHTS, getResName("out_of_frustum_spot_lights"));
  outOfFrustumOmniLightsCB.reallocate(0, MAX_OMNI_LIGHTS, getResName("out_of_frustum_omni_lights"));

  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  outOfFrustumCommonLightsShadowsCB.reallocate(1, 1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS,
    getResName("out_of_frustum_common_lights_shadow_data"), true /*persistent*/);

  outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
  outOfFrustumOmniLightsCB.update(nullptr, 0);
  outOfFrustumCommonLightsShadowsCB.update(nullptr, 0);
  ShaderGlobal::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, outOfFrustumCommonLightsShadowsCB.getId());
}

bool ClusteredLights::cullOutOfFrustumLights(mat44f_cref globtm, SpotLightsManager::mask_type_t spot_light_mask,
  OmniLightsManager::mask_type_t omni_light_mask)
{
  G_ASSERT(lightsInitialized);
  Frustum frustum(globtm);
  bbox3f far_box, near_box;
  vec4f unreachablePlane = v_make_vec4f(0, 0, 0, MAX_REAL);

  Tab<uint16_t> visibleFarOmniLightsId(framemem_ptr()), cVisibleOmniLightsId(framemem_ptr());
  omniLights.prepare(frustum, visibleFarOmniLightsId, cVisibleOmniLightsId, nullptr, far_box, near_box, unreachablePlane,
    dynamicOmniLightsShadows, 0, v_zero(), omni_light_mask);
  G_ASSERT(visibleFarOmniLightsId.size() == 0);
  cVisibleOmniLightsId.resize(min<int>(cVisibleOmniLightsId.size(), MAX_OMNI_LIGHTS));


  Tab<uint16_t> visibleFarSpotLightsId(framemem_ptr()), cVisibleSpotLightsId(framemem_ptr());
  spotLights.prepare(frustum, visibleFarSpotLightsId, cVisibleSpotLightsId, nullptr, nullptr, far_box, near_box, unreachablePlane,
    dynamicSpotLightsShadows, spot_light_mask);
  G_ASSERT(visibleFarSpotLightsId.size() == 0);
  cVisibleSpotLightsId.resize(min<int>(cVisibleSpotLightsId.size(), MAX_SPOT_LIGHTS));
  DA_PROFILE_TAG(outOfFrustumLights, "spots %d omnis %d", (int)cVisibleOmniLightsId.size(), (int)cVisibleSpotLightsId.size());

  const uint32_t spotWords = (cVisibleSpotLightsId.size() + 31) / 32, omniWords = (cVisibleOmniLightsId.size() + 31) / 32;


  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  outOfFrustumCommonLightsShadowsCB.reallocate(1 + cVisibleSpotLightsId.size() * 4 + cVisibleOmniLightsId.size(),
    1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS, getResName("out_of_frustum_common_lights_shadow_data"), true /*persistent*/);

  StaticTab<Point4, 1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS> commonShadowData;
  commonShadowData.resize(1 + cVisibleSpotLightsId.size() * 4 + cVisibleOmniLightsId.size());
  commonShadowData[0] = Point4(cVisibleSpotLightsId.size(), cVisibleOmniLightsId.size(), 4 * cVisibleSpotLightsId.size(), 0);

  outOfFrustumVisibleSpotLightsCB.reallocate(cVisibleSpotLightsId.size(), MAX_SPOT_LIGHTS, getResName("out_of_frustum_spot_lights"));
  int baseIndex = 1;
  bbox3f spotBox;
  v_bbox3_init_empty(spotBox);
  if (cVisibleSpotLightsId.size())
  {
    Tab<RenderSpotLight> outRenderSpotLights(framemem_ptr());
    outRenderSpotLights.resize(cVisibleSpotLightsId.size());
    for (int i = 0, ie = cVisibleSpotLightsId.size(); i < ie; ++i)
    {
      v_bbox3_add_box(spotBox, spotLights.getBoundingBox(cVisibleSpotLightsId[i]));
      outRenderSpotLights[i] = spotLights.getRenderLight(cVisibleSpotLightsId[i]);
    }
    outOfFrustumVisibleSpotLightsCB.update(outRenderSpotLights.data(), data_size(outRenderSpotLights));
    for (int i = 0, ie = cVisibleSpotLightsId.size(); i < ie; ++i)
    {
      uint16_t shadowId = dynamicSpotLightsShadows[cVisibleSpotLightsId[i]];
      if (shadowId != INVALID_VOLUME)
        memcpy(&commonShadowData[baseIndex + i * 4], &lightShadows->getVolumeTexMatrix(shadowId), 4 * sizeof(Point4));
      else
        memset(&commonShadowData[baseIndex + i * 4], 0, 4 * sizeof(Point4));
    }
  }
  else
  {
    outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
  }

  outOfFrustumOmniLightsCB.reallocate(cVisibleOmniLightsId.size(), MAX_OMNI_LIGHTS, getResName("out_of_frustum_omni_lights"));
  baseIndex += cVisibleSpotLightsId.size() * 4;
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
      uint16_t shadowId = dynamicOmniLightsShadows[cVisibleOmniLightsId[i]];
      if (shadowId != INVALID_VOLUME)
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
  }
  else
  {
    outOfFrustumCommonLightsShadowsCB.update(nullptr, 0);
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
    if (!outOfFrustumLightsFullGridCB || outOfFrustumLightsFullGridCB.getBuf()->ressize() < sz4 * 4)
    {
      outOfFrustumLightsFullGridCB.close();
      outOfFrustumLightsFullGridCB =
        dag::create_sbuffer(sizeof(uint32_t), sz4, SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW, 0, "oof_lights_full_grid");
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
      ShaderGlobal::set_color4_array(out_of_frustum_grid_cull_boxVarId, cb, 4);
      cull_out_of_frustum_lights_cs->dispatchThreads(OOF_GRID_W, OOF_GRID_W, OOF_GRID_VERT * totalLights);
    }
    d3d::resource_barrier({outOfFrustumLightsFullGridCB.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  }

  return hasLights;
}

void ClusteredLights::cullFrustumLights(vec4f cur_view_pos, mat44f_cref globtm, mat44f_cref view, mat44f_cref proj, float znear,
  Occlusion *occlusion, SpotLightsManager::mask_type_t spot_light_mask, OmniLightsManager::mask_type_t omni_light_mask)
{
  TIME_PROFILE(cullFrustumLights);
  buffersFilled = false;
  Frustum frustum(globtm);
  plane3f clusteredLastPlane = shrink_zfar_plane(frustum.camPlanes[4], cur_view_pos, v_splats(maxClusteredDist));

  // separate to closer than maxClusteredDist and farther to render others deferred way
  bbox3f far_box, near_box;
  G_ASSERT(sizeof(RenderOmniLight) == sizeof(OmniLightsManager::RawLight));

  visibleOmniLightsIdSet.reset();
  visibleOmniLightsId.clear();
  Tab<uint16_t> visibleFarOmniLightsId(framemem_ptr());
  omniLights.prepare(frustum, visibleFarOmniLightsId, visibleOmniLightsId, &visibleOmniLightsIdSet, occlusion, far_box, near_box,
    clusteredLastPlane, dynamicOmniLightsShadows, MARK_SMALL_LIGHT_AS_FAR_LIMIT, cur_view_pos, omni_light_mask);

  if (visibleOmniLightsId.size() > MAX_OMNI_LIGHTS)
  {
    // Spotlights were always sorted, this is only here to move the farthests ones into the far buffer.
    stlsort::sort(visibleOmniLightsId.begin(), visibleOmniLightsId.end(), [this, &cur_view_pos](uint16_t i, uint16_t j) {
      const vec3f distI = v_length3_sq(v_sub(cur_view_pos, omniLights.getBoundingSphere(i)));
      const vec3f distJ = v_length3_sq(v_sub(cur_view_pos, omniLights.getBoundingSphere(j)));
      return v_test_vec_x_lt_0(v_sub(distI, distJ));
    });
    auto oldFarSize = visibleFarOmniLightsId.size();
    auto excessSize = visibleOmniLightsId.size() - MAX_OMNI_LIGHTS;
    append_items(visibleFarOmniLightsId, excessSize, visibleOmniLightsId.begin() + MAX_OMNI_LIGHTS);
    static bool logwarnOnce;
    if (!logwarnOnce)
    {
      logwarn("too many omni lights %d, moved %d to Far buffer (before %d, after %d) at %f, %f, %f", visibleOmniLightsId.size(),
        excessSize, oldFarSize, visibleFarOmniLightsId.size(), v_extract_x(cur_view_pos), v_extract_y(cur_view_pos),
        v_extract_z(cur_view_pos));
      logwarnOnce = true;
    }
    G_UNUSED(oldFarSize);
  }
  visibleOmniLightsId.resize(min(int(visibleOmniLightsId.size()), int(MAX_OMNI_LIGHTS)));

  visibleSpotLightsIdSet.reset();
  visibleSpotLightsId.clear();
  // spotLights.prepare(frustum, visibleSpotLightsId, occlusion);

  Tab<uint16_t> visibleFarSpotLightsId(framemem_ptr());
  spotLights.prepare(frustum, visibleFarSpotLightsId, visibleSpotLightsId, &visibleSpotLightsIdSet, occlusion, far_box, near_box,
    clusteredLastPlane, dynamicSpotLightsShadows, MARK_SMALL_LIGHT_AS_FAR_LIMIT, cur_view_pos, spot_light_mask);


  stlsort::sort(visibleSpotLightsId.begin(), visibleSpotLightsId.end(), [this, &cur_view_pos](uint16_t i, uint16_t j) {
    const vec3f distI = v_length3_sq(v_sub(cur_view_pos, spotLights.getBoundingSphere(i)));
    const vec3f distJ = v_length3_sq(v_sub(cur_view_pos, spotLights.getBoundingSphere(j)));
    return v_test_vec_x_lt_0(v_sub(distI, distJ));
  });
  // separate close and far lights cb (so we can render more far lights easier)
  if (visibleSpotLightsId.size() > MAX_SPOT_LIGHTS)
  {
    auto oldFarSize = visibleFarSpotLightsId.size();
    auto excessSize = visibleSpotLightsId.size() - MAX_SPOT_LIGHTS;
    append_items(visibleFarSpotLightsId, excessSize, visibleSpotLightsId.begin() + MAX_SPOT_LIGHTS);
    static bool logwarnOnce;
    if (!logwarnOnce)
    {
      logwarn("too many spotlights %d, moved %d to Far buffer (before %d, after %d) at %f, %f, %f", visibleSpotLightsId.size(),
        excessSize, oldFarSize, visibleFarSpotLightsId.size(), v_extract_x(cur_view_pos), v_extract_y(cur_view_pos),
        v_extract_z(cur_view_pos));
      logwarnOnce = true;
    }
    G_UNUSED(oldFarSize);
  }
  visibleSpotLightsId.resize(min<int>(visibleSpotLightsId.size(), MAX_SPOT_LIGHTS));

  Tab<SpotLightsManager::RawLight> visibleSpotLights(framemem_ptr());
  Tab<vec4f> visibleSpotLightsBounds(framemem_ptr());
  Tab<vec4f> visibleOmniLightsBounds(framemem_ptr());
  visibleSpotLights.resize(visibleSpotLightsId.size());
  renderSpotLights.resize(visibleSpotLightsId.size());
  renderOmniLights.resize(visibleOmniLightsId.size());
  visibleSpotLightsBounds.resize(visibleSpotLightsId.size());
  visibleSpotLightsMasks.resize((visibleSpotLightsId.size() + 3) & ~3);
  visibleOmniLightsMasks.resize((visibleOmniLightsId.size() + 3) & ~3);
  visibleOmniLightsBounds.resize(renderOmniLights.size());
  for (int i = 0, e = visibleSpotLightsId.size(); i < e; ++i)
  {
    uint32_t id = visibleSpotLightsId[i];
    visibleSpotLightsBounds[i] = spotLights.getBoundingSphere(id);
    visibleSpotLights[i] = spotLights.getLight(id);
    renderSpotLights[i] = spotLights.getRenderLight(id);
    visibleSpotLightsMasks[i] = spotLights.getLightMask(id);
  }
  for (int i = visibleSpotLightsId.size(), e = (visibleSpotLightsId.size() + 3) & ~3; i < e; ++i)
    visibleSpotLightsMasks[i] = 0;
  for (int i = 0, e = visibleOmniLightsId.size(); i < e; ++i)
  {
    uint32_t id = visibleOmniLightsId[i];
    renderOmniLights[i] = omniLights.getRenderLight(id);
    visibleOmniLightsBounds[i] = v_ldu(reinterpret_cast<float *>(&renderOmniLights[i].posRadius));
    visibleOmniLightsMasks[i] = omniLights.getLightMask(id);
  }
  for (int i = visibleOmniLightsId.size(), e = (visibleOmniLightsId.size() + 3) & ~3; i < e; ++i)
    visibleOmniLightsMasks[i] = 0;

  visibleFarSpotLightsId.resize(min<int>(visibleFarSpotLightsId.size(), MAX_VISIBLE_FAR_LIGHTS));
  renderFarSpotLights.resize(visibleFarSpotLightsId.size());
  for (int i = 0, e = visibleFarSpotLightsId.size(); i < e; ++i)
    renderFarSpotLights[i] = spotLights.getRenderLight(visibleFarSpotLightsId[i]);

  visibleFarOmniLightsId.resize(min<int>(visibleFarOmniLightsId.size(), MAX_VISIBLE_FAR_LIGHTS));
  renderFarOmniLights.resize(visibleFarOmniLightsId.size());
  for (int i = 0, e = visibleFarOmniLightsId.size(); i < e; ++i)
    renderFarOmniLights[i] = omniLights.getRenderLight(visibleFarOmniLightsId[i]);

  // clusteredCullLights(view, proj, znear, 1, 500, (vec4f*)visibleOmniLights.data(),
  //   elem_size(visibleOmniLights)/sizeof(vec4f), visibleOmniLights.size(), 2);
  uint32_t omniWords = (renderOmniLights.size() + 31) / 32, spotWords = (visibleSpotLights.size() + 31) / 32;
  clustersOmniGrid.resize(CLUSTERS_PER_GRID * omniWords);
  clustersSpotGrid.resize(CLUSTERS_PER_GRID * spotWords);
  if (clustersOmniGrid.size() || clustersSpotGrid.size())
  {
    bool nextGridHasOmniLights = clustersOmniGrid.size() != 0, nextGridHasSpotLights = clustersSpotGrid.size() != 0;
    mem_set_0(clustersOmniGrid);
    mem_set_0(clustersSpotGrid);
    uint32_t *omniMask = clustersOmniGrid.data(), *spotMask = clustersSpotGrid.data();
    clusteredCullLights(view, proj, znear, closeSliceDist, maxClusteredDist, renderOmniLights, visibleSpotLights,
      visibleSpotLightsBounds, occlusion ? true : false, nextGridHasOmniLights, nextGridHasSpotLights, omniMask, omniWords, spotMask,
      spotWords);
    if (!nextGridHasOmniLights)
    {
      clustersOmniGrid.resize(0);
      renderOmniLights.resize(0);
      visibleOmniLightsBounds.resize(0);
    }
    if (!nextGridHasSpotLights)
    {
      clustersSpotGrid.resize(0);
      renderSpotLights.resize(0);
      visibleSpotLightsBounds.resize(0);
      // visibleSpotLightsMasks.resize(0);
    }
  }

  if (tiledLights)
  {
    mat44f invView;
    v_mat44_orthonormal_inverse43(invView, view);
    vec4f cur_view_dir = invView.col2;
    tiledLights->prepare(visibleOmniLightsBounds, visibleSpotLightsBounds, cur_view_pos, cur_view_dir);
  }
}

void ClusteredLights::fillBuffers()
{
  if (buffersFilled)
    return;
  buffersFilled = true;
  uint32_t omniWords = clustersOmniGrid.size() / CLUSTERS_PER_GRID, spotWords = clustersSpotGrid.size() / CLUSTERS_PER_GRID;
  if ((clustersOmniGrid.size() || clustersSpotGrid.size()) || gridFrameHasLights != NO_CLUSTERED_LIGHTS) // todo: only update if
                                                                                                         // something changed (which
                                                                                                         // won't happen very often)
  {
    G_ASSERT(omniWords == (renderOmniLights.size() + 31) / 32);
    G_ASSERT(spotWords == (renderSpotLights.size() + 31) / 32);
    ShaderGlobal::set_int(omniLightsCountVarId, renderOmniLights.size());
    ShaderGlobal::set_int(omniLightsWordCountVarId, omniWords);
    ShaderGlobal::set_int(spotLightsCountVarId, renderSpotLights.size());
    ShaderGlobal::set_int(spotLightsWordCountVarId, spotWords);
    ShaderGlobal::set_real(depthSliceScaleVarId, clusters.depthSliceScale);
    ShaderGlobal::set_real(depthSliceBiasVarId, clusters.depthSliceBias);
    ShaderGlobal::set_color4(shadowAtlasTexelVarId, Color4(lightShadows ? 1.f / lightShadows->getAtlasWidth() : 1,
                                                      lightShadows ? 1.f / lightShadows->getAtlasHeight() : 1, 0.f, 0.f));
    const float maxShadowDistUse = min(maxShadowDist, maxClusteredDist * 0.9f);
    const float shadowScale = 1 / (maxShadowDistUse * 0.95 - maxShadowDistUse); // last 5% of distance are used for disappearing of
                                                                                // shadows
    const float shadowBias = -shadowScale * maxShadowDistUse;
    ShaderGlobal::set_real(shadowDistScaleVarId, shadowScale);
    ShaderGlobal::set_real(shadowDistBiasVarId, shadowBias);
    ShaderGlobal::set_real(shadowZBiasVarId, shaderShadowZBias);
    ShaderGlobal::set_real(shadowSlopeZBiasVarId, shaderShadowSlopeZBias);
  }
  gridFrameHasLights = (clustersOmniGrid.size() || clustersSpotGrid.size()) ? HAS_CLUSTERED_LIGHTS : NO_CLUSTERED_LIGHTS;

  G_ASSERT(elem_size(renderOmniLights) % sizeof(vec4f) == 0);
  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  visibleOmniLightsCB.reallocate(renderOmniLights.size(), MAX_OMNI_LIGHTS, getResName("omni_lights"), true /* persistent */);
  visibleOmniLightsCB.update(renderOmniLights.data(), data_size(renderOmniLights));
  ShaderGlobal::set_buffer(omni_lightsVarId, visibleOmniLightsCB.getId());
  const SpotLightsManager::mask_type_t stubMask[1] = {0};
  if (visibleOmniLightsMasksSB) // todo: only update if something changed (which won't happen very often)
  {
    G_ASSERT(visibleOmniLightsMasks.size() <= ((MAX_OMNI_LIGHTS + 3) & ~3));
    dag::Span<const SpotLightsManager::mask_type_t> masks =
      renderOmniLights.size() ? make_span_const(visibleOmniLightsMasks) : make_span_const(stubMask);
    // bound & used framemem buffer must be updated every frame
    visibleOmniLightsMasksSB.getBuf()->updateDataWithLock(0, data_size(masks), masks.data(), VBLOCK_DISCARD);
    ShaderGlobal::set_buffer(omni_lights_flagsVarId, visibleOmniLightsMasksSB);
  }
  if (gridFrameHasLights == HAS_CLUSTERED_LIGHTS)
    fillClusteredCB(clustersOmniGrid.data(), omniWords, clustersSpotGrid.data(), spotWords);

  visibleSpotLightsCB.reallocate(renderSpotLights.size(), MAX_SPOT_LIGHTS, getResName("spot_lights"));
  visibleSpotLightsCB.update(renderSpotLights.data(), data_size(renderSpotLights));
  ShaderGlobal::set_buffer(spot_lightsVarId, visibleSpotLightsCB.getId());
  if (visibleSpotLightsMasksSB) // todo: only update if something changed (which won't happen very often)
  {
    // do that only when needed
    G_ASSERT(visibleSpotLightsMasks.size() <= ((MAX_SPOT_LIGHTS + 3) & ~3));
    dag::Span<const SpotLightsManager::mask_type_t> masks =
      renderSpotLights.size() ? make_span_const(visibleSpotLightsMasks) : make_span_const(stubMask);
    // bound & used framemem buffer must be updated every frame
    visibleSpotLightsMasksSB.getBuf()->updateDataWithLock(0, data_size(masks), masks.data(), VBLOCK_DISCARD);
    ShaderGlobal::set_buffer(spot_lights_flagsVarId, visibleSpotLightsMasksSB);
  }
  // todo: only update if something changed (which won't happen very often)
  visibleFarSpotLightsCB.reallocate(renderFarSpotLights.size(), MAX_VISIBLE_FAR_LIGHTS, getResName("far_spot_lights"));
  visibleFarSpotLightsCB.update(renderFarSpotLights.data(), data_size(renderFarSpotLights));
  visibleFarOmniLightsCB.reallocate(renderFarOmniLights.size(), MAX_VISIBLE_FAR_LIGHTS, getResName("far_omni_lights"));
  visibleFarOmniLightsCB.update(renderFarOmniLights.data(), data_size(renderFarOmniLights));

  if (tiledLights)
    tiledLights->fillBuffers();
}

void ClusteredLights::clusteredCullLights(mat44f_cref view, mat44f_cref proj, float znear, float minDist, float maxDist,
  dag::ConstSpan<RenderOmniLight> omni_lights, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights,
  dag::ConstSpan<vec4f> spot_light_bounds, bool use_occlusion, bool &has_omni_lights, bool &has_spot_lights, uint32_t *omni_mask,
  uint32_t omni_words, uint32_t *spot_mask, uint32_t spot_words)
{
  has_spot_lights = spot_lights.size() != 0;
  has_omni_lights = omni_lights.size() != 0;
  if (!omni_lights.size() && !spot_lights.size())
    return;
  TIME_D3D_PROFILE(clusteredFill);
  clusters.prepareFrustum(view, proj, znear, minDist, maxDist, use_occlusion);
  eastl::unique_ptr<FrustumClusters::ClusterGridItemMasks, framememDeleter> tempOmniItemsPtr, tempSpotItemsPtr;

  tempOmniItemsPtr.reset(new (framemem_ptr()) FrustumClusters::ClusterGridItemMasks);

  TIME_PROFILE(clustered);
  uint32_t clusteredOmniLights = clusters.fillItemsSpheres((const vec4f *)omni_lights.data(), elem_size(omni_lights) / sizeof(vec4f),
    omni_lights.size(), *tempOmniItemsPtr, omni_mask, omni_words);

  tempOmniItemsPtr.reset();

  uint32_t clusteredSpotLights = 0;
  if (spot_lights.size())
  {
    tempSpotItemsPtr.reset(new (framemem_ptr()) FrustumClusters::ClusterGridItemMasks);
    clusteredSpotLights = clusters.fillItemsSpheres(spot_light_bounds.data(), elem_size(spot_light_bounds) / sizeof(vec4f),
      spot_lights.size(), *tempSpotItemsPtr, spot_mask, spot_words);
  }

  if (clusteredSpotLights)
  {
    TIME_PROFILE(cullSpots)
    clusteredSpotLights = clusters.cullSpots((const vec4f *)spot_lights.data(), elem_size(spot_lights) / sizeof(vec4f),
      (const vec4f *)&spot_lights[0].dir_tanHalfAngle, elem_size(spot_lights) / sizeof(vec4f), *tempSpotItemsPtr, spot_mask,
      spot_words);
  }
  has_spot_lights = clusteredSpotLights != 0;
  has_omni_lights = clusteredOmniLights != 0;
  if (!clusteredSpotLights && !clusteredOmniLights)
    return;
}

bool ClusteredLights::fillClusteredCB(uint32_t *source_omni, uint32_t omni_words, uint32_t *source_spot, uint32_t spot_words)
{
  validateDensity(spot_words + omni_words); // ensure there is enough space with size

  lightsGridFrame = (lightsGridFrame + 1) % lightsFullGridCB.size();
  ShaderGlobal::set_buffer(lights_full_gridVarId, lightsFullGridCB[lightsGridFrame]);

  LockedBuffer<uint32_t> masks =
    lock_sbuffer<uint32_t>(lightsFullGridCB[lightsGridFrame].getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  if (!masks)
    return false;
  masks.updateDataRange(0, source_omni, omni_words * CLUSTERS_PER_GRID);
  masks.updateDataRange(CLUSTERS_PER_GRID * omni_words, source_spot, spot_words * CLUSTERS_PER_GRID);
  return true;
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
  const Driver3dDesc &d = d3d::get_driver_desc();
  const uint32_t res = min<uint32_t>(1024 * shadow_quality, min(d.maxtexw, d.maxtexh));
  const uint32_t qMul = clamp<uint32_t>(res / 1024, 1, MAX_SHADOWS_QUALITY);

  lightShadows->changeResolution(res, 256 * qMul, 64 * qMul, 64 * qMul, dynamic_shadow_32bit);
}

void ClusteredLights::resetShadows()
{
  dynamicLightsShadowsVolumeSet.reset();
  closeOmniShadows();
  closeSpotShadows();
  lightShadows.reset();
}

void ClusteredLights::changeShadowResolution(uint32_t shadow_quality, bool dynamic_shadow_32bit)
{
  if (!lightShadows && shadow_quality > 0)
  {
    dstReadbackLights.reset();
    lightShadows.reset();
    lightShadows = eastl::make_unique<ShadowSystem>(nameSuffix.c_str());
    lightShadows->setOverrideState(depthBiasOverrideState);
    dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights, nameSuffix.c_str());
  }

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

void ClusteredLights::init(int frame_initial_lights_count, uint32_t shadow_quality, bool use_tiled_lights, const char *name_suffix)
{
  nameSuffix = name_suffix;
  lightsInitialized = true;
  if (shadow_quality)
  {
    lightShadows = eastl::make_unique<ShadowSystem>(name_suffix);
    lightShadows->setOverrideState(depthBiasOverrideState);
    changeShadowResolutionByQuality(shadow_quality, false);
  }
  initClustered(frame_initial_lights_count);
  initConeSphere();
  initSpot();
  initOmni();
  initDebugOmni();
  initDebugSpot();

  visibleOmniLightsCB.reallocate(0, MAX_OMNI_LIGHTS, getResName("omni_lights"), true /* persistent */);
  visibleOmniLightsCB.update(nullptr, 0);
  visibleSpotLightsCB.reallocate(0, MAX_SPOT_LIGHTS, getResName("spot_lights"), true /* persistent */);
  visibleSpotLightsCB.update(nullptr, 0);

  if (lightShadows)
    dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights, nameSuffix.c_str());

  lights_full_gridVarId = ::get_shader_variable_id("lights_full_grid", true);
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

void ClusteredLights::closeOmni()
{
  visibleOmniLightsCB.close();
  visibleFarOmniLightsCB.close();
  pointLightsElem = NULL;
  del_it(pointLightsMat);
}

void ClusteredLights::closeOmniShadows()
{
  if (!lightShadows)
    return;
  for (uint16_t &shadowIdx : dynamicOmniLightsShadows)
  {
    if (shadowIdx != INVALID_VOLUME)
    {
      lightShadows->destroyVolume(shadowIdx);
      shadowIdx = INVALID_VOLUME;
    }
  }
}

void ClusteredLights::closeSpotShadows()
{
  if (!lightShadows)
    return;
  for (uint16_t &shadowIdx : dynamicSpotLightsShadows)
  {
    if (shadowIdx != INVALID_VOLUME)
    {
      lightShadows->destroyVolume(shadowIdx);
      shadowIdx = INVALID_VOLUME;
    }
  }
}

void ClusteredLights::closeSpot()
{
  visibleSpotLightsCB.close();
  visibleFarSpotLightsCB.close();
  commonLightShadowsBufferCB.close();
  spotLightsElem = NULL;
  del_it(spotLightsMat);
}

void ClusteredLights::initConeSphere()
{
  static constexpr uint32_t SLICES = 5;
  calc_sphere_vertex_face_count(SLICES, SLICES, false, v_count, f_count);
  coneSphereVb.close();
  coneSphereVb = dag::create_vb((v_count + 5) * sizeof(Point3), 0, getResName("coneSphereVb"));
  d3d_err((bool)coneSphereVb);
  coneSphereIb.close();
  coneSphereIb = dag::create_ib((f_count + 6) * 6, 0, getResName("coneSphereIb"));
  d3d_err((bool)coneSphereIb);

  LockedBuffer<uint16_t> indicesLocked = lock_sbuffer<uint16_t>(coneSphereIb.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  if (!indicesLocked)
    return;
  uint16_t *indices = indicesLocked.get();
  LockedBuffer<Point3> verticesLocked = lock_sbuffer<Point3>(coneSphereVb.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  if (!verticesLocked)
    return;
  Point3 *vertices = verticesLocked.get();

  create_sphere_mesh(dag::Span<uint8_t>((uint8_t *)vertices, v_count * sizeof(Point3)),
    dag::Span<uint8_t>((uint8_t *)indices, f_count * 6), 1.0f, SLICES, SLICES, sizeof(Point3), false, false, false, false);
  vertices += v_count;
  vertices[0] = Point3(0, 0, 0);
  vertices[1] = Point3(-1, -1, 1);
  vertices[2] = Point3(+1, -1, 1);
  vertices[3] = Point3(-1, +1, 1);
  vertices[4] = Point3(+1, +1, 1);

  indices += f_count * 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 2;
  indices[2] = v_count + 1;
  indices += 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 3;
  indices[2] = v_count + 4;
  indices += 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 1;
  indices[2] = v_count + 3;
  indices += 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 4;
  indices[2] = v_count + 2;
  indices += 3;
  indices[0] = v_count + 1;
  indices[1] = v_count + 2;
  indices[2] = v_count + 3;
  indices += 3;
  indices[0] = v_count + 3;
  indices[1] = v_count + 2;
  indices[2] = v_count + 4;
}

void ClusteredLights::initOmni()
{
  closeOmni();
  pointLightsMat = new_shader_material_by_name("point_lights");
  G_ASSERT(pointLightsMat);
  pointLightsMat->addRef();
  pointLightsElem = pointLightsMat->make_elem();
}

void ClusteredLights::initSpot()
{
  closeSpot();
  spotLightsMat = new_shader_material_by_name("spot_lights");
  G_ASSERT(spotLightsMat);
  spotLightsMat->addRef();
  spotLightsElem = spotLightsMat->make_elem();
}

void ClusteredLights::closeDebugSpot()
{
  spotLightsDebugElem = NULL;
  del_it(spotLightsDebugMat);
}

void ClusteredLights::closeDebugOmni()
{
  pointLightsDebugElem = NULL;
  del_it(pointLightsDebugMat);
}

void ClusteredLights::initDebugOmni()
{
  closeDebugOmni();
  pointLightsDebugMat = new_shader_material_by_name_optional("debug_lights");
  if (!pointLightsDebugMat)
    return;
  pointLightsDebugMat->addRef();
  pointLightsDebugElem = pointLightsDebugMat->make_elem();
}

void ClusteredLights::initDebugSpot()
{
  closeDebugSpot();
  spotLightsDebugMat = new_shader_material_by_name_optional("debug_spot_lights");
  if (!spotLightsDebugMat)
    return;
  spotLightsDebugMat->addRef();
  spotLightsDebugElem = spotLightsDebugMat->make_elem();
}

void ClusteredLights::setBuffers()
{
  fillBuffers();
  d3d::setind(coneSphereIb.getBuf());
  d3d::setvsrc(0, coneSphereVb.getBuf(), sizeof(Point3));
}

void ClusteredLights::resetBuffers() {}

void ClusteredLights::renderPrims(ShaderElement *elem, int buffer_var_id, D3DRESID replaced_buffer, int inst_count, int, int,
  int index_start, int fcount)
{
  if (!inst_count)
    return;
  fillBuffers();
  D3DRESID old_buffer = ShaderGlobal::get_buf(buffer_var_id);
  ShaderGlobal::set_buffer(buffer_var_id, replaced_buffer);
  elem->setStates(0, true);
  d3d::drawind_instanced(PRIM_TRILIST, index_start, fcount, 0, inst_count);
  ShaderGlobal::set_buffer(buffer_var_id, old_buffer);
}

void ClusteredLights::renderDebugOmniLights()
{
  if (!pointLightsDebugElem)
    return;
  if (getVisibleOmniCount() == 0)
    return;
  TIME_D3D_PROFILE(renderDebugOmniLights);

  // debug("rawLightsIn.size() = %d rawLightsOut.size() = %d", rawLightsIn.size(),rawLightsOut.size());
  setBuffers();
  renderPrims(pointLightsDebugElem, omni_lightsVarId, visibleOmniLightsCB.getId(), getVisibleClusteredOmniCount(), 0, v_count, 0,
    f_count);
  renderPrims(pointLightsDebugElem, omni_lightsVarId, visibleFarOmniLightsCB.getId(), getVisibleNotClusteredOmniCount(), 0, v_count, 0,
    f_count);
  resetBuffers();
}

void ClusteredLights::renderDebugSpotLights()
{
  if (!spotLightsDebugElem)
    return;
  if (getVisibleSpotsCount() == 0)
    return;
  TIME_D3D_PROFILE(renderDebugSpotLights);

  // debug("rawLightsIn.size() = %d rawLightsOut.size() = %d", rawLightsIn.size(),rawLightsOut.size());
  setBuffers();
  renderPrims(spotLightsDebugElem, spot_lightsVarId, visibleSpotLightsCB.getId(), getVisibleClusteredSpotsCount(), v_count, 5,
    f_count * 3, 6);
  renderPrims(spotLightsDebugElem, spot_lightsVarId, visibleFarSpotLightsCB.getId(), getVisibleNotClusteredSpotsCount(), v_count, 5,
    f_count * 3, 6);
  resetBuffers();

#if 0
  begin_draw_cached_debug_lines(false, false);
  for (int i = 0; i < visibleSpotLightsId.size(); ++i)
  {
    vec4f sphere = spotLights.getBoundingSphere(visibleSpotLightsId[i]);
    draw_cached_debug_sphere(Point3::xyz((Point4&)sphere), ((Point4&)sphere).w, 0xFFFFFF1F);
  }
  end_draw_cached_debug_lines();
#endif
}

void ClusteredLights::renderDebugLights()
{
  renderDebugSpotLights();
  renderDebugOmniLights();
}

void ClusteredLights::renderDebugLightsBboxes()
{
  spotLights.renderDebugBboxes();
  omniLights.renderDebugBboxes();
}

void ClusteredLights::destroyLight(uint32_t id)
{
  DecodedLightId typeId = decode_light_id(id);
  switch (typeId.type)
  {
    case LightType::Spot: spotLights.destroyLight(typeId.id); break;
    case LightType::Omni: omniLights.destroyLight(typeId.id); break;
    case LightType::Invalid: return;
    default: G_ASSERT_FAIL("unknown light type");
  }

  uint16_t &lightShadow = typeId.type == LightType::Spot ? dynamicSpotLightsShadows[typeId.id] : dynamicOmniLightsShadows[typeId.id];

  if (lightShadows && lightShadow != INVALID_VOLUME)
    lightShadows->destroyVolume(lightShadow);
  lightShadow = INVALID_VOLUME;
}

uint32_t ClusteredLights::addOmniLight(const OmniLight &light, OmniLightsManager::mask_type_t mask)
{
  int id = omniLights.addLight(0, light);
  if (id < 0)
    return INVALID_LIGHT;
  omniLights.setLightMask(id, mask);
  if (dynamicOmniLightsShadows.size() <= id)
  {
    int start = append_items(dynamicOmniLightsShadows, id - dynamicOmniLightsShadows.size() + 1);
    memset(dynamicOmniLightsShadows.data() + start, 0xFF,
      (dynamicOmniLightsShadows.size() - start) * elem_size(dynamicOmniLightsShadows));
  }
  return id;
}

// keep mask
void ClusteredLights::setLight(uint32_t id, const OmniLight &light, bool invalidate_shadow)
{
  DecodedLightId typeId = decode_light_id(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return,
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());
  if (lightShadows != nullptr && dynamicOmniLightsShadows[typeId.id] != INVALID_VOLUME)
  {
    uint32_t shadowId = dynamicOmniLightsShadows[typeId.id];
    invalidate_shadow &= isInvaliatingShadowsNeeded(omniLights.getLight(typeId.id), light);
    if (invalidate_shadow && lightShadows)
    {
      dynamicLightsShadowsVolumeSet.reset(shadowId);
      lightShadows->invalidateVolumeShadow(shadowId);
    }
  }
  omniLights.setLight(typeId.id, light);
}
void ClusteredLights::setLightWithMask(uint32_t id, const OmniLight &light, OmniLightsManager::mask_type_t mask,
  bool invalidate_shadow)
{
  DecodedLightId typeId = decode_light_id(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return,
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());
  if (lightShadows != nullptr && dynamicOmniLightsShadows[typeId.id] != INVALID_VOLUME)
  {
    uint32_t shadowId = dynamicOmniLightsShadows[typeId.id];
    invalidate_shadow &= isInvaliatingShadowsNeeded(omniLights.getLight(typeId.id), light);
    if (invalidate_shadow && lightShadows)
    {
      dynamicLightsShadowsVolumeSet.reset(shadowId);
      lightShadows->invalidateVolumeShadow(shadowId);
    }
  }
  omniLights.setLight(typeId.id, light);
  omniLights.setLightMask(typeId.id, mask);
}

ClusteredLights::OmniLight ClusteredLights::getOmniLight(uint32_t id) const
{
  DecodedLightId typeId = decode_light_id(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return OmniLight(),
    "omni light %d is invalid (maxIndex= %d)", typeId.id, omniLights.maxIndex());
  return omniLights.getLight(id);
}

void ClusteredLights::setLight(uint32_t id, const SpotLight &light, SpotLightsManager::mask_type_t mask, bool invalidate_shadow)
{
  DecodedLightId typeId = decode_light_id(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), return,
    "(%s) light %d is invalid (maxIndex= %d)", typeId.type == LightType::Spot ? "spot" : "omni", typeId.id, spotLights.maxIndex());
  if (lightShadows != nullptr && dynamicSpotLightsShadows[typeId.id] != INVALID_VOLUME)
  {
    uint32_t shadowId = dynamicSpotLightsShadows[typeId.id];
    invalidate_shadow &= isInvaliatingShadowsNeeded(spotLights.getLight(typeId.id), light);
    if (invalidate_shadow && lightShadows)
    {
      dynamicLightsShadowsVolumeSet.reset(shadowId);
      lightShadows->invalidateVolumeShadow(shadowId);
    }
  }
  spotLights.setLight(typeId.id, light);
  spotLights.setLightMask(typeId.id, mask);
}

ClusteredLights::SpotLight ClusteredLights::getSpotLight(uint32_t id) const
{
  DecodedLightId typeId = decode_light_id(id);
  G_ASSERTF_AND_DO(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), return SpotLight(),
    "(%s) light %d is invalid (maxIndex= %d)", typeId.type == LightType::Spot ? "spot" : "omni", id, spotLights.maxIndex());
  return spotLights.getLight(typeId.id);
}

bool ClusteredLights::isLightVisible(uint32_t id) const
{
  DecodedLightId typeId = decode_light_id(id);
  switch (typeId.type)
  {
    case LightType::Spot: G_ASSERT_RETURN(typeId.id <= spotLights.maxIndex(), false); return visibleSpotLightsIdSet.test(typeId.id);
    case LightType::Omni: G_ASSERT_RETURN(typeId.id <= omniLights.maxIndex(), false); return visibleOmniLightsIdSet.test(typeId.id);
    case LightType::Invalid: return false;
    default: G_ASSERT_FAIL("unknown light type");
  }
  return false;
}

uint32_t ClusteredLights::addSpotLight(const SpotLight &light, SpotLightsManager::mask_type_t mask)
{
  int id = spotLights.addLight(light);
  if (id < 0)
    return INVALID_LIGHT;
  spotLights.setLightMask(id, mask);
  if (dynamicSpotLightsShadows.size() <= id)
  {
    int start = append_items(dynamicSpotLightsShadows, id - dynamicSpotLightsShadows.size() + 1);
    memset(dynamicSpotLightsShadows.data() + start, 0xFF,
      (dynamicSpotLightsShadows.size() - start) * elem_size(dynamicSpotLightsShadows));
  }
  return id | SPOT_LIGHT_FLAG;
}

bool ClusteredLights::addShadowToLight(uint32_t id, ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
  uint8_t max_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
{
  DecodedLightId typeId = decode_light_id(id);
  switch (typeId.type)
  {
    case LightType::Invalid: return false;
    case LightType::Spot: spotLights.setLightShadows(typeId.id, true); break;
    default: break;
  }

  if (!lightShadows)
    return false;

  switch (typeId.type)
  {
    case LightType::Spot:
    {
      G_ASSERTF_RETURN(dynamicSpotLightsShadows[typeId.id] == INVALID_VOLUME, false, "spot light %d already has shadow", typeId.id);
      int shadowId = lightShadows->allocateVolume(casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
      if (shadowId < 0)
        return false;
      dynamicSpotLightsShadows[typeId.id] = shadowId;
      dynamicLightsShadowsVolumeSet.reset(shadowId);
    }
    break;
    case LightType::Omni:
    {
      G_ASSERTF_RETURN(dynamicOmniLightsShadows[typeId.id] == INVALID_VOLUME, false, "omni light %d already has shadow", typeId.id);
      int shadowId = lightShadows->allocateVolume(casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
      if (shadowId < 0)
        return false;
      dynamicOmniLightsShadows[typeId.id] = shadowId;
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

  DecodedLightId typeId = decode_light_id(id);
  if (typeId.type == LightType::Invalid)
    return false;

  const uint16_t &lightShadow =
    typeId.type == LightType::Spot ? dynamicSpotLightsShadows[typeId.id] : dynamicOmniLightsShadows[typeId.id];
  if (lightShadow == INVALID_VOLUME)
    return false;

  return lightShadows->getShadowProperties(lightShadow, casters, hint_dynamic, quality, priority, shadow_size_srl, render_gpu_objects);
}

void ClusteredLights::removeShadow(uint32_t id)
{
  DecodedLightId typeId = decode_light_id(id);
  switch (typeId.type)
  {
    case LightType::Invalid: return;
    case LightType::Spot: spotLights.setLightShadows(typeId.id, false); break;
    default: break;
  }

  if (!lightShadows)
    return;

  uint16_t &lightShadow = typeId.type == LightType::Spot ? dynamicSpotLightsShadows[typeId.id] : dynamicOmniLightsShadows[typeId.id];
  if (lightShadow == INVALID_VOLUME)
  {
    G_ASSERTF(0, "light %d has no shadow", id);
    return;
  }
  lightShadows->destroyVolume(lightShadow);
  lightShadow = INVALID_VOLUME;
}

void ClusteredLights::invalidateAllShadows()
{
  if (lightShadows)
    lightShadows->invalidateAllVolumes();
}
void ClusteredLights::invalidateStaticObjects(bbox3f_cref box)
{
  if (lightShadows)
    lightShadows->invalidateStaticObjects(box);
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
  if ((visibleSpotLightsId.empty() && visibleOmniLightsId.empty()) || !lightShadows)
    return;
  vec4f vposMaxShadow = v_make_vec4f(viewPos.x, viewPos.y, viewPos.z, -maxShadowDist);
  vec4f mulFactor = v_make_vec4f(1, 1, 1, -1);
  TIME_D3D_PROFILE(spotAndOmniShadows);
  SCOPE_VIEW_PROJ_MATRIX;
  SCOPE_RENDER_TARGET;

  lightShadows->startPrepareShadows();

  for (auto spotId : visibleSpotLightsId)
  {
    uint32_t shadowId = dynamicSpotLightsShadows[spotId];
    if (shadowId != INVALID_VOLUME)
    {
      setSpotLightShadowVolume(spotId);
      // only if distance to light is closer than max shadow see distance we need update shadow
      vec4f bounding = spotLights.getBoundingSphere(spotId);
      bounding = v_sub(bounding, vposMaxShadow);
      bounding = v_mul(bounding, bounding);
      bounding = v_dot4_x(bounding, mulFactor);
      if (v_test_vec_x_lt_0(bounding))
        lightShadows->useShadowOnFrame(shadowId);
    }
  }

  for (auto omniId : visibleOmniLightsId)
  {
    uint32_t shadowId = dynamicOmniLightsShadows[omniId];
    if (shadowId != INVALID_VOLUME)
    {
      setOmniLightShadowVolume(omniId);
      // only if distance to light is closer than max shadow see distance we need update shadow
      vec4f bounding = omniLights.getBoundingSphere(omniId);
      bounding = v_sub(bounding, vposMaxShadow);
      bounding = v_mul(bounding, bounding);
      bounding = v_dot4_x(bounding, mulFactor);
      if (v_test_vec_x_lt_0(bounding))
        lightShadows->useShadowOnFrame(shadowId);
    }
  }

  lightShadows->setDynamicObjectsContent(dynamicBoxes.data(), dynamicBoxes.size()); // dynamic content within those boxes

  float maxAreaToUpdate = max((float)maxShadowsToUpdateOnFrame / DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME, 1.0f) * 0.25f;
  lightShadows->endPrepareShadows(volumesToRender, maxShadowsToUpdateOnFrame, maxAreaToUpdate, viewPos, hk, globtm);

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
  if ((visibleSpotLightsId.empty() && visibleOmniLightsId.empty()) || !lightShadows)
    return;
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
          renderStatic(globTm, viewItmS, staticUpdateIndex, viewId, render_gpu_objects);
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
        shaders::overrides::set(depthBiasOverrideId);
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

  shaders::overrides::set(depthBiasOverrideId);
  dstReadbackLights->update(renderStatic);
  shaders::overrides::reset();
}

void ClusteredLights::updateShadowBuffers()
{
  StaticTab<Point4, 1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS> commonLightShadowData;
  int numSpotShadows = min<int>(visibleSpotLightsId.size(), MAX_SPOT_LIGHTS);
  int numOmniShadows = min<int>(visibleOmniLightsId.size(), MAX_OMNI_LIGHTS);
  commonLightShadowData.resize(1 + numSpotShadows * 4 + numOmniShadows);
  commonLightShadowData[0] = Point4(numSpotShadows, numOmniShadows, 4 * numSpotShadows, 0);
  int baseIndex = 1;
  for (int i = 0; i < visibleSpotLightsId.size(); ++i)
  {
    uint16_t shadowId = dynamicSpotLightsShadows[visibleSpotLightsId[i]];
    if (shadowId != INVALID_VOLUME)
    {
      memcpy(&commonLightShadowData[baseIndex + i * 4], &lightShadows->getVolumeTexMatrix(shadowId), 4 * sizeof(Point4));
    }
    else
    {
      memset(&commonLightShadowData[baseIndex + i * 4], 0, 4 * sizeof(Point4));
    }
  }
  baseIndex += visibleSpotLightsId.size() * 4;
  for (int i = 0; i < visibleOmniLightsId.size(); ++i)
  {
    uint16_t shadowId = dynamicOmniLightsShadows[visibleOmniLightsId[i]];
    if (shadowId != INVALID_VOLUME)
    {
      commonLightShadowData[baseIndex + i] = lightShadows->getOctahedralVolumeTexData(shadowId);
    }
    else
    {
      memset(&commonLightShadowData[baseIndex + i], 0, sizeof(Point4));
    }
  }

  if (numSpotShadows > 0 || numOmniShadows > 0)
  {
    // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
    commonLightShadowsBufferCB.reallocate(1 + visibleSpotLightsId.size() * 4 + numOmniShadows,
      1 + MAX_SPOT_LIGHTS * 4 + MAX_SPOT_LIGHTS, getResName("common_lights_shadows"), true /* persistent */);
    ShaderGlobal::set_buffer(common_lights_shadowsVarId, commonLightShadowsBufferCB.getId());

    commonLightShadowsBufferCB.update(commonLightShadowData.data(), data_size(commonLightShadowData));
  }

  if (spotLightSsssShadowDescBuffer && numSpotShadows > 0)
  {
    StaticTab<SpotlightShadowDescriptor, MAX_SPOT_LIGHTS> spotLightSsssShadowDesc;
    spotLightSsssShadowDesc.resize(numSpotShadows);
    for (int i = 0; i < visibleSpotLightsId.size(); ++i)
    {
      uint16_t shadowId = dynamicSpotLightsShadows[visibleSpotLightsId[i]];
      if (shadowId != INVALID_VOLUME)
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
        shadowDesc.hasDynamic = static_cast<float>(bool(casters & DYNAMIC_CASTERS));
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
  uint32_t shadowId = dynamicSpotLightsShadows[spot_light_id];
  if (dynamicLightsShadowsVolumeSet.test(shadowId))
    return;
  const SpotLightsManager::RawLight &l = spotLights.getLight(spot_light_id);
  mat44f viewITM;
  spotLights.getLightView(spot_light_id, viewITM);

  bbox3f box;
  v_bbox3_init_empty(box);
  float2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.w);
  lightShadows->setShadowVolume(shadowId, viewITM, lightZnZfar.x, lightZnZfar.y, 1. / l.dir_tanHalfAngle.w, box);
  dynamicLightsShadowsVolumeSet.set(shadowId);
}

void ClusteredLights::setOmniLightShadowVolume(int omni_light_id)
{
  uint32_t shadowId = dynamicOmniLightsShadows[omni_light_id];
  if (dynamicLightsShadowsVolumeSet.test(shadowId))
    return;
  const OmniLightsManager::RawLight &l = omniLights.getLight(omni_light_id);

  bbox3f box;
  v_bbox3_init_empty(box);
  float2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.w);
  vec3f vpos = v_make_vec4f(l.pos_radius.x, l.pos_radius.y, l.pos_radius.z, 0);
  lightShadows->setOctahedralShadowVolume(shadowId, vpos, lightZnZfar.x, lightZnZfar.y, box);
  dynamicLightsShadowsVolumeSet.set(shadowId);
}

void ClusteredLights::setOutOfFrustumLightsToShader()
{
  G_ASSERT(lightsInitialized);
  ShaderGlobal::set_color4_array(out_of_frustum_omni_boxVarId, omniOOFBox, 2);
  ShaderGlobal::set_color4_array(out_of_frustum_spot_boxVarId, spotOOFBox, 2);
  ShaderGlobal::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, outOfFrustumCommonLightsShadowsCB.getId());
}

void ClusteredLights::setInsideOfFrustumLightsToShader()
{
  G_ASSERT(lightsInitialized);
  if (!buffersFilled)
    return;
  ShaderGlobal::set_buffer(omni_lightsVarId, visibleOmniLightsCB.getId());
  ShaderGlobal::set_buffer(spot_lightsVarId, visibleSpotLightsCB.getId());
  ShaderGlobal::set_buffer(common_lights_shadowsVarId, commonLightShadowsBufferCB.getId());
}

void ClusteredLights::setBuffersToShader() { fillBuffers(); }

/*static void draw_cached_debug_wired_box(Point3 p[8], E3DCOLOR color)
{
  for (int z = 0; z <= 4; z+=4)
  {
    draw_cached_debug_line ( p[z+0], p[z+1], color );
    draw_cached_debug_line ( p[z+1], p[z+3], color );
    draw_cached_debug_line ( p[z+3], p[z+2], color );
    draw_cached_debug_line ( p[z+2], p[z+0], color );
  }

  draw_cached_debug_line ( p[0], p[4+0], color );
  draw_cached_debug_line ( p[1], p[4+1], color );
  draw_cached_debug_line ( p[2], p[4+2], color );
  draw_cached_debug_line ( p[3], p[4+3], color );
}*/

void ClusteredLights::drawDebugClusters(int slice)
{
  G_UNREFERENCED(slice);
  /*begin_draw_cached_debug_lines(false, false);
  TMatrix fView;
  v_mat_43cu_from_mat44(fView[0], clusters.view);
  set_cached_debug_lines_wtm(inverse(fView));
  int minZ = slice ? slice-1 : 0;
  int maxZ = slice ? slice : CLUSTERS_D-1;
  for (int z = minZ; z < maxZ; ++z)
    for (int y = 0; y < CLUSTERS_H; ++y)
      for (int x = 0; x < CLUSTERS_W; ++x)
      {
        Point3 p[8];
        for (int cp = 0, k = 0; k<2; ++k)
          for (int j = 0; j<2; ++j)
            for (int i = 0; i<2; ++i, cp++)
              p[cp] = (clusters.frustumPoints.data())[(x+i)+(y+j)*(CLUSTERS_W+1)+(z+k)*(CLUSTERS_W+1)*(CLUSTERS_H+1)];
        E3DCOLOR c = tempSpotItemsPtr->gridCount[x+y*CLUSTERS_W+z*CLUSTERS_W*CLUSTERS_H]>0 ? 0xffff00FF : 0x7f7f7F7F;
        draw_cached_debug_wired_box(p, c);
      }
  set_cached_debug_lines_wtm(TMatrix::IDENT);
  end_draw_cached_debug_lines();*/
  /*begin_draw_cached_debug_lines(false, false);
  for (int i = 0; i < clusters.frustumPoints.size(); ++i)
    draw_cached_debug_sphere(Point3::xyz(clusters.frustumPoints[i]), 0.02, 0xFFFFFFFF);
  end_draw_cached_debug_lines();*/

  /*begin_draw_cached_debug_lines(true, false);
  for (int i = 0; i < visibleSpotLightsBounds.size(); ++i)
  {
    const Point4 &sph = *(Point4*)&visibleSpotLightsBounds[i];
    draw_cached_debug_sphere(Point3::xyz(sph), sph.w,0xFFFFFFFF);
  }
  end_draw_cached_debug_lines();*/
}


bool ClusteredLights::reallocate_common(UniqueBuf &buf, uint16_t &size, int target_size, const char *stat_name, bool persistent)
{
  if (size >= target_size)
    return true;
  BufPtr cb2 = persistent ? dag::buffers::create_persistent_cb(target_size, stat_name)
                          : dag::buffers::create_one_frame_cb(target_size, stat_name);
  if (!cb2)
  {
    logerr("can't re-create buffer <%s> for size %d from %d", stat_name, target_size, size);
    return false;
  }
  size = target_size;
  buf = eastl::move(cb2);
  return true;
}

bool ClusteredLights::updateConsts(Sbuffer *buf, void *data, int data_size, int elems_count)
{
  uint32_t *destData = 0;
  bool ret = buf->lock(0, 0, (void **)&destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  d3d_err(ret);
  if (!ret || !destData)
    return false;
  if (elems_count >= 0)
  {
    destData[0] = elems_count;
    destData[1] = (elems_count + 31) / 32;
    destData[2] = elems_count ? elems_count - 1 : 0;
    destData[3] = 0;
    destData += 4;
  }
  if (data_size)
    memcpy(destData, data, data_size);
  buf->unlock();
  return true;
}

void ClusteredLights::afterReset()
{
  initConeSphere();
  if (lightShadows)
    lightShadows->afterReset();
}

bbox3f ClusteredLights::getActiveShadowVolume() const
{
  if (!lightShadows)
  {
    bbox3f ret;
    v_bbox3_init_empty(ret);
    return ret;
  }
  return lightShadows->getActiveShadowVolume();
}

void ClusteredLights::setNeedSsss(bool need_ssss)
{
  spotLightSsssShadowDescBuffer.close();
  if (need_ssss)
    spotLightSsssShadowDescBuffer = dag::create_sbuffer(sizeof(SpotlightShadowDescriptor), MAX_SPOT_LIGHTS,
      SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, "spot_lights_ssss_shadow_desc");
}
