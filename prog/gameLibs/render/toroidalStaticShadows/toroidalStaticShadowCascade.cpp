// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadowDepthScroller.h"
#include "staticShadowsBoxListHelpers.h"
#include "staticShadowsMatrices.h"

#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <util/dag_string.h>
#include <shaders/dag_shaders.h>
#include <render/scopeRenderTarget.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/utility.h> // eastl::swap
#include <render/viewTransformData.h>
#include <math/dag_viewMatrix.h>
#include <render/toroidalStaticShadows.h>

static constexpr int TEXEL_ALIGN = 8; // align by 4 texels for performance (faster rasterization)

static IBBox2 texel_ibbox2(const BBox3 &b)
{
  return IBBox2(ipoint2(floor(Point2::xy(b[0]) - Point2(0.5, 0.5))), // we use floor -0.5 as it is rasterization rules
    ipoint2(floor(Point2::xy(b[1]) + Point2(0.5, 0.5))));            // we use floor +0.5 as it is rasterization rules
}


void ToroidalStaticShadowCascade::calcScaleOfs(const FrameData &fr, float &xy_scale, Point2 &xy_ofs, float &z_scale, float &z_ofs,
  const Point2 &origin, float bias) const
{
  xy_scale = 1. / distance;
  z_scale = 1. / (fr.zf - fr.zn);
  const float depth_bias = fr.isOrtho ? bias * 2 : bias;
  z_ofs = fr.zf * z_scale - depth_bias;
  xy_ofs = Point2(-origin.x * xy_scale, origin.y * xy_scale);
}

void ToroidalStaticShadowCascade::calcScaleOfs(const FrameData &fr, float &xy_scale, Point2 &xy_ofs, float &z_scale,
  float &z_ofs) const
{
  calcScaleOfs(fr, xy_scale, xy_ofs, z_scale, z_ofs, fr.alignedOrigin, constDepthBias / helper.texSize);
}

TMatrix ToroidalStaticShadowCascade::calcToCSPMatrix(const FrameData &fr, float bias, const Point2 &origin) const
{
  G_ASSERT(lengthSq(fr.sunDir) > 0.999f && lengthSq(fr.sunDir) < 1.001f);
  Point2 xyOfs;
  float scale, zScale, zOfs;
  calcScaleOfs(fr, scale, xyOfs, zScale, zOfs, origin, bias);
  TMatrix wtm2 = scale_z_dist(fr.lightViewProj, scale, zScale, zOfs);
  wtm2[3][0] += xyOfs.x;
  wtm2[3][1] += xyOfs.y;
  return wtm2;
}

Point3 ToroidalStaticShadowCascade::toTexelSpace(const Point3 &p0) const
{
  const FrameData &fr = getReadFrameData();
  // todo: optimize matrix mul with vec4
  Point3 csp = calcToCSPMatrix(fr, 0, Point2(0, 0)) * p0;
  csp.x = (0.5 * csp.x) * helper.texSize;
  csp.y = (-0.5 * csp.y) * helper.texSize;
  return csp;
}

BBox3 ToroidalStaticShadowCascade::toTexelSpace(const BBox3 &box_) const
{
  BBox3 box = box_;     // clamp box to world Box
  if (!current.isOrtho) // out of our world box
  {
    box[0].y = max(box[0].y, current.minHt);
    box[1].y = min(box[1].y, current.maxHt);
    if (box[0].y > box[1].y)
      return BBox3();
  }

  // transform box to our light clip space
  const FrameData &fr = getReadFrameData();
  alignas(16) TMatrix wtm = calcToCSPMatrix(fr, 0, Point2(0, 0));

  bbox3f vbox = v_ldu_bbox3(box);
  mat44f m4;
  v_mat44_make_from_43cu_unsafe(m4, wtm[0]);
  bbox3f csbox3;
  v_bbox3_init(csbox3, m4, vbox); // transform box to clip projection space

  vec4f halfTexSize = v_make_vec4f(0.5 * helper.texSize, -0.5 * helper.texSize, 1, 0);
  csbox3.bmin = v_mul(csbox3.bmin, halfTexSize);
  csbox3.bmax = v_mul(csbox3.bmax, halfTexSize);
  v_stu_bbox3(box, csbox3);
  eastl::swap(box.lim[0].y, box.lim[1].y); // min max changes with sign change
  return box;
}

