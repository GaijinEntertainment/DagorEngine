//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_traceRayTriangle.h>
#include <math/dag_wooray2d.h>
#include <math/dag_math2d.h>
#include <math/dag_bounds3.h>   // BBox3
#include <math/dag_mathUtils.h> // v_ldu_bbox3
#include <util/dag_compilerDefs.h>
#include <EASTL/type_traits.h>


// Supplementary class used for heightmap raytracing.

// Trace ray through heightmap with cells represented as 4-triangle 5-point geometry,
// with 4 points at grid vertices and 1 at the cell center.
//
// Class HM must implement following methods:
// real getHeightmapCellSize();
// bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid);
template <class T>
struct HmapGetMinMax
{
  template <typename U = T>
  static int SFINAE(decltype(&U::getHeightmapCell5PtMinMax));
  template <typename U = T>
  static char SFINAE(...);

  template <typename U = T>
  static int SFINAE2(decltype(&U::getWorldBox));
  template <typename U = T>
  static char SFINAE2(...);

  static constexpr bool has_method = sizeof(SFINAE<T>(nullptr)) == sizeof(int);
  static constexpr bool has_world_box = sizeof(SFINAE2<T>(nullptr)) == sizeof(int);

  template <typename U = T>
  static eastl::enable_if_t<HmapGetMinMax<U>::has_method, bool> getHeightmapCell5PtMinMax(const T &hm, const IPoint2 &cell, float &h0,
    float &hx, float &hy, float &hxy, float &med, float minY, float maxY)
  {
    return hm.getHeightmapCell5PtMinMax(cell, h0, hx, hy, hxy, med, minY, maxY);
  }
  template <typename U = T>
  static eastl::enable_if_t<!HmapGetMinMax<U>::has_method, bool> getHeightmapCell5PtMinMax(const T &hm, const IPoint2 &cell, float &h0,
    float &hx, float &hy, float &hxy, float &med, float minY, float maxY)
  {
    if (!hm.getHeightmapCell5Pt(cell, h0, hx, hy, hxy, med))
      return false;
    return max(max(h0, hx), max(hy, hxy)) >= minY && min(min(h0, hx), min(hy, hxy)) <= maxY;
  }

  template <typename U = T>
  static eastl::enable_if_t<HmapGetMinMax<U>::has_world_box, bbox3f> getWorldBox(const T &hm)
  {
    BBox3 bs = hm.getWorldBox();
    return v_ldu_bbox3(bs);
  }
  template <typename U = T>
  static eastl::enable_if_t<!HmapGetMinMax<U>::has_world_box, bbox3f> getWorldBox(const T &hm)
  {
    const float cellSize = hm.getHeightmapCellSize();
    const Point3 ofs = hm.getHeightmapOffset();
    Point2 world(cellSize * hm.getHeightmapSizeX(), cellSize * hm.getHeightmapSizeY());
    return v_ldu_bbox3(BBox3{ofs - Point3(0, 100000, 0), ofs + Point3(world.x, 100000, world.y)});
  }
};

// cells the XZ walk can enter over dir * [0, mint]: its crossings, both end cells and the last piece
inline int hml_cell_walk_limit(const Point3 &dir, real mint, real cell_size)
{
  return (int)ceilf((fabsf(dir.x) + fabsf(dir.z)) * mint / cell_size) + 3;
}

