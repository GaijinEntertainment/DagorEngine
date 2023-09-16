//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathAng.h>
#include <math/dag_math3d.h>

namespace pathfinder
{
static inline float tilecache_calc_ri_obstacle_padding(float walkableHeight)
{
  // We would like to use whole 'walkableHeight' for obstacle y padding, but
  // the thing is that obstacle can be tilted, so it's not necessarily unwalkable
  // when distance between its aabb and ground is <= walkableHeight. So we just use
  // half of this distance, it turns out to be pretty safe, most of the RI obstacles
  // are on ground anyway (e.g. fences, haystacks, etc.).
  return walkableHeight * 0.5f;
}

static inline bool tilecache_ri_obstacle_too_low(float unpadded_aabb_height, float walkable_climb)
{
  // Both editor and the game need to agree on what a 'too low' is. Obstacles that can be stepped on
  // are considered too low and are excluded.
  return unpadded_aabb_height < walkable_climb;
}

static inline void tilecache_apply_obstacle_padding(float cell_size, const Point2 &padding, Point3 &center, Point3 &ext)
{
  ext.x += padding.x;
  ext.z += padding.x;
  ext.y += padding.y * 0.5f;
  center.y -= padding.y * 0.5f;

  // Always add cell size, otherwise resulting navmesh might pass through
  // obstacle AABB.
  ext.x += cell_size;
  ext.z += cell_size;
}

static inline void tilecache_calc_obstacle_pos(const TMatrix &tm, const BBox3 &oobb, float cell_size, const Point2 &padding,
  Point3 &center, Point3 &ext, float &yRadians)
{
  // Note! It's particularly important to keep caclulation of obstacle's AABB and angle
  // the same in riMgrServiceAces.cpp RendinstVertexDataCb::processCollision and tilecache_start. All stationary obstacles
  // are baked into navmesh tiles on build, so we don't need to re-build anything at runtime, this makes level loading
  // fast. But we must make sure that obstacles that are created at runtime match the "holes" in navmesh, otherwise
  // we'll get obstacles that are a bit off from "holes" in navmesh.
  center = tm * oobb.center();
  Point3 eulerAngles;
  matrix_to_euler(tm, eulerAngles.y, eulerAngles.z, eulerAngles.x);
  TMatrix finalTm = tm;
  finalTm.setcol(3, ZERO<Point3>());
  finalTm = rotyTM(eulerAngles.y) * finalTm;
  finalTm.setcol(3, tm.getcol(3));
  ext = (finalTm * oobb).width() * 0.5f;
  tilecache_apply_obstacle_padding(cell_size, padding, center, ext);
  yRadians = eulerAngles.y;
}

struct LadderInfo
{
  Point3 from, to;
  Point3 base, forw;
  float hmin, hmax;
};
static inline void tilecache_calc_ladder_info(LadderInfo &out_info, mat44f_cref ladder_tm, float from_dist, float to_dist)
{
  const float pt1[] = {0.0f, -0.5f, 0.0f, 0.0f};
  const float pt2[] = {0.0f, 0.5f, 0.0f, 0.0f};
  vec4f from = v_mat44_mul_vec3p(ladder_tm, v_ldu(pt1));
  vec4f to = v_mat44_mul_vec3p(ladder_tm, v_ldu(pt2));
  out_info.from.set(v_extract_x(from), v_extract_y(from), v_extract_z(from));
  out_info.to.set(v_extract_x(to), v_extract_y(to), v_extract_z(to));
  out_info.base.set(v_extract_x(ladder_tm.col3), v_extract_y(ladder_tm.col3), v_extract_z(ladder_tm.col3));
  out_info.forw.set(v_extract_x(ladder_tm.col0), v_extract_y(ladder_tm.col0), v_extract_z(ladder_tm.col0));
  Point3 into(v_extract_x(ladder_tm.col0), 0.f, v_extract_z(ladder_tm.col0));
  into.normalize();
  out_info.from -= into * from_dist;
  out_info.to += into * to_dist;
  out_info.hmin = v_extract_y(from);
  out_info.hmax = v_extract_y(to);
}

enum
{
  OVERLINK_LADDER = 0,  // ladders (climb cost)
  OVERLINK_BRIDGE = 1,  // bridges (length-cost), two-way
  OVERLINK_JUMPOFF = 2, // jump-offs (low cost), one-way
  OVERLINK_WARP = 3     // warps (no cost/instant), one-way or two-way
  // NB: please do not change/add new fields inside this enum, hardcoded 2-bit
};
}; // namespace pathfinder