BBox3 ToroidalStaticShadowCascade::toTexelSpaceClipped(const BBox3 &box_) const
{
  return toTexelSpace(worldBox.isempty() ? box_ : worldBox.getIntersection(box_));
}

void ToroidalStaticShadowCascade::close()
{
  helper.texSize = 0;
  transitionTex.close();
}

bool ToroidalStaticShadowCascade::setSunDir(const TMatrix &light_view_proj, const Point3 &dir_to_sun, bool ortho,
  float cos_force_threshold, float cos_lazy_threshold)
{
  float cosChange = dot(dir_to_sun, next.sunDir);
  G_ASSERT(cos_lazy_threshold >= cos_force_threshold);
  if (cosChange >= cos_lazy_threshold)
  {
    if (next.isOrtho == ortho)
      return false;
  }

  bool forceInvalidate = cosChange < cos_force_threshold;
  beginFrameTransition();
  invalidate(forceInvalidate);
  next.isOrtho = ortho;
  next.sunDir = dir_to_sun;
  next.lightViewProj = light_view_proj;

  return true;
}

void ToroidalStaticShadowCascade::getBoxViews(const BBox3 &box, ViewTransformData *out_views, int &out_cnt) const
{
  out_cnt = 0;
  IBBox2 region = getClippedBoxRegion(box);
  if (region.isEmpty())
    return;

  IBBox2 splitBoxes[4];
  split_toroidal(region, helper, splitBoxes, out_cnt);

  const FrameData &fr = getReadFrameData();
  const TMatrix wtm = calcToCSPMatrix(fr, 0, Point2(0, 0));
  for (int i = 0; i < out_cnt; ++i)
  {
    ToroidalQuadRegion region = getToroidalQuad(splitBoxes[i]);
    TMatrix4_vec4 globtm =
      TMatrix4(get_region_proj_tm(wtm, point2(region.texelsFrom) / helper.texSize, point2(region.wd) / helper.texSize));
    out_views[i] = ViewTransformData{
      (mat44f_cref)globtm, region.lt.x, int(helper.texSize) - region.wd.y - region.lt.y, region.wd.x, region.wd.y, 0.0f, 1.0f};
  }
}

void ToroidalStaticShadowCascade::setWorldBox(const BBox3 &box)
{
  worldBox = box;
  setDistance(configDistance);
}

void ToroidalStaticShadowCascade::setDistance(float distance_)
{
  configDistance = distance_;
  float maxWorldDistance = worldBox.isempty() ? configDistance : 0.55f * max(worldBox.width().x, worldBox.width().z);
  float cDistance = min(configDistance, maxWorldDistance);
  if (fabsf(cDistance - distance) < 1e-5f)
    return;
  distance = cDistance;
  texelSize = (distance * 2.f) / helper.texSize;
  invalidate(true);
}

void ToroidalStaticShadowCascade::setMaxHtRange(float max_ht_range)
{
  if (fabsf(next.maxHtRange - max_ht_range) > 1e-5f)
    next.maxHtRange = max_ht_range;
}

void ToroidalStaticShadowCascade::init(int cascade_id_, int texSize, float distance_, float ht_range)
{
  String tmpStr;
  cascade_id = cascade_id_;
  for (int i = 0; i < 4; i++)
  {
    tmpStr.printf(0, "static_shadow_matrix_%d_%d", i, cascade_id);
    static_shadow_matrixVarId[i] = ::get_shader_variable_id(tmpStr.str(), true);
  }
  tmpStr.printf(0, "static_shadow_cascade_%d_scale_ofs_z_tor", cascade_id);
  static_shadow_cascade_scale_ofs_z_torVarId = ::get_shader_variable_id(tmpStr.str(), true);

  helper = ToroidalHelper();
  helper.texSize = texSize;

  setMaxHtRange(ht_range);
  setDistance(distance_);
  invalidate(true);
}

void ToroidalStaticShadowCascade::invalidate(bool force)
{
  invalidRegions.clear();
  if (!force)
  {
    invalidRegions.push_back(get_texture_box(helper));
  }
  else
  {
    helper.curOrigin = IPoint2(-1000000, 100000);
    setShadervarsToInvalid();
  }
}

