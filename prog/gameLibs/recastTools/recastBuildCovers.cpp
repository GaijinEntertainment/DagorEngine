// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastBuildCovers.h>

#include <EASTL/sort.h>

#include <math/dag_bounds2.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_mathUtils.h>

namespace recastbuild
{
void build_covers_v1(Tab<covers::Cover> &out_covers, const BBox3 &tile_box, const Tab<Edge> &edges, const CoversParams &covParams,
  const rcCompactHeightfield *chf, const rcHeightfield *solid, RenderEdges *out_render_edges);

void build_covers_v2(Tab<covers::Cover> &out_covers, const BBox3 &tile_box, const Tab<Edge> &edges, const CoversParams &covParams,
  const rcCompactHeightfield *place_chf, const rcHeightfield *place_solid, const rcCompactHeightfield *trace_chf,
  const rcHeightfield *trace_solid, RenderEdges *out_render_edges);

int make_fit_cover_flags_v2(const covers::Cover &cover, const CoversParams &covParams);

int try_fit_cover_v2(float &max_volume, covers::Cover &cover, int cover_flags, const covers::Cover &by_cover,
  const CoversParams &covParams);

static int make_fit_cover_flags(const covers::Cover &cover, const CoversParams &covParams)
{
  switch (covParams.typeGen)
  {
    case COVERS_TYPEGEN_ALT1: return make_fit_cover_flags_v2(cover, covParams);
  }
  return 0;
}

static int try_fit_cover(float &max_vol, covers::Cover &cover, int cover_flags, const covers::Cover &by_cover,
  const CoversParams &covParams)
{
  switch (covParams.typeGen)
  {
    case COVERS_TYPEGEN_ALT1: return try_fit_cover_v2(max_vol, cover, cover_flags, by_cover, covParams);
  }
  return 0;
}
} // namespace recastbuild

void recastbuild::build_covers(Tab<covers::Cover> &out_covers, const BBox3 &tile_box, const Tab<Edge> &edges,
  const CoversParams &covParams, const rcCompactHeightfield *place_chf, const rcHeightfield *place_solid,
  const rcCompactHeightfield *trace_chf, const rcHeightfield *trace_solid, RenderEdges *out_render_edges)
{
  if (!covParams.enabled)
    return;

  switch (covParams.typeGen)
  {
    case COVERS_TYPEGEN_BASIC:
      build_covers_v1(out_covers, tile_box, edges, covParams, place_chf, place_solid, out_render_edges);
      break;
    case COVERS_TYPEGEN_ALT1:
      build_covers_v2(out_covers, tile_box, edges, covParams, place_chf, place_solid, trace_chf, trace_solid, out_render_edges);
      break;
  }
}

