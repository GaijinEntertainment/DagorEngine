//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_traceRayTriangle.h>
#include <math/dag_wooray2d.h>
#include <math/dag_math2d.h>
#include <math/dag_bounds3.h> // BBox3
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
    bbox3f b;
    b.bmin = v_ldu(&bs[0].x);
    b.bmax = v_ldu(&bs[1].x);
    return b;
  }
  template <typename U = T>
  static eastl::enable_if_t<!HmapGetMinMax<U>::has_world_box, bbox3f> getWorldBox(const T &hm)
  {
    const float cellSize = hm.getHeightmapCellSize();
    const Point3 ofs = hm.getHeightmapOffset();
    Point2 world(cellSize * hm.getHeightmapSizeX(), cellSize * hm.getHeightmapSizeY());
    const Point3 bmn = ofs - Point3(0, 100000, 0), bmx = ofs + Point3(world.x, 100000, world.y);
    bbox3f b;
    b.bmin = v_ldu(&bmn.x);
    b.bmax = v_ldu(&bmx.x);
    return b;
  }
};

template <class HM, bool useMinMax = true>
inline bool trace_heightmap_cell(const HM &heightmap, const IPoint2 &currentCell, float cellSize, float rayMinY, float rayMaxY,
  const Point3 &pt, const Point3 &dir, real &mint, Point3 *normal, bool cull)
{
  real h0, hx, hy, hxy, hmid;

  if (!HmapGetMinMax<HM>::getHeightmapCell5PtMinMax(heightmap, currentCell, h0, hx, hy, hxy, hmid, rayMinY, rayMaxY))
    return false;
  float halfCellSize = 0.5f * cellSize;
  vec4f mid = v_make_vec4f(currentCell.x * cellSize + halfCellSize, hmid, currentCell.y * cellSize + halfCellSize, 0);
  vec4f e0 = v_make_vec4f(-halfCellSize, h0 - hmid, -halfCellSize, 0);
  vec4f e1 = v_make_vec4f(-halfCellSize, hy - hmid, +halfCellSize, 0);
  vec4f e2 = v_make_vec4f(+halfCellSize, hxy - hmid, +halfCellSize, 0);
  vec4f e3 = v_make_vec4f(+halfCellSize, hx - hmid, -halfCellSize, 0);

  vec4f vertices[4][3];
  vertices[0][0] = mid;
  vertices[0][1] = v_add(mid, e0);
  vertices[0][2] = v_add(mid, e1);
  vertices[1][0] = mid;
  vertices[1][1] = v_add(mid, e1);
  vertices[1][2] = v_add(mid, e2);
  vertices[2][0] = mid;
  vertices[2][1] = v_add(mid, e2);
  vertices[2][2] = v_add(mid, e3);
  vertices[3][0] = mid;
  vertices[3][1] = v_add(mid, e3);
  vertices[3][2] = v_add(mid, e0);
  int hit = traceray4Triangles(v_ldu(&pt.x), v_ldu(&dir.x), mint, vertices, 4, !cull);
  if (hit < 0)
    return false;
  if (normal)
  {
    Point3_vec4 n;
    vec4f vn = v_norm3(v_cross3(v_sub(vertices[hit][1], mid), v_sub(vertices[hit][2], mid)));
    v_st(&n.x, vn);
    *normal = n;
  }
  return true;
}