// The cell's four triangles as the kernel's rows, in the heightmap's own frame: p0 is the centre, and triangle k runs from
// corner k in p1 to corner k + 1 in p2, round the corners (x0, z0), (x0, z1), (x1, z1), (x1, z0), so it lies left, top, right, bottom.
template <class HM>
inline bool heightmap_cell_triangles(const HM &heightmap, const IPoint2 &currentCell, float cellSize, float rayMinY, float rayMaxY,
  mat43f &p0, mat43f &p1, mat43f &p2)
{
  real h0, hx, hy, hxy, hmid;
  if (!HmapGetMinMax<HM>::getHeightmapCell5PtMinMax(heightmap, currentCell, h0, hx, hy, hxy, hmid, rayMinY, rayMaxY))
    return false;
  const float x0 = currentCell.x * cellSize, x1 = (currentCell.x + 1) * cellSize;
  const float z0 = currentCell.y * cellSize, z1 = (currentCell.y + 1) * cellSize;
  const float halfCellSize = 0.5f * cellSize;
  p0.row0 = v_splats(x0 + halfCellSize);
  p0.row1 = v_splats(hmid);
  p0.row2 = v_splats(z0 + halfCellSize);
  p1.row0 = v_make_vec4f(x0, x0, x1, x1);
  p1.row1 = v_make_vec4f(h0, hy, hxy, hx);
  p1.row2 = v_make_vec4f(z0, z1, z1, z0);
  p2.row0 = v_rot_1(p1.row0);
  p2.row1 = v_rot_1(p1.row1);
  p2.row2 = v_rot_1(p1.row2);
  return true;
}

template <class HM>
inline bool trace_heightmap_cell(const HM &heightmap, const IPoint2 &currentCell, float cellSize, float rayMinY, float rayMaxY,
  vec3f pt, vec3f dir, real &mint, Point3 *normal, bool cull)
{
  mat43f p0, p1, p2;
  if (!heightmap_cell_triangles<HM>(heightmap, currentCell, cellSize, rayMinY, rayMaxY, p0, p1, p2))
    return false;
  const int hit = traceray4Triangles(pt, dir, mint, p0, p1, p2, !cull);
  if (hit < 0)
    return false;
  if (normal)
  {
    mat43f e1, e2, n;
    v_mat43_sub(e1, p1, p0);
    v_mat43_sub(e2, p2, p0);
    v_mat43_cross(n, e1, e2);
    alignas(16) float nx[4], ny[4], nz[4];
    v_st(nx, n.row0);
    v_st(ny, n.row1);
    v_st(nz, n.row2);
    v_stu_p3(&normal->x, v_norm3(v_make_vec4f(nx[hit], ny[hit], nz[hit], 0)));
  }
  return true;
}

static constexpr int8_t HMAP_CROSSING_ENTER = 1, HMAP_CROSSING_LEAVE = -1;

