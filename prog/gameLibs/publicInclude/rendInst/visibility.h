//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/visibilityDecl.h>

#include <3d/dag_texStreamingContext.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/fixed_function.h>
#include <EASTL/optional.h>


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
extern void setRIGenVisibilityMinLod(RiGenVisibility *visibility, int ri_lod, int ri_extra_lod);
extern void setRIGenVisibilityAtestSkip(RiGenVisibility *visibility, bool skip_atest, bool skip_noatest);
bool isRiGenVisibilityForcedLodLoaded(const RiGenVisibility *visibility);
void riGenVisibilityScheduleForcedLodLoading(const RiGenVisibility *visibility);

extern void setRIGenVisibilityRendering(RiGenVisibility *visibility, VisibilityRenderingFlags r);


using VisibilityExternalFilter = eastl::fixed_function<sizeof(vec4f), bool(vec4f bbmin, vec4f bbmax)>;

// prepares visibility for specified frustum/position.
// if forShadow is true, only opaque part will be created, and only partially transparent cells are added to opaque
// returns false if nothing is visible
//  if for_visual_collision is true, only rendinst without collision will be rendered
extern bool prepareRIGenVisibility(const Frustum &frustum, const Point3 &viewPos, RiGenVisibility *, bool forShadow,
  Occlusion *occlusion, bool for_visual_collision = false, const VisibilityExternalFilter &external_filter = {});

extern void sortRIGenVisibility(RiGenVisibility *visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
  float horizontalFov, float areaThreshold, unsigned renderMaskO);


enum class RiExtraCullIntention
{
  MAIN,
  DRAFT_DEPTH,
  REFLECTIONS,
  LANDMASK,
};

template <bool use_external_filter = false, bool external_filter_use_bbox = false>
bool prepareExtraVisibilityInternal(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow, Occlusion *occlusion,
  eastl::optional<IPoint2> target = {}, RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN,
  bool for_visual_collision = false, bool filter_rendinst_clipmap = false, bool for_vsm = false,
  const VisibilityExternalFilter &external_filter = {});

bool prepareRIGenExtraVisibility(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow, Occlusion *occlusion,
  eastl::optional<IPoint2> target = {}, RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN,
  bool for_visual_collision = false, bool filter_rendinst_clipmap = false, bool for_vsm = false,
  const VisibilityExternalFilter &external_filter = {}, bool filter_precise_bbox = false);
bool prepareRIGenExtraVisibilityBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist, RiGenVisibility &vbase,
  bbox3f *result_box = nullptr);
bool prepareRIGenExtraVisibilityForGrassifyBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist,
  RiGenVisibility &vbase, bbox3f *result_box = nullptr);
bool prepareRIGenExtraVisibilityBoxInternal(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist, bool filter_grassify,
  RiGenVisibility &vbase, bbox3f *result_box);
void filterVisibility(RiGenVisibility &from, RiGenVisibility &to, const VisibilityExternalFilter &external_filter);

void requestLodsByDistance(const Point3 &view_pos);

void sortTransparentRiExtraInstancesByDistance(RiGenVisibility *vb, const Point3 &view_pos);
void sortTransparentRiExtraInstancesByDistanceAndPartitionOutsideSphere(RiGenVisibility *vb, const Point3 &view_pos,
  const Point4 &sphere_pos_rad);
void sortTransparentRiExtraInstancesByDistanceAndPartitionInsideSphere(RiGenVisibility *vb, const Point3 &view_pos,
  const Point4 &sphere_pos_rad);

} // namespace rendinst