void ToroidalStaticShadowCascade::setShadervarsToInvalid()
{
  const Point4 matrixVals[] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {-1, -1, -1, 0}};
  for (int i = 0; i < 4; i++)
  {
    ShaderGlobal::set_color4(static_shadow_matrixVarId[i], matrixVals[i]);
  }
}

Point3 ToroidalStaticShadowCascade::clipPointToWorld(const Point3 &pos) const
{
  Point3 p0 = pos;
  if (worldBox.isempty())
    return pos;
  p0.y = clamp(p0.y, worldBox[0].y, worldBox[1].y);      // no shadows outside box
  Point2 distance95(distance * 0.95f, distance * 0.95f); // instead of full distance, use 95%, for fadeout.
  BBox2 worldBox2(Point2::xz(worldBox[0]) + distance95, Point2::xz(worldBox[1]) - distance95);
  if (worldBox2.isempty())
    p0 = Point3::xVz(worldBox.center(), p0.y);
  else
  {
    p0.x = clamp(p0.x, worldBox2[0].x, worldBox2[1].x);
    p0.z = clamp(p0.z, worldBox2[0].y, worldBox2[1].y);
  }
  return p0;
}


bool ToroidalStaticShadowCascade::isValid(const BBox3 &box) const
{
  if (notRenderedRegions.empty())
    return true;
  IBBox2 bx = texel_ibbox2(toTexelSpaceClipped(box));
  get_texture_box(helper).clipBox(bx);
  return !intersects(bx, notRenderedRegions.begin(), notRenderedRegions.end());
}

void ToroidalStaticShadowCascade::updateSunBelowHorizon(const ManagedTex &tex)
{
  if (next.sunDir.y >= 0.0f || !(current != next))
    return;

  // since the shadow texture's clear value is different based on the sun is below or above the horizon,
  //  we have to make sure we clear the texture once per change
  const bool updateTexture = current.sunDir.y >= 0.0f;

  // when the sun goes below horizon, we don't update the cascade, so we have to update "current" here
  current = next;
  inTransition = false; // Abort transition, nothing needs to be done

  if (updateTexture)
  {
    discardRegion(IPoint2(0, 0), IPoint2((int)helper.texSize, (int)helper.texSize));
    setRenderParams(tex.getBaseTex());
  }
}

bool ToroidalStaticShadowCascade::isCompletelyValid(const BBox3 &box) const
{
  if (invalidRegions.empty())
    return true;
  IBBox2 bx = texel_ibbox2(toTexelSpaceClipped(box));
  get_texture_box(helper).clipBox(bx);
  return !intersects(bx, invalidRegions.begin(), invalidRegions.end());
}

bool ToroidalStaticShadowCascade::isInside(const BBox3 &box) const
{
  const IBBox2 bx = texel_ibbox2(toTexelSpaceClipped(box));
  return bx.isEmpty() ? false : bx & get_texture_box(helper);
}

void ToroidalStaticShadowCascade::setShaderVars()
{
  if (!helper.texSize)
    return;

  const FrameData &fr = getReadFrameData();

  ShaderGlobal::set_color4(static_shadow_cascade_scale_ofs_z_torVarId, 1, 0, -fr.torOfs.x + 0.5, fr.torOfs.y + 0.5);

  TMatrix wtm = getLightViewProj();

  ShaderGlobal::set_color4(static_shadow_matrixVarId[0], P3D(wtm.getcol(0)), 1. / helper.texSize);
  ShaderGlobal::set_color4(static_shadow_matrixVarId[1], P3D(wtm.getcol(1)), 0);
  ShaderGlobal::set_color4(static_shadow_matrixVarId[2], P3D(wtm.getcol(2)), 0);
  ShaderGlobal::set_color4(static_shadow_matrixVarId[3], P3D(wtm.getcol(3)), 0);
}

TMatrix ToroidalStaticShadowCascade::getLightViewProj() const
{
  const FrameData &fr = getReadFrameData();
  Point2 xyOfs;
  float scale, zScale, zOfs;
  calcScaleOfs(fr, scale, xyOfs, zScale, zOfs);

  TMatrix wtm = fr.lightViewProj;
  if (fr.sunDir.y >= 0.0f) // If the sun is below the horizon, just set the dummy matrix, that transforms everything to (0,0,2)
  {
    wtm = scale_z_dist(wtm, scale, zScale, zOfs);
    wtm[3][0] += xyOfs.x;
    wtm[3][1] += xyOfs.y;
  }
  return wtm;
}

