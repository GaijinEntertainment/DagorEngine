// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <util/dag_stlqsort.h>
#include "shadowDepthScroller.h"
#include "staticShadowsViewMatrices.h"
#include <render/toroidalStaticShadows.h>
#include <EASTL/fixed_vector.h>
#include <shaders/dag_shaders.h>
#include <vecmath/dag_vecMath.h>

ToroidalStaticShadows::~ToroidalStaticShadows() = default;

void ToroidalStaticShadows::invalidate(bool force)
{
  for (auto &s : cascades)
    s.invalidate(force);
  if (force)
    clearTexture();
}

void ToroidalStaticShadows::invalidateBox(const BBox3 &box)
{
  for (auto &s : cascades)
    s.invalidateBox(box);
}

void ToroidalStaticShadows::invalidateBoxes(const BBox3 *box, uint32_t cnt)
{
  for (auto &s : cascades)
    s.invalidateBoxes(box, cnt);
}

void ToroidalStaticShadows::enableTransitionCascade(int cascade_id, bool en)
{
  G_ASSERT_RETURN(cascade_id >= 0 && cascade_id < cascades.size(), );
  cascades[cascade_id].enableTransitionCascade(en);
}

void ToroidalStaticShadows::enableOptimizeBigQuadsRender(int cascade_id, bool en)
{
  G_ASSERT_RETURN(cascade_id >= 0 && cascade_id < cascades.size(), );
  cascades[cascade_id].optimizeBigQuadsRender = en;
}

void ToroidalStaticShadows::enableStopRenderingAfterRegionSplit(int cascade_id, bool en)
{
  G_ASSERT_RETURN(cascade_id >= 0 && cascade_id < cascades.size(), );
  cascades[cascade_id].stopRenderingAfterRegionSplit = en;
}

bool ToroidalStaticShadows::setSunDir(const Point3 &dir_to_sun, float cos_force_threshold, float cos_lazy_threshold)
{
  bool isSunUnderHorizon = dir_to_sun.y < 0.0f;
  isOrtho = dir_to_sun.y < ORTHOGONAL_THRESHOLD;
  lightViewProj = isSunUnderHorizon
                    ? under_horizon_tm()
                    : (isOrtho ? light_dir_ortho_tm(dir_to_sun) : calc_skewed_light_tm(dir_to_sun, useTextureSpaceAlignment));
  bool ret = false;
  for (auto &s : cascades)
    ret |= s.setSunDir(lightViewProj, dir_to_sun, isOrtho, cos_force_threshold, cos_lazy_threshold);
  return ret;
}

void ToroidalStaticShadows::setMaxHtRange(float max_ht_range)
{
  // fixme: should causes rescale!
  if (maxHtRange == max_ht_range)
    return;
  maxHtRange = max_ht_range;
  for (auto &c : cascades)
    c.setMaxHtRange(maxHtRange);
}

static int static_shadows_cascades = -1;

ToroidalStaticShadows::ToroidalStaticShadows(int tsz, int cnt, float dist, float ht_range, bool is_array)
{
  isRendered = false;
  texSize = tsz;
  maxHtRange = ht_range;
  maxDistance = dist;
  if (cnt == 0)
    is_array = false;

  // tex type used in shader is bound to hardware.fsh_5_0 and must be matched with tex type created
  if (getMaxFSHVersion() >= 5.0_sm)
    is_array = true;

  cnt = is_array ? clamp(cnt, 1, 4) : 1;
  cascades.resize(cnt);
  for (int i = 0; i < cnt; ++i)
  {
    float cDist = maxDistance * (i == cnt - 1 ? 1.f : (i + 1.f) / (cnt + 1));
    cascades[i].init(i, texSize, cDist, maxHtRange);
  }
  const uint32_t fmt =
    d3d::get_driver_desc().issues.hasRenderPassClearDataRace ? TEXCF_RTARGET | TEXFMT_DEPTH32_S8 : TEXCF_RTARGET | TEXFMT_DEPTH16;
  TexPtr tex = is_array ? dag::create_array_tex(texSize, texSize, cnt, fmt, 1, "static_shadow_tex_arr")
                        : dag::create_tex(nullptr, texSize, texSize, fmt, 1, "static_shadow_tex2d");
  staticShadowTex = UniqueTexHolder(eastl::move(tex), "static_shadow_tex");
  restoreShadowSampler();
  clearTexture();
  static_shadows_cascades = get_shader_variable_id("static_shadows_cascades", true);
  // @NOTE: this check is enough as long as we never resize cascades anywhere but here
  if (ShaderGlobal::is_var_assumed(static_shadows_cascades))
  {
    int assumedVal = ShaderGlobal::get_interval_assumed_value(static_shadows_cascades);
    int desiredVal = cascadesCount();
    if (assumedVal != desiredVal)
      logerr("static_shadows_cascades was assumed to =%d, but using %d cascades", assumedVal, desiredVal);
  }
  setSunDir(Point3(0, 1, 0), 2, 2); // to force non-zero matrices in cascades
}