// mint is a ceiling here, not shrunk: each of the four triangles answers for itself. Reported in t order, so the stream never
// goes back inside a cell, and only where the ground point lies inside the triangle: the kernel accepts a hit a little past
// an edge, and that point is the neighbour's to answer.
template <class HM, typename CrossingCb>
inline void trace_heightmap_cell_all_crossings(const HM &heightmap, const IPoint2 &currentCell, float cellSize, float rayMinY,
  float rayMaxY, vec3f pt, vec3f dir, real mint, real t_ofs, real tie_tol, CrossingCb cb)
{
  mat43f p0, p1, p2;
  if (!heightmap_cell_triangles<HM>(heightmap, currentCell, cellSize, rayMinY, rayMaxY, p0, p1, p2))
    return;
  vec4f t4 = v_splats(mint);
  const vec4f hit = traceray4TrianglesVecMask(pt, dir, t4, p0, p1, p2, true);
  if (v_check_xyzw_all_false(hit))
    return;
  mat43f e1, e2, n, d, c, r;
  v_mat43_sub(e1, p1, p0);
  v_mat43_sub(e2, p2, p0);
  v_mat43_cross(n, e1, e2);
  d.row0 = v_splat_x(dir), d.row1 = v_splat_y(dir), d.row2 = v_splat_z(dir);
  const vec4f nd = v_dot(n, d);
  const int enter = v_truemask(v_cmp_gt(v_zero(), nd));
  // The kernel's own weights, from the ray and the corners; a hit point would round at the map's scale. The band decides the
  // centre-to-corner edges; on the grid edge the kernel's tighter relative bound, its TRACE_EPSILON, decides first.
  v_mat43_sub_vec4f(c, p0, pt);
  v_mat43_cross(r, d, c);
  const vec4f flip = v_and(nd, v_msbit()), area = v_abs(nd);
  const vec4f u = v_xor(v_dot(r, e1), flip), v = v_xor(v_neg(v_dot(r, e2)), flip);
  const int kept = v_truemask(v_and(hit, v_cmp_ge(v_min(v_min(u, v), v_sub(v_sub(area, u), v)), v_mul(v_splats(-1e-5f), area))));
  const float edgeTol = 0.5f * 4e-6f * cellSize; // how far past its grid edge that bound keeps a point, so a neighbour keeps it too
  alignas(16) float t[4];
  v_st(t, t4);
  int reported = 0;
  float keptT[4];
  int keptTri[4];
  for (int i = 0; i < 4; ++i)
  {
    if (!(kept & (1 << i)))
      continue;
    int k = reported++; // triangle order is not t order
    for (; k > 0 && keptT[k - 1] > t[i]; --k)
    {
      keptT[k] = keptT[k - 1];
      keptTri[k] = keptTri[k - 1];
    }
    keptT[k] = t[i];
    keptTri[k] = i;
  }
  const auto facingOf = [&](int tri) DAGOR_FORCE_INLINE_LAMBDA -> int8_t {
    return tri < 0 ? 0 : (enter >> tri) & 1 ? HMAP_CROSSING_ENTER : HMAP_CROSSING_LEAVE;
  };
  // the triangle the ground point lies in at t, by its quadrant about the centre; -1 farther than reach from the centre
  const auto triangleAt = [&](float at, float reach) DAGOR_FORCE_INLINE_LAMBDA {
    const float dx = v_extract_x(dir) * at - v_extract_x(c.row0);
    const float dz = v_extract_z(dir) * at - v_extract_x(c.row2);
    if (max(fabsf(dx), fabsf(dz)) > reach)
      return -1;
    return fabsf(dx) >= fabsf(dz) ? (dx < 0.f ? 0 : 2) : (dz > 0.f ? 1 : 3);
  };
  // A vertex or an edge answers once per triangle at one t. The ray crosses there only where the triangle it leaves and the one
  // it goes into agree. A side past the cell is the next cell's to say only for a point on the cell's edge, which it keeps too.
  for (int i = 0, j = 0; i < reported; i = j)
  {
    for (j = i + 1; j < reported && keptT[j] - keptT[i] <= tie_tol;)
      ++j;
    int8_t facing = facingOf(keptTri[i]);
    if (j - i > 1)
    {
      const float reach = triangleAt(keptT[i], 0.5f * cellSize - edgeTol) < 0 ? 0.5f * cellSize : cellSize;
      const int8_t before = facingOf(triangleAt(keptT[i] - 10.f * tie_tol, reach));
      const int8_t after = facingOf(triangleAt(keptT[i] + 10.f * tie_tol, reach));
      facing = before && after ? (before == after ? before : 0) : int8_t(before | after);
    }
    if (facing)
      cb(keptT[i] + t_ofs, facing);
  }
}

// The one clip of a ray to the world box for every walker: mint shrinks to the
// box, and the walk starts rayTBoxMin along the ray. False when the ray misses it.
template <class HM>
inline bool heightmap_clip_ray(const HM &heightmap, vec3f rayStart, vec3f rayDir, real &mint, float &rayTBoxMin)
{
  rayTBoxMin = 0;
  bbox3f worldBox = HmapGetMinMax<HM>::getWorldBox(heightmap);
  bbox3f rayBox;
  v_bbox3_init_by_ray(rayBox, rayStart, rayDir, v_splats(mint));
  if (v_bbox3_test_box_inside(worldBox, rayBox))
    return true;
  float minMaxT[2];
  v_stu_half(minMaxT, v_ray_box_intersect_dist(worldBox.bmin, worldBox.bmax, rayStart, rayDir, v_zero()));
  if (minMaxT[0] > mint || minMaxT[0] > minMaxT[1])
    return false;
  // ray intersects box if tmax >= max(ray.tmin, tmin) && tmin <= ray.tmax
  rayTBoxMin = max(minMaxT[0] - 1e-3f, 0.f);
  mint = min(mint, minMaxT[1] + 1e-3f) - rayTBoxMin;
  return true;
}