void ToroidalStaticShadowCascade::enableTransitionCascade(bool en)
{
  if (!en)
  {
    transitionTex.close();
    inTransition = false;
    return;
  }

  if (!transitionTex)
  {
    transitionTex = dag::create_tex(nullptr, helper.texSize, helper.texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1,
      String(26, "static_shadow_trans_tex_%d", cascade_id));
  }
}

void ToroidalStaticShadowCascade::beginFrameTransition()
{
  if (!transitionTex || inTransition)
    return;

  backFrame = current;
  inTransition = true;
}

bool ToroidalStaticShadowCascade::endFrameTransition(const ManagedTex &tex)
{
  if (!transitionTex)
    return false;

  bool wasTrans = inTransition;

  if (wasTrans)
  {
    copyTransitionAfterRender = true;
    transitionCopyTarget = tex.getBaseTex();
  }

  inTransition = false;
  return wasTrans;
}

bool ToroidalStaticShadowCascade::changeDepth(const ManagedTex &tex, ShadowDepthScroller *s, float scale, float ofs)
{
  if (ofs == 0 && scale == 1.0f) // we compare for exact!
    return false;
  if (s == nullptr)
  {
    // no scroller. we can only force invalidation
    invalidate(true);
    return false;
  }
  if (!s->translateDepth(tex.getBaseTex(), tex.getArrayTex() ? cascade_id : -1, scale, ofs)) // nothing has changed
    return false;
  // todo: add partially invalidate parts to re-render - those part of texture quad that, if clamped to worldBox, were outside
  invalidate(false); // todo: we should invalidate only parts of texture that were invalid (using worldBox)
  return true;
}

void ToroidalStaticShadowCascade::calcNext(const Point3 &pos)
{
  const uint32_t depthAccuracy = 1 << 16; // use snapped depth, to avoid rounding error
  const bool hasWorldBox = !worldBox.isempty();
  const bool worldFitInFullZRange = (!next.isOrtho && hasWorldBox && worldBox.width().y <= next.maxHtRange);
  if (!worldFitInFullZRange)
  {
    float camHt = pos.y;
    if (hasWorldBox)
    {
      camHt = max(camHt, worldBox[0].y + next.maxHtRange * 0.5f);
      camHt = min(camHt, worldBox[1].y - next.maxHtRange * 0.5f); // order is important. there is no need to render above maxHt
    }
    Point2 minMaxHt = Point2(camHt - next.maxHtRange * 0.5f, camHt + next.maxHtRange * 0.5f);
    if (fabsf(minMaxHt[0] - next.minHt) > next.maxHtRange * 0.1 || fabsf(minMaxHt[1] - next.maxHt) > next.maxHtRange * 0.1 ||
        fabsf(next.maxHt - next.minHt - next.maxHtRange) > 1.f)
    {
      next.minHt = minMaxHt[0];
      next.maxHt = minMaxHt[1];
    }
  }
  else
  {
    next.minHt = worldBox[0].y;
    next.maxHt = worldBox[1].y;
  }
  if (next.isOrtho)
  {
    calculate_ortho_znzfar(pos, next.sunDir, distance, next.zn, next.zf);
  }
  else
  {
    calculate_skewed_znzfar(next.minHt, next.maxHt, next.sunDir, next.zn, next.zf);
  }
  double zRange = (next.zf - next.zn);
  zRange = ceilf(zRange * depthAccuracy) / depthAccuracy;
  next.zf = ceilf(next.zf * depthAccuracy) / depthAccuracy;
  next.zn = next.zf - zRange;

  next.torOfs = current.torOfs;
  next.alignedOrigin = current.alignedOrigin;
}

IPoint2 ToroidalStaticShadowCascade::getTexelOrigin(const Point3 &pos) const
{
  const Point3 txP = toTexelSpace(clipPointToWorld(pos));

  IPoint2 newTexelsOrigin = ipoint2(Point2::xy(txP));
  newTexelsOrigin += IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2);
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);
  return newTexelsOrigin;
}

