//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/visibilityDecl.h>

#include <3d/dag_texStreamingContext.h>
#include <math/dag_frustum.h>
#include <EASTL/fixed_function.h>


class Occlusion;

namespace rendinst
{

extern RiGenVisibility *createRIGenVisibility(IMemAlloc *mem);
extern void setRIGenVisibilityDistMul(RiGenVisibility *visibility, float dist_mul);
extern void destroyRIGenVisibility(RiGenVisibility *visibility);
extern void setRIGenVisibilityMinLod(RiGenVisibility *visibility, int ri_lod, int ri_extra_lod);
extern void setRIGenVisibilityAtestSkip(RiGenVisibility *visibility, bool skip_atest, bool skip_noatest);
bool isRiGenVisibilityLodsLoaded(const RiGenVisibility *visibility);

extern void setRIGenVisibilityRendering(RiGenVisibility *visibility, VisibilityRenderingFlags r);


using VisibilityExternalFilter = eastl::fixed_function<sizeof(vec4f), bool(vec4f bbmin, vec4f bbmax)>;

// prepares visibility for specified frustum/position.
// if forShadow is true, only opaque part will be created, and only partially transparent cells are added to opaque
// returns false if nothing is visible
//  if for_visual_collision is true, only rendinst without collision will be rendered
extern bool prepareRIGenVisibility(const Frustum &frustum, const Point3 &viewPos, RiGenVisibility *, bool forShadow,
  Occlusion *occlusion, bool for_visual_collision = false, const VisibilityExternalFilter &external_filter = {});

extern void sortRIGenVisibility(RiGenVisibility *visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
  float horizontalFov, float areaThreshold);


enum class RiExtraCullIntention
{
  MAIN,
  DRAFT_DEPTH,
  REFLECTIONS,
  LANDMASK,
};

template <bool use_external_filter = false>
bool prepareExtraVisibilityInternal(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow, Occlusion *occlusion,
  RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN, bool for_visual_collision = false,
  bool filter_rendinst_clipmap = false, bool for_vsm = false, const VisibilityExternalFilter &external_filter = {});

bool prepareRIGenExtraVisibility(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow, Occlusion *occlusion,
  RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN, bool for_visual_collision = false,
  bool filter_rendinst_clipmap = false, bool for_vsm = false, const VisibilityExternalFilter &external_filter = {});
bool prepareRIGenExtraVisibilityBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist, RiGenVisibility &vbase,
  bbox3f *result_box = nullptr);

void sortTransparentRiExtraInstancesByDistance(RiGenVisibility *vb, const Point3 &view_pos);

} // namespace rendinst