void ToroidalStaticShadows::restoreShadowSampler()
{
  d3d::SamplerInfo smpInfo;
  smpInfo.filter_mode = d3d::FilterMode::Compare;
  smpInfo.mip_map_mode = d3d::MipMapMode::Point;
  ShaderGlobal::set_sampler(get_shader_variable_id("static_shadow_tex_samplerstate", true), d3d::request_sampler(smpInfo));
}

ToroidalStaticShadowCascade::BeforeRenderReturned ToroidalStaticShadows::updateOriginAndRender(const Point3 &origin,
  float move_texels_threshold_part, float move_z_threshold, float max_update_texels_part, bool uniform_update, IStaticShadowsCB &cb,
  bool update_only_last_cascade)
{
  ToroidalStaticShadowCascade::BeforeRenderReturned result = updateOrigin(origin, move_texels_threshold_part, move_z_threshold,
    uniform_update, max_update_texels_part, update_only_last_cascade);
  render(cb);
  return result;
}

ToroidalStaticShadowCascade::BeforeRenderReturned ToroidalStaticShadows::updateOrigin(const Point3 &origin,
  float move_texels_threshold_part, float move_z_threshold, bool uniform_update, float max_update_texels_part,
  bool update_only_last_cascade)
{
  ShaderGlobal::set_int(static_shadows_cascades, cascadesCount());

  for (ToroidalStaticShadowCascade &cascade : cascades)
    cascade.updateSunBelowHorizon(staticShadowTex);

  // when sun is below horizon, everything is considered to be in shadow, no need to do anything
  if (!cascades.empty() && cascades[0].isSunBelowHorizon())
    return ToroidalStaticShadowCascade::BeforeRenderReturned{0, false};

  if (!isRendered)
    update_only_last_cascade = false; // render all cascades after change settings
  isRendered = true;

  eastl::fixed_vector<IPoint2, 4> cascadeWeights;
  for (int i = update_only_last_cascade ? cascades.size() - 1 : 0, e = cascades.size(); i < e; ++i)
  {
    auto &c = cascades[i];
    const int weight =
      (++c.framesSinceLastUpdate + ((c.notRenderedRegions.empty() || c.inTransition) ? 0 : 100)) * (e - i); // increase weight of
                                                                                                            // bigger cascades
    cascadeWeights.emplace_back(i, weight);
  }
  stlsort::sort(cascadeWeights.begin(), cascadeWeights.end(), [&](const auto &ai, const auto &bi) { return ai.y > bi.y; }); // sort by
                                                                                                                            // weight

  const bool needScrolling = isOrtho || worldBox.isempty() || maxHtRange < worldBox.width().y;
  if (needScrolling && !depthScroller)
  {
    depthScroller.reset(new ShadowDepthScroller);
    // Scarlett places a barrier between each tile read/write causing massive spikes
    // Doing the entire slice in one pass is much faster at the cost of more memory
    if (d3d::get_driver_code().is(d3d::scarlett))
      depthScroller->tileSizeW = depthScroller->tileSizeH = texSize;
    depthScroller->init();
  }
  bool varsChanged = false;
  for (auto cw : cascadeWeights)
  {
    const int i = cw.x;
    // we scale allowed amount of part of texture to update based on a total distance covered
    // that's because amount of objects is linear function of area in meters, not in texels
    // render time depends on both area in meters and in area in pixels
    // so we use linear scale of distance (sqrt of area)
    const float cascadeScale = cascades.back().getDistance() / (0.01f + cascades[i].getDistance());
    const float max_update = min(1.0f, max_update_texels_part * cascadeScale);
    ToroidalStaticShadowCascade::BeforeRenderReturned ret = cascades[i].updateOrigin(staticShadowTex, depthScroller.get(), origin,
      move_texels_threshold_part, move_z_threshold, max_update, uniform_update);
    if (ret.pixelsRendered)
      return ret;
    varsChanged |= ret.varsChanged;
  }
  return ToroidalStaticShadowCascade::BeforeRenderReturned{0, varsChanged};
}