template <class HM>
inline bool trace_ray_midpoint_heightmap(const HM &heightmap, const Point3 &pt_, const Point3 &dir, real &mint_, Point3 *normal,
  bool cull = true)
{
  float rayTBoxMin = 0;
  bbox3f worldBox = HmapGetMinMax<HM>::getWorldBox(heightmap);
  float minMaxT[2], mint = mint_;
  vec3f rayStart = v_ldu(&pt_.x), rayDir = v_ldu(&dir.x);
  bbox3f rayBox;
  v_bbox3_init_by_ray(rayBox, rayStart, rayDir, v_splats(mint));
  if (!v_bbox3_test_box_inside(worldBox, rayBox))
  {
    v_stu_half(minMaxT, v_ray_box_intersect_dist(worldBox.bmin, worldBox.bmax, rayStart, rayDir, v_zero()));
    if (minMaxT[0] > mint || minMaxT[0] > minMaxT[1])
      return false;
    // ray intersects box if tmax >= max(ray.tmin, tmin) && tmin <= ray.tmax
    rayTBoxMin = max(minMaxT[0] - 1e-3f, 0.f);
    mint = min(mint, minMaxT[1] + 1e-3f) - rayTBoxMin;
  }
  const Point3 pt = pt_ + dir * rayTBoxMin - heightmap.getHeightmapOffset();

  real cellSize = heightmap.getHeightmapCellSize();
  real halfCellSize = cellSize * 0.5f;

  Point3 endPt = pt + dir * mint;

  bool wasHit = false;
  float curRayY = 0;
  WooRay2dInf ray;
  ray.init(Point2::xz(pt), Point2::xz(dir), Point2(cellSize, cellSize));
  int n = (int)ceil(4 * mint / cellSize);

  for (; n > 0; --n)
  {
    IPoint2 currentCell = ray.currentCell();
    float nextT;
    ray.nextCell(nextT);
    if (nextT >= mint) // since we traverse 2d ray, but mint is 3d
    {
      nextT = mint;
      n = 0;
    }
    const float prevRayY = curRayY;
    curRayY = dir.y * nextT;
    if (uint32_t(currentCell.x >= heightmap.getHeightmapSizeX()) || uint32_t(currentCell.y >= heightmap.getHeightmapSizeY()))
      continue;
    const float rayMinY = pt.y + min(curRayY, prevRayY), rayMaxY = pt.y + max(curRayY, prevRayY);
    wasHit |= trace_heightmap_cell(heightmap, currentCell, cellSize, rayMinY, rayMaxY, pt, dir, mint, normal, cull);
  }
  if (wasHit)
    mint_ = mint + rayTBoxMin;
  return wasHit;
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
  float rayTBoxMin = 0;
  bbox3f worldBox = HmapGetMinMax<HM>::getWorldBox(heightmap);
  float minMaxT[2];
  vec3f rayStart = v_ldu(&pt_.x), rayDir = v_ldu(&dir.x);
  bbox3f rayBox;
  v_bbox3_init_by_ray(rayBox, rayStart, rayDir, v_splats(mint));
  if (!v_bbox3_test_box_inside(worldBox, rayBox))
  {
    v_stu_half(minMaxT, v_ray_box_intersect_dist(worldBox.bmin, worldBox.bmax, rayStart, rayDir, v_zero()));
    if (minMaxT[0] > mint || minMaxT[0] > minMaxT[1])
      return false;
    // ray intersects box if tmax >= max(ray.tmin, tmin) && tmin <= ray.tmax
    rayTBoxMin = max(minMaxT[0] - 1e-3f, 0.f);
    mint = min(mint, minMaxT[1] + 1e-3f) - rayTBoxMin;
  }

  const Point3 pt = pt_ + dir * rayTBoxMin - heightmap.getHeightmapOffset();
  real cellSize = heightmap.getHeightmapCellSize();
  real halfCellSize = cellSize * 0.5f;

  WooRay2dInf ray;
  ray.init(Point2::xz(pt), Point2::xz(dir), Point2(cellSize, cellSize));

  int num = int(ceil(4 * mint / cellSize));
  float nextT = 0;
  float curRayY = 0;
  int precise_steps = approx ? (num < 4 ? num : 4) : num;
  for (int n = precise_steps; n > 0; --n)
  {
    IPoint2 currentCell = ray.currentCell();
    ray.nextCell(nextT);
    if (nextT >= mint) // since we traverse 2d ray, but mint is 3d
    {
      nextT = mint;
      n = num = 0;
    }
    const float prevRayY = curRayY;
    curRayY = dir.y * nextT;
    if (uint32_t(currentCell.x >= heightmap.getHeightmapSizeX()) || uint32_t(currentCell.y >= heightmap.getHeightmapSizeY()))
      continue;

    // if (heightmap.getHeightmapCell5Pt(currentCell, h0, hx, hy, hxy, hmid))
    const float rayMinY = pt.y + min(curRayY, prevRayY), rayMaxY = pt.y + max(curRayY, prevRayY);
    if (trace_heightmap_cell(heightmap, currentCell, cellSize, rayMinY, rayMaxY, pt, dir, mint, nullptr, true))
      return true;
  }
  if (!approx)
    return false;
  for (int n = num - precise_steps; n > 0; --n)
  {
    IPoint2 currentCell = ray.currentCell();
    float oldT = nextT;
    ray.nextCell(nextT);
    if (nextT >= mint) // since we traverse 2d ray, but mint is 3d
    {
      nextT = mint;
      n = 0;
    }
    if (uint32_t(currentCell.x >= heightmap.getHeightmapSizeX()) || uint32_t(currentCell.y >= heightmap.getHeightmapSizeY()))
      continue;

    real ht;
    if (heightmap.getHeightmapHeight(currentCell, ht))
    {
      if (pt.y + dir.y * (nextT + oldT) * 0.5 < ht) // mid shadow
        // if (pt.y+max(dir.y*nextT, dir.y*oldT)<ht) //less shadowy
        return true;
    }
  }

  return false;
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
  float rayTBoxMin = 0;
  bbox3f worldBox = HmapGetMinMax<HM>::getWorldBox(heightmap);
  float minMaxT[2];
  vec3f rayStart = v_ldu(&pt_.x), rayDir = v_ldu(&dir.x);
  bbox3f rayBox;
  v_bbox3_init_by_ray(rayBox, rayStart, rayDir, v_splats(mint));
  if (!v_bbox3_test_box_inside(worldBox, rayBox))
  {
    v_stu_half(minMaxT, v_ray_box_intersect_dist(worldBox.bmin, worldBox.bmax, rayStart, rayDir, v_zero()));
    if (minMaxT[0] > mint || minMaxT[0] > minMaxT[1])
      return false;
    // ray intersects box if tmax >= max(ray.tmin, tmin) && tmin <= ray.tmax
    rayTBoxMin = max(minMaxT[0] - 1e-3f, 0.f);
    mint = min(mint, minMaxT[1] + 1e-3f) - rayTBoxMin;
  }

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
  int width = heightmap.getHeightmapSizeX();
  // G_ASSERT(is_pow_of2(width));
  // int bitWidth = get_log2w(width);
  scaledDirY *= 0.5; // remove one multiplication from comparison
  // for (; n < steps; ++n)
  ray.nextCell(nextT); // intentionally do not check first pixel. Src or Dest can not be below heightmap, only due inaccuracy.
  if (nextT >= mint)
    return false;

  for (;;)
  {
    if (!(hmapCellBox & ray.currentCell()))
      return false;
    uint16_t ht = heightmap.getHeightmapHeightUnsafe(ray.currentCell());
    float oldT = nextT;
    ray.nextCell(nextT);
    if (nextT >= mint) // intentionally do not check last pixel. Src or Dest can not be below heightmap, only due inaccuracy.
      return false;
    real minRayY = pt.y + scaledDirY * (min(mint, nextT) + oldT); // scaledDirY already multiplied by 0.5
    if ((int)ht > (int)minRayY)
      return true;
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
