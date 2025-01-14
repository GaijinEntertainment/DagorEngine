// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daGI2/treesAboveDepth.h>

#include <memory/dag_framemem.h>
#include <math/dag_bounds3.h>
#include <math/dag_mathUtils.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_texture.h>
#include <util/dag_convar.h>

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>

#include <perfMon/dag_statDrv.h>

#include <generic/dag_sort.h>
#include <generic/dag_carray.h>

#include <render/toroidal_update_regions.h>
#define GLOBAL_VARS_LIST              \
  VAR(world_to_trees_tex_ofs)         \
  VAR(world_to_trees_tex_mul)         \
  VAR(trees2d_depth_region)           \
  VAR(trees2d_depth_samplerstate)     \
  VAR(trees2d_depth_min_samplerstate) \
  VAR(trees2d_samplerstate)           \
  VAR(trees2d_clear_regions_arr)

static ShaderVariableInfo object_tess_factorVarId{"object_tess_factor", true};

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

#define MAX_REGIONS_PER_FRAME 1u
#define MAX_STORED_REGIONS    16u


template <class Container>
inline void add_non_intersected_box_with_collapse(Container &boxes, const IBBox2 &ibox, unsigned max_boxes)
{
  add_non_intersected_box(boxes, ibox);
  if (boxes.size() > max_boxes)
  {
    // merge into single bounding box to keep number of boxes down
    IBBox2 fullBox;
    for (auto &b : boxes)
      fullBox += b;
    boxes.resize(1);
    boxes[0] = fullBox;
  }
}


void TreesAboveDepth::renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
{
  G_UNUSED(texelsFrom);
  add_non_intersected_box_with_collapse(regionsToClear, IBBox2(lt, lt + wd - IPoint2(1, 1)), MAX_STORED_REGIONS);
}


static inline int signed_wrap(int u, int size_pow2) { return ((u + (size_pow2 >> 1)) & (size_pow2 - 1)) - (size_pow2 >> 1); }

static inline int wrapped_box_distance(int bmin, int bmax, int pos, int size_pow2)
{
  int u0 = signed_wrap(bmin - pos, size_pow2);
  int u1 = signed_wrap(bmax - pos, size_pow2);

  // if different signs and not swapped, then pos is inside the box
  if ((u0 ^ u1) < 0 && u0 < u1)
    return 0;
  else
    return eastl::min(abs(u0), abs(u1));
}

static inline int wrapped_box_distance(const IBBox2 &box, const IPoint2 &pos, int size_pow2)
{
  return wrapped_box_distance(box.left(), box.right(), pos.x, size_pow2) +
         wrapped_box_distance(box.top(), box.bottom(), pos.y, size_pow2);
}


