//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/visibilityDecl.h>
#include <rendInst/layerFlags.h>

#include <3d/dag_texStreamingContext.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_TMatrix.h>
#include <EASTL/optional.h>
#include <generic/dag_functionRef.h>


class Occlusion;

namespace rendinst
{

// Used for shadow visibility check: if the objects shadow cannot be
// possibly seen with the current sun direction and camera frustum,
// it is not rendered into the shadowmap.
void setDirFromSun(const Point3 &d);

extern RiGenVisibility *createRIGenVisibility(IMemAlloc *mem);
extern void setRIGenVisibilityDistMul(RiGenVisibility *visibility, float dist_mul);
extern void destroyRIGenVisibility(RiGenVisibility *visibility);
// releases buffer capacity retained from the last prepare (e.g. on level unload);
// the visibility stays valid and is regrown by the next prepare
extern void shrinkRIGenVisibility(RiGenVisibility *visibility);
extern void setRIGenVisibilityMinLod(RiGenVisibility *visibility, int ri_lod, int ri_extra_lod);
extern void setRIGenVisibilityForcedLodRangeCull(RiGenVisibility *visibility, bool range_cull);
extern void setRIForcedLocalPoolOrder(RiGenVisibility *visibility, bool forced_local_pool_order);
extern void setRIGenVisibilityAtestSkip(RiGenVisibility *visibility, bool skip_atest, bool skip_noatest);
bool isRiGenVisibilityForcedLodLoaded(const RiGenVisibility *visibility);
void riGenVisibilityScheduleForcedLodLoading(const RiGenVisibility *visibility);
bool isRiGenVisibilityResLoadingFinished(const RiGenVisibility *visibility);

extern void setRIGenVisibilityRendering(RiGenVisibility *visibility, VisibilityRenderingFlags r);
// opt-in for prefetching lods of destroyed models (_destr) of visible riExtra; expected to be
// set for the main view only, other views render the same riExtra from unrelated distances
extern void setRIGenVisibilityRequestDestrLods(RiGenVisibility *visibility, bool request);


// borrowed for the call only, so the callable must outlive it
using VisibilityExternalFilter = dag::FunctionRef<bool(vec4f bbmin, vec4f bbmax) const>;
using VisibilityTileExternalFilter = dag::FunctionRef<int(vec4f bbmin, vec4f bbmax) const>;
using VisibilityExternalIdFilter = dag::FunctionRef<bool(int ri_idx, const TMatrix &tm) const>;

struct PrepareRiGenVisibilityParams
{
  rendinst::LayerFlags layerFlags = {};
  Point3 viewPos = Point3::ZERO;
  Occlusion *occlusion = nullptr;
  const bbox3f *tileCullBox = nullptr;
  VisibilityExternalFilter externalFilter;
  bool forShadow = false;
  bool forVisualCollision = false;
};
// prepares visibility for specified frustum/position.
// if forShadow is true, only opaque part will be created, and only partially transparent cells are added to opaque
// returns false if nothing is visible
//  if for_visual_collision is true, only rendinst without collision will be rendered
bool prepareRIGenVisibility(RiGenVisibility *vis, const Frustum &frustum, const PrepareRiGenVisibilityParams &params);
inline bool prepareRIGenVisibility(const Frustum &frustum, const Point3 &view_pos, RiGenVisibility *visibility, bool for_shadow,
  Occlusion *occlusion, bool for_visual_collision = false, VisibilityExternalFilter external_filter = {})
{
  return prepareRIGenVisibility(visibility, frustum,
    PrepareRiGenVisibilityParams{.viewPos = view_pos,
      .occlusion = occlusion,
      .externalFilter = external_filter,
      .forShadow = for_shadow,
      .forVisualCollision = for_visual_collision});
}

extern void filterRIGenVisibilityById(const RiGenVisibility *visibility, RiGenVisibility *filteredVis,
  const VisibilityExternalIdFilter &id_filter);

extern void sortRIGenVisibility(RiGenVisibility *visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
  float horizontalFov, float areaThreshold, unsigned renderMaskO);


enum class RiExtraCullIntention
{
  MAIN,
  DRAFT_DEPTH,
  REFLECTIONS,
  LANDMASK,
};

struct PrepareRiexVisibilityParams
{
  const Point3 &viewPos = Point3::ZERO;
  eastl::optional<IPoint2> target;
  RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN;
  Occlusion *occlusion = nullptr;
  const bbox3f *tileCullBox = nullptr;
  VisibilityExternalFilter externalFilter;
  VisibilityTileExternalFilter tileExternalFilter;
  float minSizeToDistRatio = -1.f;
  bool forShadow = false; // a piece of a tall combined rendinst skips externalFilter
  bool forVisualCollision = false;
  bool filterRendinstClipmap = false;
  bool forVsm = false;
  bool filterPreciseBBox = false;
  bool disableRegionPrefilter = false;
  // Bypasses forced occlusion->getCurViewProj() in dev build.
  bool useExactGlobtm = false;
  bool forceCullUnderwaterOnly = false;
};
bool prepareRIGenExtraVisibility(RiGenVisibility &v, mat44f_cref gtm, const PrepareRiexVisibilityParams &params);
inline bool prepareRIGenExtraVisibility(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow,
  Occlusion *occlusion, eastl::optional<IPoint2> target = {}, RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN,
  bool for_visual_collision = false, bool filter_rendinst_clipmap = false, bool for_vsm = false,
  VisibilityExternalFilter external_filter = {}, bool filter_precise_bbox = false)
{
  return prepareRIGenExtraVisibility(v, gtm,
    {.viewPos = viewPos,
      .target = target,
      .cullIntention = cullIntention,
      .occlusion = occlusion,
      .externalFilter = external_filter,
      .forShadow = forShadow,
      .forVisualCollision = for_visual_collision,
      .filterRendinstClipmap = filter_rendinst_clipmap,
      .forVsm = for_vsm,
      .filterPreciseBBox = filter_precise_bbox});
}
bool prepareRIGenExtraVisibilityBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist, RiGenVisibility &vbase,
  bbox3f *result_box = nullptr);