int ToroidalStaticShadows::getRegionToRenderCount(int cascade) const { return cascades[cascade].getRegionToRenderCount(); }

void ToroidalStaticShadows::clearRegionToRender(int cascade) { cascades[cascade].clearRegionToRender(); }

TMatrix4 ToroidalStaticShadows::getRegionToRenderCullTm(int cascade, int region) const
{
  return cascades[cascade].getRegionToRenderCullTm(region);
}

void ToroidalStaticShadows::render(IStaticShadowsCB &cb)
{
  for (int i = 0; i < cascades.size(); ++i)
  {
    cascades[i].render(cb);
  }
}

bool ToroidalStaticShadows::isInside(const BBox3 &box) const { return !cascades.empty() ? cascades.back().isInside(box) : false; }

bool ToroidalStaticShadows::isCompletelyValid(const BBox3 &box) const
{
  for (auto &c : cascades)
    if (!c.isCompletelyValid(box))
      return false;
  return true;
}

bool ToroidalStaticShadows::isValid(const BBox3 &box) const
{
  for (auto &c : cascades)
    if (!c.isValid(box))
      return false;
  return true;
}

int ToroidalStaticShadows::getBoxViews(const BBox3 &box, ViewTransformData *out_views, int &out_cnt) const
{
  out_cnt = 0;
  if (cascades.empty())
    return -1;
  // we should return best possible cascade in view, the one that fit fully in box and is Valid
  for (int i = 0, e = cascades.size(); i < e; ++i)
  {
    auto &c = cascades[i];
    if (c.isInside(box) && c.isCompletelyValid(box))
    {
      c.getBoxViews(box, out_views, out_cnt);
      return i;
    }
  }
  return -1;
}

bool ToroidalStaticShadows::getCascadeWorldBoxViews(int cascade_id, ViewTransformData *out_views, int &out_cnt) const
{
  out_cnt = 0;
  if (cascade_id >= cascades.size())
    return false;
  cascades[cascade_id].getBoxViews(cascades[cascade_id].worldBox, out_views, out_cnt);
  return true;
}

void ToroidalStaticShadows::setWorldBox(const BBox3 &box)
{
  worldBox = box;
  for (auto &c : cascades)
    c.setWorldBox(worldBox);
}

float ToroidalStaticShadows::getMaxWorldDistance() const { return 0.55f * max(worldBox.width().x, worldBox.width().z); }

float ToroidalStaticShadows::calculateCascadeDistance(int cascade, int cascade_count, float max_distance) const
{
  G_ASSERT_RETURN(cascade < cascade_count, 0.f);
  const bool isArray = cascade_count > 1;
  cascade_count = isArray ? clamp(cascade_count, 1, 4) : 1;
  const bool isLastCascade = (cascade == cascade_count - 1);
  const float cDist = max_distance * (isLastCascade ? 1.f : (cascade + 1.f) / (cascade_count + 1.f));

  const float maxWorldDistance = worldBox.isempty() ? cDist : getMaxWorldDistance();
  return min(cDist, maxWorldDistance);
};

void ToroidalStaticShadows::setDistance(float distance)
{
  maxDistance = distance; //?
  for (int i = 0, cnt = cascades.size(); i < cnt; ++i)
  {
    float cDist = maxDistance * (i == cnt - 1 ? 1.f : (i + 1.f) / (cnt + 1));
    cascades[i].setDistance(cDist);
  }
}

void ToroidalStaticShadows::setCascadeDistance(int cascade, float distance)
{
  cascades[cascade].setDistance(distance);
  maxDistance = max(maxDistance, distance);
}

void ToroidalStaticShadows::setShaderVars()
{
  if (cascades.empty())
    return;

  for (auto &cascade : cascades)
  {
    cascade.setShaderVars();
  }
}

bool ToroidalStaticShadows::setDepthBias(float cd)
{
  bool changed = false;
  for (auto &c : cascades)
  {
    if (fabsf(c.constDepthBias - cd) > 1e-7f)
    {
      c.constDepthBias = cd;
      changed = true;
    }
  }
  return changed;
}