ToroidalStaticShadowCascade::BeforeRenderReturned ToroidalStaticShadowCascade::updateOrigin(const ManagedTex &tex,
  ShadowDepthScroller *scroller, const Point3 &pos, float move_part, float move_z_threshold, float max_update_part, bool uniformUpdate)
{
  framesSinceLastUpdate = 0;
  if (!helper.texSize)
    return BeforeRenderReturned{0, false};

  calcNext(clipPointToWorld(pos));
  if (current != next)
  {
    const bool changedOrtho = next.isOrtho != current.isOrtho;
    const double cZRange = (current.zf - current.zn), nextZRange = (next.zf - next.zn), cZf = current.zf;
    const bool z_moved = (cZRange != nextZRange || fabs(next.zn - current.zn) / nextZRange > move_z_threshold);
    double z_scale = cZRange / nextZRange, z_ofs = (next.zf - cZf) / nextZRange;
    const bool forceInvalidate = changedOrtho || dot(current.sunDir, next.sunDir) < 0.999f || z_scale + z_ofs <= 0.0f // point with
                                                                                                                      // depth 1.0
                                                                                                                      // moved below
                                                                                                                      // 0.0
                                 || z_ofs >= 1.0f; // point with depth 0.0 moved above 1.0
    if (!z_moved && !forceInvalidate)
    {
      const IPoint2 move = abs(getTexelOrigin(pos) - helper.curOrigin);
      if (max(move.x, move.y) < helper.texSize)
      {
        // if we teleported anyway, we better use next zn/zf anyway!
        next.zn = current.zn;
        next.zf = current.zf;
      }
    }

    current = next;
    const IPoint2 move = abs(getTexelOrigin(pos) - helper.curOrigin);
    const bool fullUpdate = (max(move.x, move.y) >= helper.texSize);
    // check if we are completely invalid anyway (teleported) and then better not change depth
    if (z_moved)
    {
      if (scroller && !forceInvalidate && !fullUpdate)
      {
        if (inTransition)
        {
          backFrame.zn = current.zn;
          backFrame.zf = current.zf;
        }

        changeDepth(tex, scroller, z_scale, z_ofs);
        return BeforeRenderReturned{helper.texSize * helper.texSize, true, true};
      }
      else // if we are completely invalid anyway, better invalidate it
      {
        invalidate(true);
      }
    }
  }

  IPoint2 newTexelsOrigin = getTexelOrigin(pos);

  const IPoint2 halfTexSize(helper.texSize >> 1, helper.texSize >> 1);

  int texelsMoveThreshold = ((move_part * helper.texSize) + TEXEL_ALIGN - 1);
  texelsMoveThreshold = max((int)0, min(texelsMoveThreshold, (int)helper.texSize));
  texelsMoveThreshold = texelsMoveThreshold - texelsMoveThreshold % TEXEL_ALIGN;
  const int maxTexelsToUpdate = clamp(max_update_part, 0.001f, 1.f) * helper.texSize * helper.texSize;

  IPoint2 move = abs(helper.curOrigin - newTexelsOrigin);
  if (move.x < texelsMoveThreshold && move.y < texelsMoveThreshold && !invalidRegions.size())
    return BeforeRenderReturned{0, false};
  const int dynamicThreshold = invalidRegions.size() ? min(texelsMoveThreshold * 2, (int)helper.texSize / 2) : texelsMoveThreshold;
  // preferrably update already invalidRegions, unless we moved even more
  // so we have less invalid regions
  bool ret = move.x >= dynamicThreshold || move.y >= dynamicThreshold;
  if (!ret && invalidRegions.empty())
  {
    return BeforeRenderReturned{0, endFrameTransition(tex)};
  }

  Tab<IBBox2> regionsToDiscard;
  if (ret)
  {
    const float fullUpdateThreshold = optimizeBigQuadsRender ? 0.75 : 0.45; // -V547
    const int fullUpdateThresholdTexels = fullUpdateThreshold * helper.texSize;
    if (max(move.x, move.y) < fullUpdateThresholdTexels) // if distance travelled is too big, there is no need to update movement in
                                                         // two steps
    {
      if (move.x < move.y)
        newTexelsOrigin.x = helper.curOrigin.x;
      else
        newTexelsOrigin.y = helper.curOrigin.y;
    }

    ToroidalGatherCallback::RegionTab regions0;
    ToroidalGatherCallback shadowcb(regions0);
    uint32_t texelsUpdated = (uint32_t)max((int)0, toroidal_update(newTexelsOrigin, helper, fullUpdateThresholdTexels, shadowcb));
    current.torOfs = point2((helper.mainOrigin - helper.curOrigin) % helper.texSize) / helper.texSize;
    current.alignedOrigin = point2(helper.curOrigin) * texelSize;

    G_ASSERT(texelsUpdated > 0);
    clip_list_to(notRenderedRegions, get_texture_box(helper));
    toroidal_clip_regions(helper, invalidRegions);
    if (texelsUpdated <= maxTexelsToUpdate * 2 || !optimizeBigQuadsRender) // -V560
    {
      for (int i = 0; i < regions0.size(); ++i)
      {
        ToroidalQuadRegion &s = regions0[i];
        IBBox2 b = IBBox2(s.texelsFrom, s.texelsFrom + s.wd);
        toroidal_clip_region(helper, b);
        subtract_region_from_non_intersected_list(b, notRenderedRegions);
      }

      setRenderParams(tex.getBaseTex());
      renderRegions(regions0);

      return BeforeRenderReturned{texelsUpdated, true};
    }
    else // too much
    {
      for (int i = 0; i < regions0.size(); ++i)
      {
        ToroidalQuadRegion &s = regions0[i];
        IBBox2 b = IBBox2(s.texelsFrom, s.texelsFrom + s.wd);
        toroidal_clip_region(helper, b);
        add_non_intersected_box(invalidRegions, b);
        add_non_intersected_box(regionsToDiscard, b);
      }
    }
  }

  if (inTransition)
    setRenderParams(transitionTex.getBaseTex());
  else
    setRenderParams(tex.getBaseTex());

  for (auto &b : regionsToDiscard)
  {
    add_non_intersected_box(notRenderedRegions, b);
    Point2 p = transform_point_to_viewport(b[0], helper);
    Point2 w = b.width();
    IPoint2 lt = IPoint2(p.x, helper.texSize - w.y - p.y - 1);
    IPoint2 wh = IPoint2(w.x + 1, w.y + 1);
    IPoint2 br = lt + wh;

    lt.x = clamp<int>(lt.x, 0, helper.texSize);
    lt.y = clamp<int>(lt.y, 0, helper.texSize);
    br.x = clamp<int>(br.x, 0, helper.texSize);
    br.y = clamp<int>(br.y, 0, helper.texSize);
    wh = br - lt;

    if (wh.x > 0 && wh.y > 0)
      discardRegion(lt, wh);
  }

  uint32_t pixelsRendered = 0;
  // get closest invalid region to re-render
  const bool notRenderedChosen = !notRenderedRegions.empty();
  Tab<IBBox2> &choosenRegions = notRenderedChosen ? notRenderedRegions : invalidRegions; // we preferrably render discarded regions
                                                                                         // first
  Tab<IBBox2> &otherRegions = !notRenderedChosen ? notRenderedRegions : invalidRegions;

  const int desiredSideDims = int(sqrtf(float(maxTexelsToUpdate)));

  while (pixelsRendered < maxTexelsToUpdate)
  {
    const int id = get_closest_region_and_split(helper, choosenRegions);
    if (id >= 0)
    {
      IBBox2 restRegions[4];
      int restCount = 0;
      if (!uniformUpdate)
      {
        // split region along it's longest axis based on area
        restCount = split_region_to_size(maxTexelsToUpdate, choosenRegions[id], choosenRegions[id], restRegions, helper.curOrigin);
      }
      else
      {
        // split region twice along it's longest axis based on linear dims
        restCount =
          split_region_to_size_linear(desiredSideDims, choosenRegions[id], choosenRegions[id], restRegions, helper.curOrigin);
        restCount += split_region_to_size_linear(desiredSideDims, choosenRegions[id], choosenRegions[id], &restRegions[restCount],
          helper.curOrigin);
      }
      if (restCount)
        append_items(choosenRegions, restCount, restRegions);

      // finally, update chosen invalid region.
      subtract_region_from_non_intersected_list(choosenRegions[id], otherRegions);
      ToroidalQuadRegion quad = getToroidalQuad(choosenRegions[id]);
      pixelsRendered += quad.wd.x * quad.wd.y;
      renderRegions(dag::ConstSpan<ToroidalQuadRegion>(&quad, 1));

      // and remove it from update list
      erase_items(choosenRegions, id, 1);

      if (restCount && stopRenderingAfterRegionSplit)
        break;
    }
    if (notRenderedRegions.empty()) // if there is no empty regions, stop now. better lazily update
      break;
  }

  if (!notRenderedRegions.empty())
  {
    // todo:
    // find biggest updated quad from center, so to avoid blinking
    //  may be one float is enough, but we can also use two for both coordinates
  }

  if (invalidRegions.empty())
    ret = endFrameTransition(tex);

  return BeforeRenderReturned{pixelsRendered, ret};
}