void recastbuild::fit_intersecting_covers(Tab<covers::Cover> &covers, int num_fixed, const BBox2 &nav_area,
  const CoversParams &covParams)
{
  eastl::sort(covers.begin(), covers.end(), [](const auto &cover1, const auto &cover2) {
    const bool shoot1 = cover1.hasLeftPos || cover1.hasRightPos;
    const bool shoot2 = cover2.hasLeftPos || cover2.hasRightPos;
    const bool zero1 = cover1.shootLeft == ZERO<Point3>() && cover1.shootRight == ZERO<Point3>();
    const bool zero2 = cover2.shootLeft == ZERO<Point3>() && cover2.shootRight == ZERO<Point3>();
    const int value1 = shoot1 ? 1 : (!zero1 ? 2 : 0); // non-zero are half covers (most important)
    const int value2 = shoot2 ? 1 : (!zero2 ? 2 : 0); // non-zero are half covers (most important)
    if (value1 == value2)
    {
      const float lenSq1 = lengthSq(cover1.groundLeft - cover1.groundRight);
      const float lenSq2 = lengthSq(cover2.groundLeft - cover2.groundRight);
      return lenSq1 > lenSq2; // sort long to short, to avoid removals of long by short
    }
    return value1 > value2;
  });

  const float cosThreshold = cosf(DegToRad(25));
  const float volumePercentageThreshold = 0.4f;
  bbox3f oobb{v_make_vec4f(-0.5f, -0.5f, -0.5f, 1.f), v_make_vec4f(0.5f, 0.5f, 0.5f, 1.f)};
  BBox3 coverBox(as_point3(&oobb.bmin), as_point3(&oobb.bmax));

  Tab<Tab<uint32_t>> coversGridIndices;
  constexpr int numGridSplits = 32;
  coversGridIndices.resize(numGridSplits * numGridSplits);
  const Point2 gridBase = nav_area[0];
  const Point2 gridStep = nav_area.width() / numGridSplits;
  const Point2 gridInvStep(safeinv(gridStep.x), safeinv(gridStep.y));
  Point3_vec4 coverCenter_s;
  for (int coverIdx = 0; coverIdx < covers.size(); ++coverIdx)
  {
    auto &cover = covers[coverIdx];

    mat44f coverTm = cover.calcTm();
    mat44f invCoverTm;
    v_mat44_inverse43(invCoverTm, coverTm);

    v_st(&coverCenter_s.x, coverTm.col3);
    int gridX = clamp(int((coverCenter_s.x - gridBase.x) * gridInvStep.x), 0, numGridSplits - 1);
    int gridY = clamp(int((coverCenter_s.z - gridBase.y) * gridInvStep.y), 0, numGridSplits - 1);
    uint32_t gridIdx = gridY * numGridSplits + gridX;

    if (coverIdx < num_fixed)
    {
      coversGridIndices[gridIdx].push_back(coverIdx);
      continue;
    }

    // Since we may use "extended" tile for building covers we need to remove
    // excess covers here. Just checking intersection percentage works fine.
    bool foundIntersecting = false;
    const int coverFlags = recastbuild::make_fit_cover_flags(cover, covParams);
    for (int gy = max(gridY - 1, 0); gy <= min(gridY + 1, numGridSplits - 1); ++gy)
    {
      for (int gx = max(gridX - 1, 0); gx <= min(gridX + 1, numGridSplits - 1); ++gx)
      {
        for (uint32_t covIdx : coversGridIndices[gy * numGridSplits + gx])
        {
          const covers::Cover &existingCover = covers[covIdx];
          if (dot(Point3::x0z(existingCover.dir), Point3::x0z(cover.dir)) < cosThreshold)
            continue;
          mat44f tm;
          v_mat44_mul43(tm, invCoverTm, existingCover.calcTm());
          bbox3f aabb;
          v_bbox3_init(aabb, tm, oobb);
          BBox3 existingCoverBox;
          v_stu_bbox3(existingCoverBox, aabb);
          // existingCoverBox is AABB in frame of coverBox and since they're oriented
          // similarly (i.e. with cosThreshold) they can both be treated as AABBs and
          // intersection can be calculated, it'll be pretty close to actual intersection
          // in world.

          Point3 s1 = coverBox.width();
          Point3 s2 = existingCoverBox.width();
          float minVolume = min(s1.x * s1.y * s1.z, s2.x * s2.y * s2.z);
          s1 = coverBox.getIntersection(existingCoverBox).width();
          float isectVolume = s1.x * s1.y * s1.z;

          float isectMaxVolume = 0.001f;
          const auto result = recastbuild::try_fit_cover(isectMaxVolume, cover, coverFlags, existingCover, covParams);
          if (result < 0)
            continue;
          if (result > 0)
          {
            foundIntersecting = true;
            break;
          }
          if ((isectVolume > isectMaxVolume) && ((isectVolume / minVolume) > volumePercentageThreshold))
          {
            // Cut covers that intersect too much.
            foundIntersecting = true;
            break;
          }
        }
      }
    }

    if (!foundIntersecting)
      coversGridIndices[gridIdx].push_back(coverIdx);
    else
    {
      covers[coverIdx] = covers.back();
      covers.pop_back();
      --coverIdx;
    }
  }
}