// The one 2D cell walk of the heightmap walkers. body(cell, rayMinY, rayMaxY) answers true to stop, or shrinks mint to end it there.
// Returns 1 when the body stopped it, -1 when the ray ended inside the budget, a shrunk mint included, 0 when the budget ran out;
// nextT carries on, so a second walk can continue from the same ray.
template <class HM, typename Body>
inline int heightmap_walk_cells(const HM &heightmap, WooRay2dInf &ray, const Point3 &pt, const Point3 &dir, const real &mint,
  int steps, float &nextT, Body body)
{
  float curRayY = dir.y * nextT;
  for (; steps > 0; --steps)
  {
    const IPoint2 currentCell = ray.currentCell();
    ray.nextCell(nextT);
    bool last = false;
    if (nextT >= mint) // the walk is 2D and mint is 3D
    {
      nextT = mint;
      last = true;
    }
    const float prevRayY = curRayY;
    curRayY = dir.y * nextT;
    if (uint32_t(currentCell.x) < uint32_t(heightmap.getHeightmapSizeX()) &&
        uint32_t(currentCell.y) < uint32_t(heightmap.getHeightmapSizeY()))
    {
      const float rayMinY = pt.y + min(curRayY, prevRayY), rayMaxY = pt.y + max(curRayY, prevRayY);
      if (body(currentCell, rayMinY, rayMaxY))
        return 1;
    }
    if (last)
      return -1;
  }
  return 0;
}

template <class HM>
inline bool trace_ray_midpoint_heightmap(const HM &heightmap, const Point3 &pt_, const Point3 &dir, real &mint_, Point3 *normal,
  bool cull = true)
{
  float rayTBoxMin, mint = mint_;
  vec3f rayStart = v_ldu_p3_safe(&pt_.x), rayDir = v_ldu_p3_safe(&dir.x);
  if (!heightmap_clip_ray(heightmap, rayStart, rayDir, mint, rayTBoxMin))
    return false;
  const Point3 pt = pt_ + dir * rayTBoxMin - heightmap.getHeightmapOffset();
  const vec3f ptv = v_ldu_p3_safe(&pt.x);

  real cellSize = heightmap.getHeightmapCellSize();

  bool wasHit = false;
  float nextT = 0;
  WooRay2dInf ray;
  ray.init(Point2::xz(pt), Point2::xz(dir), Point2(cellSize, cellSize));
  heightmap_walk_cells(heightmap, ray, pt, dir, mint, hml_cell_walk_limit(dir, mint, cellSize), nextT,
    [&](const IPoint2 &cell, float rayMinY, float rayMaxY) DAGOR_FORCE_INLINE_LAMBDA {
      wasHit |= trace_heightmap_cell(heightmap, cell, cellSize, rayMinY, rayMaxY, ptv, rayDir, mint, normal, cull);
      return false;
    });
  if (wasHit)
    mint_ = mint + rayTBoxMin;
  return wasHit;
}