void ToroidalStaticShadowCascade::render(IStaticShadowsCB &cb)
{
  if (renderData.regionsToDiscard.empty() && renderData.regionsToRender.empty())
    return;

  TIME_D3D_PROFILE(staticShadowRender);
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;
  d3d_err(d3d::set_render_target(nullptr, 0));

  if (renderData.target->restype() == RES3D_ARRTEX)
    d3d::set_depth(renderData.target, cascade_id, DepthAccess::RW);
  else
    d3d::set_depth(renderData.target, DepthAccess::RW);

  const float shadowClearValue = ToroidalStaticShadows::GetShadowClearValue(renderData.isSunBelowHorizon);

  for (const RenderData::RegionToDiscard &region : renderData.regionsToDiscard)
  {
    d3d::setview(region.lt.x, region.lt.y, region.wh.x, region.wh.y, 0.0f, 1.0f);
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, shadowClearValue, 0);
  }
  renderData.regionsToDiscard.clear();


  d3d::settm(TM_VIEW, renderData.viewTm);
  cb.startRenderStaticShadow(renderData.viewTm, renderData.znzf, renderData.minHt, renderData.maxHt);

  for (int i = 0; i < renderData.regionsToRender.size(); ++i)
  {
    const RenderData::RegionToRender &region = renderData.regionsToRender[i];
    d3d::settm(TM_PROJ, &region.projMatrix);
    d3d::setview(region.transform.x, region.transform.y, region.transform.w, region.transform.h, region.transform.minz,
      region.transform.maxz);
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, shadowClearValue, 0);

    cb.renderStaticShadowDepth((const mat44f &)region.cullViewProj, region.transform, i);
  }
  renderData.regionsToRender.clear();

  cb.endRenderStaticShadow();

  d3d::resource_barrier({renderData.target, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});

  if (copyTransitionAfterRender)
  {
    if (transitionCopyTarget->restype() == RES3D_ARRTEX)
    {
      transitionCopyTarget->updateSubRegion(transitionTex.getTex2D(), 0, 0, 0, 0, helper.texSize, helper.texSize, 1, cascade_id, 0, 0,
        0);
    }
    else
    {
      transitionCopyTarget->update(transitionTex.getTex2D());
    }
    d3d::resource_barrier({transitionCopyTarget, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    copyTransitionAfterRender = false;
  }
}

void ToroidalStaticShadowCascade::invalidateBox(const BBox3 &box)
{
  TIME_PROFILE_DEV(staticShadowCascade_invalidateBox);
  IBBox2 ibox = getClippedBoxRegion(box);
  if (ibox.isEmpty())
    return;

  DA_PROFILE_TAG(invalidRegions.size, "%u", invalidRegions.size());
  add_non_intersected_box(invalidRegions, ibox);
}

inline int area(const IBBox2 &a)
{
  auto v = a.width();
  return v.x * v.y;
}
void ToroidalStaticShadowCascade::invalidateBoxes(const BBox3 *box, uint32_t cnt)
{
  if (!cnt)
    return;
  TIME_PROFILE_DEV(staticShadowCascade_invalidateBoxes);
  dag::Vector<IBBox2, framemem_allocator> clippedBoxes;
  for (auto be = box + cnt; box != be; ++box)
  {
    IBBox2 ibox = getClippedBoxRegion(*box);
    if (!ibox.isEmpty())
      clippedBoxes.push_back(ibox);
  }
  if (clippedBoxes.empty())
    return;
  stlsort::sort(clippedBoxes.begin(), clippedBoxes.end(), [](auto &a, auto &b) { return area(a) > area(b); });
  DA_PROFILE_TAG(invalidRegions.size, "%u %u", invalidRegions.size(), clippedBoxes.size());
  for (auto &ibox : clippedBoxes)
    add_non_intersected_box(invalidRegions, ibox);
}

IBBox2 ToroidalStaticShadowCascade::getClippedBoxRegion(const BBox3 &box) const
{
  if (!current.isOrtho && (box[0].y > current.maxHt || box[1].y < current.minHt)) // out of our world box
    return IBBox2();

  const BBox3 tspBox = toTexelSpace(box);

  if (tspBox.isempty() || tspBox[0].z > 1.f || tspBox[1].z < 0.f)
    return IBBox2();
  // transform box to texel space

  IBBox2 ibox(ipoint2(floor(Point2::xy(tspBox[0]))), ipoint2(floor(Point2::xy(tspBox[1])))); // it is floor for lim1, as we 'include'
                                                                                             // rightbottom texel
  toroidal_clip_region(helper, ibox);
  return ibox;
}

ToroidalQuadRegion ToroidalStaticShadowCascade::getToroidalQuad(const IBBox2 &region) const
{
  return ToroidalQuadRegion(transform_point_to_viewport(region[0], helper), region.width() + IPoint2(1, 1), region[0]);
}


void ToroidalStaticShadowCascade::setRenderParams(BaseTexture *target)
{
  renderData.target = target;
  renderData.viewTm = light_dir_ortho_tm(current.sunDir);
  renderData.viewItm = orthonormalized_inverse(renderData.viewTm);
  renderData.wtm = calcToCSPMatrix(current, 0, Point2(0, 0));
  renderData.znzf = Point2(current.zn, current.zf);
  renderData.minHt = current.minHt;
  renderData.maxHt = current.maxHt;
  renderData.isSunBelowHorizon = current.sunDir.y < 0;
}

void ToroidalStaticShadowCascade::discardRegion(const IPoint2 &leftTop, const IPoint2 &widthHeight)
{
  renderData.regionsToDiscard.push_back({leftTop, widthHeight});
}

void ToroidalStaticShadowCascade::renderRegions(dag::ConstSpan<ToroidalQuadRegion> regions)
{
  for (int i = 0, e = regions.size(); i < e; ++i)
  {
    RenderData::RegionToRender &region = renderData.regionsToRender.emplace_back();

    TMatrix4 lightProj;
    TMatrix4_vec4 cullViewProj;
    // new version:
    const TMatrix scaledTm =
      get_region_proj_tm(renderData.wtm, point2(regions[i].texelsFrom) / helper.texSize, point2(regions[i].wd) / helper.texSize);
    lightProj = TMatrix4(scaledTm * renderData.viewItm);
    // todo: check if we can simplify math
    TMatrix4 z_scale = TMatrix4::IDENT;
    z_scale[2][2] = 0.5; // increase 'up' distance 2 times
    z_scale[3][2] = 0.5;
    cullViewProj = TMatrix4(scaledTm) * z_scale; //*0.5

    region.projMatrix = lightProj;

    G_ASSERTF(regions[i].lt.x + regions[i].wd.x <= helper.texSize && helper.texSize - regions[i].lt.y <= helper.texSize,
      "lt = %@, wd = %@ texSize = %@", regions[i].lt, regions[i].wd, helper.texSize);

    ViewTransformData curTransform;

    TMatrix4 shadowGlobTm = TMatrix4(renderData.viewTm) * lightProj;
    v_mat44_make_from_44cu(curTransform.globtm, &shadowGlobTm._11);

    curTransform.x = regions[i].lt.x;
    curTransform.y = helper.texSize - regions[i].wd.y - regions[i].lt.y;
    curTransform.w = regions[i].wd.x;
    curTransform.h = regions[i].wd.y;
    curTransform.minz = 0;
    curTransform.maxz = 1;
    curTransform.cascade = cascade_id;

    region.cullViewProj = cullViewProj;
    region.transform = curTransform;
  }
}

int ToroidalStaticShadowCascade::getRegionToRenderCount() const { return renderData.regionsToRender.size(); }

void ToroidalStaticShadowCascade::clearRegionToRender() { renderData.regionsToRender.clear(); }

TMatrix4 ToroidalStaticShadowCascade::getRegionToRenderCullTm(int region) const
{
  return renderData.regionsToRender[region].cullViewProj;
}