bool ToroidalStaticShadows::setDepthBiasForCascade(int cascade, float cd)
{
  if (cascade >= cascades.size())
    return false;

  bool changed = false;
  auto &c = cascades[cascade];
  if (fabsf(c.constDepthBias - cd) > 1e-7f)
  {
    c.constDepthBias = cd;
    changed = true;
  }
  return changed;
}

void ToroidalStaticShadows::enableTextureSpaceAlignment(bool en) { useTextureSpaceAlignment = en; }

void ToroidalStaticShadows::clearTexture()
{
  d3d::GpuAutoLock lock;
  SCOPE_RENDER_TARGET;
  d3d::set_render_target(nullptr, 0);
  const float shadowClearValue =
    ToroidalStaticShadows::GetShadowClearValue(cascades.empty() ? false : cascades[0].isSunBelowHorizon());
  if (staticShadowTex.getArrayTex())
  {
    for (int i = 0; i < cascadesCount(); ++i)
    {
      d3d::set_depth(staticShadowTex.getArrayTex(), i, DepthAccess::RW);
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, shadowClearValue, 0);
    }
  }
  else
  {
    d3d::set_depth(staticShadowTex.getTex2D(), DepthAccess::RW);
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, shadowClearValue, 0);
  }
  d3d::resource_barrier({staticShadowTex.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
}

bool ToroidalStaticShadows::isAnyCascadeInTransition() const
{
  for (auto &c : cascades)
  {
    if (c.inTransition)
      return true;
  }
  return false;
}

TMatrix ToroidalStaticShadows::getLightViewProj(const int cascadeId) const { return cascades[cascadeId].getLightViewProj(); }

bool ToroidalStaticShadows::isRegionInsideCascade(mat44f_cref cascade_inv_viewproj, const ViewTransformData &region_data)
{
  mat44f cascadeToRegion;
  v_mat44_mul(cascadeToRegion, region_data.globtm, cascade_inv_viewproj);
  vec4f cascadeLeftTop = v_make_vec4f(-1, 1, 0, 0);
  vec4f cascadeMin = v_mat44_mul_vec3p(cascadeToRegion, cascadeLeftTop);
  vec4f cascadeMax = v_mat44_mul_vec3p(cascadeToRegion, v_neg(cascadeLeftTop));
  // cascadeMin.xy <= -1 and cascadeMax.xy >= 1 means that our region is totally inside shadow cascade.
  // Otherwise some part of our region won't be rasterized and we can't verify shadows visibility there.
  vec4f eps = v_splats(0.00001f); // we lose some precision during matrix inversion/multiplication
  return v_test_all_bits_ones(v_cmp_ge(v_perm_xyab(v_neg(cascadeMin), cascadeMax), v_sub(V_C_ONE, eps)));
}

void ToroidalStaticShadows::printCascadesStatistics() const
{
#if COLLECT_STATIC_SHADOWS_STATISTICS
  String statMessage("Static shadows render statistics\n");
  int cascadeId = 0;
  for (const ToroidalStaticShadowCascade &cascade : cascades)
  {
    statMessage.aprintf(0, "Cascade %d:\n", cascadeId);
    const ToroidalStaticShadowCascade::StaticShadowsStatistics &s = cascade.staticShadowsStatistics;
    const uint32_t totalFrames = s.renderRegionFirstTimeFrames + s.invalidateRegionFrames + s.completelyValidFrames;
    statMessage.aprintf(0, "- render region first time frames %d (%.1f%%)\n", s.renderRegionFirstTimeFrames,
      100.0f * s.renderRegionFirstTimeFrames / totalFrames);
    statMessage.aprintf(0, "- invalidate region frames %d (%.1f%%)\n", s.invalidateRegionFrames,
      100.0f * s.invalidateRegionFrames / totalFrames);
    statMessage.aprintf(0, "- completely valid frames %d (%.1f%%)\n", s.completelyValidFrames,
      100.0f * s.completelyValidFrames / totalFrames);
    statMessage.aprintf(0, "- render region first time count %d\n", s.renderRegionFirstTimeCount);
    statMessage.aprintf(0, "- invalidate region count %d\n", s.invalidateRegionCount);
    cascadeId++;
  }
  debug("%s", statMessage.c_str());
#endif
}