// cb(t, facing) gets every crossing of the surface in cell order; t runs from pt_ along dir and rises by cell, not strictly.
// Both sides answer, hence no cull flag; facing is +1 where the ray goes into the ground. Returns the number of cb calls.
template <class HM, typename CrossingCb>
inline int trace_all_crossings_heightmap(const HM &heightmap, const Point3 &pt_, const Point3 &dir, real maxt, CrossingCb cb)
{
  float rayTBoxMin, mint = maxt;
  vec3f rayStart = v_ldu_p3_safe(&pt_.x), rayDir = v_ldu_p3_safe(&dir.x);
  if (!heightmap_clip_ray(heightmap, rayStart, rayDir, mint, rayTBoxMin))
    return 0;
  const Point3 pt = pt_ + dir * rayTBoxMin - heightmap.getHeightmapOffset();
  const vec3f ptv = v_ldu_p3_safe(&pt.x);
  const real cellSize = heightmap.getHeightmapCellSize();

  int reported = 0;
  int8_t lastFacing = 0, groupFirst = 0, groupLast = 0;
  float groupT = 0.f;
  const float tieTol = 1e-4f * cellSize / length(dir);
  // Answers within tieTol of the first are one point several cells answer, a cell the walk only touched there among them: the
  // first and the last are the sides the ray leaves and enters, and a repeat of the last crossing is that same crossing.
  const auto flush = [&]() DAGOR_FORCE_INLINE_LAMBDA {
    if (groupFirst && groupFirst == groupLast && groupFirst != lastFacing)
    {
      lastFacing = groupFirst;
      ++reported;
      cb(groupT, groupFirst);
    }
  };
  const auto merge = [&](float t, int8_t facing) DAGOR_FORCE_INLINE_LAMBDA {
    if (groupFirst && t - groupT <= tieTol)
    {
      groupLast = facing;
      return;
    }
    flush();
    groupFirst = groupLast = facing;
    groupT = t;
  };
  float nextT = 0;
  WooRay2dInf ray;
  ray.init(Point2::xz(pt), Point2::xz(dir), Point2(cellSize, cellSize));
  heightmap_walk_cells(heightmap, ray, pt, dir, mint, hml_cell_walk_limit(dir, mint, cellSize), nextT,
    [&](const IPoint2 &cell, float rayMinY, float rayMaxY) DAGOR_FORCE_INLINE_LAMBDA {
      trace_heightmap_cell_all_crossings(heightmap, cell, cellSize, rayMinY, rayMaxY, ptv, rayDir, mint, rayTBoxMin, tieTol, merge);
      return false;
    });
  flush();
  return reported;
}

// Check ray intersection with heightmap cells represented as 4-triangle 5-point geometry,
// with 4 points at grid vertices and 1 at the cell center.
//
// Class HM must implement following methods:
// real getHeightmapCellSize();
// bool getHeightmapHeight(const IPoint2 &cell, real &ht);
// bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid);
template <bool approx, class HM>
inline bool ray_hit_midpoint_heightmap_base(HM &heightmap, const Point3 &pt_, const Point3 &dir, real mint)
{
  float rayTBoxMin;
  vec3f rayStart = v_ldu_p3_safe(&pt_.x), rayDir = v_ldu_p3_safe(&dir.x);
  if (!heightmap_clip_ray(heightmap, rayStart, rayDir, mint, rayTBoxMin))
    return false;

  const Point3 pt = pt_ + dir * rayTBoxMin - heightmap.getHeightmapOffset();
  const vec3f ptv = v_ldu_p3_safe(&pt.x);
  real cellSize = heightmap.getHeightmapCellSize();

  WooRay2dInf ray;
  ray.init(Point2::xz(pt), Point2::xz(dir), Point2(cellSize, cellSize));

  const int num = hml_cell_walk_limit(dir, mint, cellSize);
  float nextT = 0;
  const int precise_steps = approx ? (num < 4 ? num : 4) : num;
  const int walked = heightmap_walk_cells(heightmap, ray, pt, dir, mint, precise_steps, nextT,
    [&](const IPoint2 &cell, float rayMinY, float rayMaxY) DAGOR_FORCE_INLINE_LAMBDA {
      return trace_heightmap_cell(heightmap, cell, cellSize, rayMinY, rayMaxY, ptv, rayDir, mint, nullptr, true);
    });
  if (walked > 0)
    return true;
  if (walked < 0 || !approx)
    return false;
  return heightmap_walk_cells(heightmap, ray, pt, dir, mint, num - precise_steps, nextT,
           [&](const IPoint2 &cell, float rayMinY, float rayMaxY) DAGOR_FORCE_INLINE_LAMBDA {
             real ht;
             return heightmap.getHeightmapHeight(cell, ht) && (rayMinY + rayMaxY) * 0.5f < ht;
           }) > 0;
}