bool prepareRIGenExtraVisibilityForGrassifyBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist,
  RiGenVisibility &vbase, bbox3f *result_box = nullptr);
bool prepareRIGenExtraVisibilityBoxForRIClipmapBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist,
  RiGenVisibility &vbase, bbox3f *result_box = nullptr);
bool prepareRIGenExtraVisibilityBoxInternal(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist, bool filter_grassify,
  bool filter_ri_clipmap, RiGenVisibility &vbase, bbox3f *result_box);
void filterVisibility(RiGenVisibility &from, RiGenVisibility &to, VisibilityExternalFilter external_filter);
void filterRIGenExtraVisibilityById(const RiGenVisibility *visibility, RiGenVisibility *filteredVis,
  const VisibilityExternalIdFilter &id_filter);

void requestRiExtraLodsByDistance(const Point3 &view_pos);

void sortTransparentRiExtraInstancesByDistance(RiGenVisibility *vb, const Point3 &view_pos);
void sortTransparentRiExtraInstancesByDistanceAndPartitionOutsideSphere(RiGenVisibility *vb, const Point3 &view_pos,
  const Point4 &sphere_pos_rad);
void sortTransparentRiExtraInstancesByDistanceAndPartitionInsideSphere(RiGenVisibility *vb, const Point3 &view_pos,
  const Point4 &sphere_pos_rad);

struct RiVisibilityDrawStats
{
  int drawCalls = 0;
  int submittedInstances = 0;
  int triangles = 0;
  int visibleInstances = 0;
};
RiVisibilityDrawStats getRIGenVisibleDrawStats(const RiGenVisibility *visibility, int start_stage, int end_stage);
RiVisibilityDrawStats getRIGenExtraVisibleDrawStats(const RiGenVisibility *visibility, int start_stage, int end_stage);

} // namespace rendinst