void TreesAboveDepth::prepare(const Point3 &origin, float minZ, float maxZ, const render_cb &rcb)
{
  if (!trees2d.getTex2D())
    return;
  Point2 alignedOrigin = Point2::xz(origin);
  const float fullDistance = 2 * trees2dDist;
  const float texelSize = (fullDistance / trees2dHelper.texSize);
  static constexpr int TEXEL_ALIGN = 4;
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);

  static constexpr int THRESHOLD = TEXEL_ALIGN * 4;
  IPoint2 move = abs(trees2dHelper.curOrigin - newTexelsOrigin);
  if (move.x >= THRESHOLD || move.y >= THRESHOLD)
  {
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * trees2dHelper.texSize;
    if (max(move.x, move.y) < fullUpdateThresholdTexels) // if distance travelled is too big, there is no need to update movement in
                                                         // two steps
    {
      if (move.x < move.y)
        newTexelsOrigin.x = trees2dHelper.curOrigin.x;
      else
        newTexelsOrigin.y = trees2dHelper.curOrigin.y;
    }
    else if (max(move.x, move.y) >= trees2dHelper.texSize)
    {
      regionsToClear.clear();
      regionsToUpdate.clear();
    }

    toroidal_update(newTexelsOrigin, trees2dHelper, fullUpdateThresholdTexels, *this);

    Point2 ofs = point2((trees2dHelper.mainOrigin - trees2dHelper.curOrigin) % trees2dHelper.texSize) / trees2dHelper.texSize;

    alignedOrigin = point2(trees2dHelper.curOrigin) * texelSize;

    ShaderGlobal::set_color4(world_to_trees_tex_mulVarId, 1.0f / fullDistance, -alignedOrigin.x / fullDistance + 0.5,
      -alignedOrigin.y / fullDistance + 0.5, trees2dHelper.texSize);
    ShaderGlobal::set_color4(world_to_trees_tex_ofsVarId, (maxZ - minZ), minZ, ofs.x, ofs.y);
  }

  for (auto &b : regionsToClear)
    add_non_intersected_box_with_collapse(regionsToUpdate, b, MAX_STORED_REGIONS);

  if (regionsToUpdate.empty())
  {
    regionsToUpdate.shrink_to_fit();
    G_ASSERT(regionsToClear.empty());
    return;
  }

  // split big update regions to reduce spikes
  for (int i = 0; i < regionsToUpdate.size(); i++)
  {
    auto &b0 = regionsToUpdate[i];
    auto wd = b0.size() + IPoint2(1, 1);
    if (wd.x * 2 > trees2dHelper.texSize)
    {
      auto m0 = b0[0].x + wd.x / 4;
      auto m1 = b0[0].x + wd.x * 3 / 4;
      IBBox2 b1 = b0, b2 = b0;
      b0[1].x = m0 - 1;
      b1[0].x = m0;
      b1[1].x = m1 - 1;
      b2[0].x = m1;
      regionsToUpdate.push_back(b2);
      regionsToUpdate.push_back(b1);
    }
    else if (wd.y * 2 > trees2dHelper.texSize)
    {
      auto m0 = b0[0].y + wd.y / 4;
      auto m1 = b0[0].y + wd.y * 3 / 4;
      IBBox2 b1 = b0, b2 = b0;
      b0[1].y = m0 - 1;
      b1[0].y = m0;
      b1[1].y = m1 - 1;
      b2[0].y = m1;
      regionsToUpdate.push_back(b2);
      regionsToUpdate.push_back(b1);
    }
  }

  DA_PROFILE_GPU;
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;

  // clear all regions first
  if (!regionsToClear.empty())
  {
    TIME_D3D_PROFILE(trees_above_clear);
    carray<IPoint4, MAX_STORED_REGIONS> shaderRegions;
    G_ASSERT(regionsToClear.size() <= shaderRegions.size());

    for (int i = 0; i < regionsToClear.size(); i++)
    {
      auto &b = regionsToClear[i];
      shaderRegions[i] = IPoint4(b.getMin().x, b.getMin().y, b.getMax().x, b.getMax().y);
    }
    for (int i = regionsToClear.size(); i < shaderRegions.size(); i++)
      shaderRegions[i] = IPoint4(1, 1, 0, 0); // empty, inclusive min/max

    ShaderGlobal::set_int4_array(trees2d_clear_regions_arrVarId, shaderRegions.data(), shaderRegions.size());

    d3d::set_render_target(nullptr, 0);
    d3d::set_depth(trees2dDepthMin.getTex2D(), 0, DepthAccess::RW);

    d3d::setview(0, 0, trees2dHelper.texSize, trees2dHelper.texSize, 0, 1);
    clearRegions.render();

    d3d::set_render_target(trees2d.getTex2D(), 0);
    d3d::set_depth(trees2dDepth.getTex2D(), 0, DepthAccess::RW);

    d3d::setview(0, 0, trees2dHelper.texSize, trees2dHelper.texSize, 0, 1);
    clearRegions.render();

    d3d::resource_barrier({trees2dDepth.getTex2D(), RB_RO_SRV | stageAll | RB_SOURCE_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({trees2dDepthMin.getTex2D(), RB_RO_SRV | stageAll | RB_SOURCE_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({trees2d.getTex2D(), RB_RO_SRV | stageAll | RB_SOURCE_STAGE_PIXEL, 0, 0});

    clear_and_shrink(regionsToClear);
  }

  // sort regions by proximity to camera
  auto texCenter =
    wrap_texture(IPoint2(trees2dHelper.texSize >> 1, trees2dHelper.texSize >> 1) + trees2dHelper.curOrigin - trees2dHelper.mainOrigin,
      trees2dHelper.texSize);
  stlsort::sort(regionsToUpdate.begin(), regionsToUpdate.end(),
    [size = trees2dHelper.texSize, texCenter](const IBBox2 &a, const IBBox2 &b) {
      return (wrapped_box_distance(a, texCenter, size) > wrapped_box_distance(b, texCenter, size));
    });

  auto currentRegions = make_span(regionsToUpdate).last(min(MAX_REGIONS_PER_FRAME, regionsToUpdate.size()));

  // disable tesselation for speed as its contribution is very low
  float oldPnTriangulation = ShaderGlobal::get_real(object_tess_factorVarId);
  ShaderGlobal::set_real(object_tess_factorVarId, 0);

  d3d::set_render_target(nullptr, 0);
  d3d::set_depth(trees2dDepthMin.getTex2D(), 0, DepthAccess::RW);

  for (auto &reg : currentRegions)
    renderRegion(reg, texelSize, minZ, maxZ, rcb, true);

  d3d::set_render_target(trees2d.getTex2D(), 0);
  d3d::set_depth(trees2dDepth.getTex2D(), 0, DepthAccess::RW);

  for (auto &reg : currentRegions)
    renderRegion(reg, texelSize, minZ, maxZ, rcb, false);

  d3d::resource_barrier({trees2dDepth.getTex2D(), RB_RO_SRV | stageAll | RB_SOURCE_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({trees2dDepthMin.getTex2D(), RB_RO_SRV | stageAll | RB_SOURCE_STAGE_PIXEL, 0, 0});

  // fixme: replace with quads rendering
  d3d::set_render_target(trees2d.getTex2D(), 0);
  for (auto &reg : currentRegions)
    renderRegionAlpha(reg);

  d3d::resource_barrier({trees2d.getTex2D(), RB_RO_SRV | stageAll | RB_SOURCE_STAGE_PIXEL, 0, 0});
  ShaderGlobal::set_real(object_tess_factorVarId, oldPnTriangulation);

  regionsToUpdate.resize(regionsToUpdate.size() - currentRegions.size());
}

void TreesAboveDepth::init(float half_distance, float texel_size)
{
  const int trees2dDRes = get_bigger_pow2(ceilf(2.f * half_distance / texel_size));
  const float nextTrees2dDist = half_distance;
  if (trees2dDRes == trees2dHelper.texSize)
  {
    if (fabsf(nextTrees2dDist - trees2dDist) > texel_size)
    {
      trees2dDist = nextTrees2dDist;
      invalidate();
    }
    return;
  }

  trees2dDist = nextTrees2dDist;
  debug("init trees2d map tex %d^2, texel %f, range %f", trees2dDRes, trees2dDist * 2 / trees2dDRes, trees2dDist);
  trees2d.set(d3d::create_tex(NULL, trees2dDRes, trees2dDRes, TEXCF_RTARGET | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, 1, "trees2d"),
    "trees2d");
  trees2d.setVar();

  trees2dDepth.set(d3d::create_tex(NULL, trees2dDRes, trees2dDRes, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1, "trees2d_depth"),
    "trees2d_depth");
  trees2dDepth.setVar();

  trees2dDepthMin.set(d3d::create_tex(NULL, trees2dDRes, trees2dDRes, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1, "trees2d_depth_min"),
    "trees2d_depth_min");
  trees2dDepthMin.setVar();

  trees2d.getTex2D()->disableSampler();
  trees2dDepth.getTex2D()->disableSampler();
  trees2dDepthMin.getTex2D()->disableSampler();
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    trees2d_samplerstateVarId.set_sampler(sampler);
    trees2d_depth_samplerstateVarId.set_sampler(sampler);
    trees2d_depth_min_samplerstateVarId.set_sampler(sampler);
  }
  trees2dHelper.curOrigin = IPoint2(-1000000, 1000000);
  trees2dHelper.texSize = trees2dDRes;
  writeDepthToAlpha.init("trees2d_depth_write_to_alpha");
  clearRegions.init("trees2d_clear");
}

void TreesAboveDepth::invalidate() { trees2dHelper.curOrigin = IPoint2(-1024 * 1024, 1024 * 1024); }

void TreesAboveDepth::renderRegionAlpha(IBBox2 &reg)
{
  ShaderGlobal::set_int4(trees2d_depth_regionVarId, reg.left(), reg.top(), 0, 0); // fixme: render in one dip with quadso
  d3d::setview(reg.left(), reg.top(), reg.size().x + 1, reg.size().y + 1, 0, 1);
  writeDepthToAlpha.render();
}

void TreesAboveDepth::renderRegion(IBBox2 &reg, float texelSize, float minZ, float maxZ, const render_cb &cb, bool depth_min)
{
  auto newCenter = trees2dHelper.curOrigin;
  auto mainCenter = trees2dHelper.mainOrigin;
  auto texSize = trees2dHelper.texSize;
  IPoint2 newLT = newCenter - IPoint2(texSize, texSize) / 2;
  IPoint2 texelsFrom = (reg.leftTop() + ((mainCenter - newCenter) % texSize) + IPoint2(texSize, texSize)) % texSize + newLT;

  d3d::setview(reg.left(), reg.top(), reg.size().x + 1, reg.size().y + 1, 0, 1);
  BBox2 box2(texelSize * point2(texelsFrom), texelSize * point2(texelsFrom + reg.size() + IPoint2(1, 1)));
  BBox3 box(Point3(box2[0].x, minZ, box2[0].y), Point3(box2[1].x, maxZ, box2[1].y));
  // d3d::clearview(CLEAR_ZBUFFER | CLEAR_TARGET, 0, 0.0f, 0);

  cb(box, depth_min);
}

void TreesAboveDepth::invalidateTrees2d(const BBox3 &box, const TMatrix &tm)
{
  if (!trees2d.getTex2D())
    return;

  mat44f mat;
  v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

  bbox3f bbox;
  v_bbox3_init(bbox, mat, v_ldu_bbox3(box));
  BBox2 box2;
  v_stu(&box2.lim[0].x, v_perm_xzac(bbox.bmin, bbox.bmax));

  const float fullDistance = 2 * trees2dDist;
  const float texelSize = (fullDistance / trees2dHelper.texSize);
  toroidal_invalidate_boxes(trees2dHelper, texelSize, make_span(&box2, 1), *this);
}