// Checks if ray lies under heightmap cells.
//
// Class HM must implement following methods:
// real getHeightmapCellSize();
// Point3 getHeightmapOffset();
// real getHeightMin();
// real getHeightScaleRaw();
// real getHeightmapHeightUnsafe(const IPoint2 &cell);

template <class HM>
inline bool ray_under_heightmap(HM &heightmap, const Point3 &pt_, const Point3 &dir, real mint)
{
  if (check_nan(pt_.x + pt_.z + dir.x + dir.z + mint + dir.y))
    return false;
  float rayTBoxMin;
  if (!heightmap_clip_ray(heightmap, v_ldu_p3_safe(&pt_.x), v_ldu_p3_safe(&dir.x), mint, rayTBoxMin))
    return false;

  Point3 pt = pt_ + dir * rayTBoxMin - heightmap.getHeightmapOffset();

  real cellSize = heightmap.getHeightmapCellSize();

  IBBox2 hmapCellBox(IPoint2(0, 0), IPoint2(heightmap.getHeightmapSizeX() - 1, heightmap.getHeightmapSizeY() - 1));
  WooRay2dInf ray;
  ray.init(Point2::xz(pt), Point2::xz(dir), Point2(cellSize, cellSize));

  float nextT = 0;
  for (;;)
  {
    IPoint2 currentCell = ray.currentCell();
    if (hmapCellBox & currentCell)
      break;

    ray.nextCell(nextT);
    if (nextT >= mint)
      return false;
  }
  pt.y = safediv(pt.y - heightmap.getHeightMin(), heightmap.getHeightScaleRaw());
  float scaledDirY = safediv(dir.y, heightmap.getHeightScaleRaw());
  scaledDirY *= 0.5;   // remove one multiplication from comparison
  ray.nextCell(nextT); // intentionally do not check first pixel. Src or Dest can not be below heightmap, only due inaccuracy.
  if (nextT >= mint)
    return false;

  for (;;)
  {
    IPoint2 cc = ray.currentCell();
    if (!(hmapCellBox & cc))
      return false;

    uint16_t ht = heightmap.getHeightmapHeightUnsafe(cc);
    float oldT = nextT;
    ray.nextCell(nextT);
    if (nextT >= mint) // intentionally do not check last pixel. Src or Dest can not be below heightmap, only due inaccuracy.
      return false;
    real minRayY = pt.y + scaledDirY * (min(mint, nextT) + oldT); // scaledDirY already multiplied by 0.5
    if ((int)ht > (int)minRayY)
    {
      uint16_t h10 = (uint32_t(cc.x + 1) < hmapCellBox[1].x + 1) ? heightmap.getHeightmapHeightUnsafe(IPoint2(cc.x + 1, cc.y)) : ht;
      uint16_t h01 = (uint32_t(cc.y + 1) < hmapCellBox[1].y + 1) ? heightmap.getHeightmapHeightUnsafe(IPoint2(cc.x, cc.y + 1)) : ht;
      uint16_t h11 = (uint32_t(cc.x + 1) < hmapCellBox[1].x + 1 && uint32_t(cc.y + 1) < hmapCellBox[1].y + 1)
                       ? heightmap.getHeightmapHeightUnsafe(IPoint2(cc.x + 1, cc.y + 1))
                       : ht;
      if ((int)h10 > (int)minRayY && (int)h01 > (int)minRayY && (int)h11 > (int)minRayY)
        return true;
    }
  }
  return false;
}

template <class HM>
inline bool ray_hit_midpoint_heightmap_approximate(HM &heightmap, const Point3 &pt_, const Point3 &dir, real mint)
{
  return ray_hit_midpoint_heightmap_base<true, HM>(heightmap, pt_, dir, mint);
}

template <class HM>
inline bool ray_hit_midpoint_heightmap(HM &heightmap, const Point3 &pt_, const Point3 &dir, real mint)
{
  return ray_hit_midpoint_heightmap_base<false, HM>(heightmap, pt_, dir, mint);
}
