//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds2.h>
#include <math/dag_mathBase.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_adjpow2.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/sort.h>
#include <limits.h>
#include <vecmath/dag_vecMath.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include "../../render/l8_64_noise_lut_api.h"
#include "../../landMesh/shaders/land_detile.hlsli"

// metres per noise feature; not a power of two, so cluster switching gets no pow-2 rhythm
#define LAND_DETILE_DEFAULT_NOISE_SIZE 497.f

// De-tiling of a landclass tile: landMesh/detiled_landclass.md
struct LandDetile
{
  int clusters = 0; // 0 = off
  float angle = 0;  // radians, cluster 0
  float spread = 0; // radians, +- for clusters 1..
  float size = 1;   // tile size, metres

  bool enabled() const { return clusters > 0; }
  // one rule for every source of a count; a size the shader cannot invert keeps it off
  int clampClusters(int raw) const { return size > 0.f ? clamp(raw, 0, LAND_DETILE_MAX_CLUSTERS) : 0; }

  // false = the keys were corrected: the count clamped, or de-tiling turned off; out_err_msg then gets the reason
  bool load(const DataBlock &detail_blk, String *out_err_msg = nullptr)
  {
    size = detail_blk.getPoint2("size", Point2(1, 1)).x;
    const int raw = detail_blk.getInt("detileClusters", 0);
    clusters = clampClusters(raw);
    angle = DegToRad(detail_blk.getReal("detileMainDir", 0.f));
    spread = DegToRad(detail_blk.getReal("detileAngleSpread", 180.f));
    // constraint: the land shaders de-tile only a simple or a detailmap landclass; load_land_class() picks the kind in this order
    const char *unsupported = nullptr;
    if (detail_blk.getStr("shader", nullptr))
      unsupported = "custom shader";
    else if (!detail_blk.getStr("detailmap", nullptr) && detail_blk.getStr("splattingmap", nullptr))
      unsupported = "splattingmap";
    if (clusters > 0 && unsupported)
    {
      clusters = 0;
      if (out_err_msg)
        out_err_msg->printf(0, "detileClusters ignored, a %s landclass is not de-tiled", unsupported);
      return false;
    }
    if (clusters == raw)
      return true;
    if (out_err_msg)
      out_err_msg->printf(0, "detileClusters out of 0..%d (clamped) or size <= 0 (de-tiling off)", LAND_DETILE_MAX_CLUSTERS);
    return false;
  }
};

// The CPU twin of shaders/land_detile_inc.dshl. LUT, hash, floor and mul only,
// so every machine generates the same objects (the RIGen determinism contract).
namespace land_detile
{
extern float noise_size;           // the level's noise feature size, metres: one value for the shaders and the generators
void set_noise_size(float metres); // lmeshLandClasses.cpp: the CPU value and the shader global
// the CPU value alone, for a module that reaches the shaders by interface (the daEditor plugin)
inline void set_noise_size_cpu(float metres) { noise_size = max(metres, 1.f); }
inline float level_noise_size(const DataBlock &blk) { return blk.getReal("landDetileNoiseSize", LAND_DETILE_DEFAULT_NOISE_SIZE); }

inline uint32_t pcg_hash(uint32_t v)
{
  uint32_t state = v * 747796405u + 2891336453u;
  uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// the quintic value noise of smooth_noise_lut.dshl over the L8 LUT of render/noiseTex.cpp
inline float lut(int x, int y) { return l8_64_noise_lut(x, y) * (1.f / 255.f); }
inline float noise_blend(float tx, float ty, float l00, float l10, float l01, float l11)
{
  const float bx = tx * tx * tx * (tx * (tx * 6.f - 15.f) + 10.f), by = ty * ty * ty * (ty * (ty * 6.f - 15.f) + 10.f);
  return l00 * (1 - bx) * (1 - by) + l10 * bx * (1 - by) + l01 * (1 - bx) * by + l11 * bx * by;
}
inline float noise(const Point2 &p)
{
  const float fx = floorf(p.x), fy = floorf(p.y);
  const int x = (int)fx, y = (int)fy;
  return noise_blend(p.x - fx, p.y - fy, lut(x, y), lut(x + 1, y), lut(x, y + 1), lut(x + 1, y + 1));
}
// noise() that keeps the LUT corners: spots close together mostly share one LUT cell
struct NoiseCache
{
  int x = INT_MIN, y = INT_MIN;
  float l00 = 0, l10 = 0, l01 = 0, l11 = 0;
  void load(int ix, int iy)
  {
    if (ix == x && iy == y)
      return;
    x = ix;
    y = iy;
    l00 = lut(ix, iy);
    l10 = lut(ix + 1, iy);
    l01 = lut(ix, iy + 1);
    l11 = lut(ix + 1, iy + 1);
  }
  float operator()(const Point2 &p)
  {
    const float fx = floorf(p.x), fy = floorf(p.y);
    load((int)fx, (int)fy);
    return noise_blend(p.x - fx, p.y - fy, l00, l10, l01, l11);
  }
};
// noise_blend for four points of one LUT cell
VECTORCALL inline vec4f noise_blend4(vec4f tx, vec4f ty, const NoiseCache &nc)
{
  const vec4f c6 = v_splats(6.f), c15 = v_splats(15.f), c10 = v_splats(10.f);
  const vec4f bx = v_mul(v_mul(v_mul(tx, tx), tx), v_add(v_mul(tx, v_sub(v_mul(tx, c6), c15)), c10));
  const vec4f by = v_mul(v_mul(v_mul(ty, ty), ty), v_add(v_mul(ty, v_sub(v_mul(ty, c6), c15)), c10));
  const vec4f ax = v_sub(V_C_ONE, bx), ay = v_sub(V_C_ONE, by);
  return v_add(v_add(v_mul(v_mul(v_splats(nc.l00), ax), ay), v_mul(v_mul(v_splats(nc.l10), bx), ay)),
    v_add(v_mul(v_mul(v_splats(nc.l01), ax), by), v_mul(v_mul(v_splats(nc.l11), bx), by)));
}
// the exact range of noise() over an axis-aligned box in noise units:
// inside one LUT cell the value is bilinear in the monotonic fades, so its extremes sit at the corners of each cell piece
inline void noise_range(const BBox2 &box, float &lo, float &hi, NoiseCache &nc)
{
  vec4f vlo = v_splats(1.f), vhi = v_zero();
  const int x0 = (int)floorf(box[0].x), x1 = (int)floorf(box[1].x), y0 = (int)floorf(box[0].y), y1 = (int)floorf(box[1].y);
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++)
    {
      nc.load(x, y);
      const float tx0 = max(box[0].x - x, 0.f), tx1 = min(box[1].x - x, 1.f);
      const float ty0 = max(box[0].y - y, 0.f), ty1 = min(box[1].y - y, 1.f);
      const vec4f v = noise_blend4(v_make_vec4f(tx0, tx1, tx0, tx1), v_make_vec4f(ty0, ty0, ty1, ty1), nc);
      vlo = v_min(vlo, v);
      vhi = v_max(vhi, v);
    }
  lo = v_extract_x(v_hmin(vlo));
  hi = v_extract_x(v_hmax(vhi));
}

// s in [0, clusters-1]: cluster floor(s) fades into floor(s)+1 (detile_nodes in the shader)
inline void nodes(float s, int &seed, float &w)
{
  const float fs = floorf(s);
  seed = (int)fs;
  w = 1.f - (s - fs);
  if (w <= LAND_DETILE_HARDNESS)
  {
    seed += 1;
    w = 1.f;
  }
  else if (w < 1.f - LAND_DETILE_HARDNESS)
  {
    w = (w - LAND_DETILE_HARDNESS) * (1.f / (1.f - 2.f * LAND_DETILE_HARDNESS));
    w = w * w * (3.f - 2.f * w);
  }
  else
    w = 1.f;
}

inline int dominant_of_noise(float n, int clusters)
{
  int seed;
  float w;
  nodes(n * (clusters - 1), seed, w);
  return w >= 0.5f ? seed : seed + 1;
}
// the cluster an object at world_xz belongs to: the one that shows most of the texture there
inline int dominant_cluster(const LandDetile &d, const Point2 &world_xz, float noise_size_metres)
{
  if (d.clusters < 1)
    return 0;
  return dominant_of_noise(noise(world_xz * (1.f / noise_size_metres)), d.clusters);
}
inline int dominant_cluster(const LandDetile &d, const Point2 &world_xz) { return dominant_cluster(d, world_xz, noise_size); }

struct Cluster
{
  float s = 0, c = 1;  // sin, cos of the angle
  Point2 ofs = {0, 0}; // metres
};
// detile_cluster in the shader: cluster 0 exact, the others a stratum of the spread and an R2 offset
inline Cluster cluster(const LandDetile &d, int k)
{
  Cluster r;
  float ang = d.angle;
  if (k > 0)
  {
    const float jitter = (pcg_hash(k + 1u) & 0xFFFFu) * (1.f / 65535.f);
    const float t = (float(k - 1) + 0.25f + 0.5f * jitter) / float(d.clusters - 1);
    ang += (t * 2.f - 1.f) * d.spread;
    const Point2 q = Point2(LAND_DETILE_R2_X, LAND_DETILE_R2_Y) * float(k);
    r.ofs = Point2(q.x - floorf(q.x), q.y - floorf(q.y)) * d.size;
  }
  // vecmath's polynomial, not libm: the same bits on every generating platform
  vec4f s4, c4;
  v_sincos_x(v_splats(ang), s4, c4);
  r.s = v_extract_x(s4);
  r.c = v_extract_x(c4);
  return r;
}
// the texture at world p shows the tile at q = to_pattern(p); to_world is the inverse
inline Point2 to_pattern(const Cluster &c, const Point2 &p) { return Point2(c.c * p.x + c.s * p.y, c.c * p.y - c.s * p.x) + c.ofs; }
inline Point2 to_world(const Cluster &c, const Point2 &q)
{
  const Point2 v = q - c.ofs;
  return Point2(c.c * v.x - c.s * v.y, c.c * v.y + c.s * v.x);
}
// an object authored on the tile turns with it: its axes follow to_world
inline void rotate_tm(const Cluster &c, TMatrix &tm)
{
  for (int i = 0; i < 3; ++i)
  {
    const Point3 v = tm.getcol(i);
    tm.setcol(i, Point3(c.c * v.x - c.s * v.z, v.y, c.c * v.z + c.s * v.x));
  }
}
// The de-tile state of a generation pass: tiled walks each cluster that can show, planted picks one per spot; off = plain grid
struct ClusterWalk
{
  const LandDetile *d = nullptr;
  Point2 eofs = {0, 0}; // the caller's frame: its rects and local positions are world metres minus this
  float noiseSize;      // snapshot: the selection reads no ambient state mid-pass
  Cluster c;
  int k = 0;
  Cluster table[LAND_DETILE_MAX_CLUSTERS];
  // table_: cluster(*detile, k) for every k, computed once by the owner; null = computed here
  ClusterWalk(const LandDetile *detile = nullptr, const Point2 &eofs_ = Point2(0, 0), const Cluster *table_ = nullptr) :
    d(detile && detile->enabled() ? detile : nullptr), eofs(eofs_), noiseSize(noise_size)
  {
    for (int i = 0; d && i < d->clusters; i++)
      table[i] = table_ ? table_[i] : cluster(*d, i);
  }
  bool on() const { return d != nullptr; }
  int clusters() const { return d ? d->clusters : 1; }
  void select(int k_)
  {
    k = k_;
    c = d ? table[k_] : Cluster();
  }
  // the clusters that can show somewhere in rect (the caller's frame); off = the one plain pass
  void clusterRange(const BBox2 &rect, int &lo, int &hi, NoiseCache &nc) const
  {
    lo = hi = 0;
    if (!d || d->clusters < 2)
      return;
    const float r = 1.f / noiseSize, spread = float(d->clusters - 1);
    float nlo, nhi;
    noise_range(BBox2((rect[0] + eofs) * r, (rect[1] + eofs) * r), nlo, nhi, nc);
    // dominant_cluster rounds the scaled noise; the margin covers its float path
    lo = max(int(ceilf(nlo * spread - 0.5f - 1e-3f)), 0);
    hi = min(int(floorf(nhi * spread + 0.5f + 1e-3f)), d->clusters - 1);
  }
  // the pattern-space rect the walk must cover to fill rect, both in the caller's frame
  BBox2 walkRect(const BBox2 &rect) const
  {
    if (!d)
      return rect;
    const BBox2 w(rect[0] + eofs, rect[1] + eofs);
    BBox2 r;
    r += to_pattern(c, w[0]);
    r += to_pattern(c, Point2(w[1].x, w[0].y));
    r += to_pattern(c, Point2(w[0].x, w[1].y));
    r += to_pattern(c, w[1]);
    return BBox2(r[0] - eofs, r[1] - eofs);
  }
  // p in absolute metres: the pattern-space spot in, the world spot out; returns its world xz for shows()
  Point2 toWorld(Point3 &p) const
  {
    if (!d)
      return Point2::xz(p);
    const Point2 w = to_world(c, Point2::xz(p));
    p.x = w.x;
    p.z = w.y;
    return w;
  }
  Point2 toWorldLocal(Point3 &p) const // the same for p in the caller's frame; still returns absolute world xz
  {
    if (!d)
      return Point2::xz(p) + eofs;
    Point3 a(p.x + eofs.x, p.y, p.z + eofs.y);
    const Point2 w = toWorld(a);
    p.x = w.x - eofs.x;
    p.z = w.y - eofs.y;
    return w;
  }
  // false = another cluster shows at world_xz; the costly test, so callers run it last, and one cluster skips its noise read
  bool shows(const Point2 &world_xz) const { return !d || d->clusters == 1 || dominant_cluster(*d, world_xz, noiseSize) == k; }
  void rotateTm(TMatrix &tm) const // a placed object turns with the tile
  {
    if (d)
      rotate_tm(c, tm);
  }
  // the same yaw in the matrix's own frame: a terrain-aligned basis keeps its up axis
  void rotateTmLocal(TMatrix &tm) const
  {
    if (!d)
      return;
    TMatrix r = TMatrix::IDENT;
    r.setcol(0, Point3(c.c, 0.f, c.s));
    r.setcol(2, Point3(-c.s, 0.f, c.c));
    tm = tm * r;
  }
  // half-open: adjacent preimages overlap, so exactly one rect keeps a shared-edge candidate
  bool owns(const BBox2 &rect, const Point2 &p) const
  {
    if (!d)
      return true;
    return p.x >= rect[0].x && p.x < rect[1].x && p.y >= rect[0].y && p.y < rect[1].y;
  }
};

// the density map of a planted group, in pattern-space metres
struct PlantGrid
{
  Point2 org;  // the pattern-space corner of density cell (0, 0): a fixed anchor, the map offset from the level origin
  Point2 cell; // metres per density cell
  Point2 cellRcp;
  PlantGrid(const Point2 &org_, const Point2 &cell_) : org(org_), cell(cell_), cellRcp(1.f / cell_.x, 1.f / cell_.y) {}
};
// the index of a spot at [0, size): a power of two masks, others take a float floor
inline int wrap_cell(int c, int size, float size_rcp)
{
  if (is_pow_of2(size))
    return c & (size - 1);
  int r = c - size * int(floorf(float(c) * size_rcp));
  return r < 0 ? r + size : r >= size ? r - size : r;
}
// one bit per L1 leaf of a density map, set where the leaf holds any set cell:
// the planted pass asks it whether a block's footprint can hold a spot, without walking the hierarchy
struct LeafOccupancy
{
  int w = 0, h = 0, rowWords = 0; // in leaves
  int shift = 0;                  // cells to leaves
  mutable dag::Vector<uint64_t> bits;
  template <class M>
  void build(const M &m)
  {
    using L1 = typename M::L2Bmp::L1Bmp;
    shift = L1::FULL_SHBITS;
    w = m.getW() >> shift;
    h = m.getH() >> shift;
    rowWords = (w + 63) >> 6;
    bits.assign(rowWords * h, 0);
    m.enumerateL1Blocks(*this, IBBox2(IPoint2(0, 0), IPoint2(m.getW() - 1, m.getH() - 1)));
  }
  template <class L1>
  void onNonEmptyL1Block(const L1 *, int u0, int v0) const
  {
    const int x = u0 / L1::SZ, y = v0 / L1::SZ;
    bits[y * rowWords + (x >> 6)] |= 1ull << (x & 63);
  }
  // any set cell in the leaves that hold cells [x0, x1] x [y0, y1]; cells past the map are empty
  bool any(const IBBox2 &cells) const
  {
    const int x0 = max(cells[0].x >> shift, 0), x1 = min(cells[1].x >> shift, w - 1);
    const int y0 = max(cells[0].y >> shift, 0), y1 = min(cells[1].y >> shift, h - 1);
    for (int y = y0; y <= y1; y++)
      for (int wi = x0 >> 6; wi <= x1 >> 6; wi++)
      {
        const int lo = max(x0 - (wi << 6), 0), hi = min(x1 - (wi << 6), 63);
        if (bits[y * rowWords + wi] & ((~0ull >> (63 - hi)) & (~0ull << lo)))
          return true;
      }
    return false;
  }
};
struct AnyL1Block
{
  mutable bool any = false;
  template <class L1>
  void onNonEmptyL1Block(const L1 *, int, int) const
  {
    any = true;
  }
};
// false = the density map is empty under the pattern-space footprint of world_box for cluster c;
// occ, when given, answers from the leaf bits of the same map
template <class DensMap>
inline bool density_under(const DensMap &dm, const LeafOccupancy *occ, const PlantGrid &g, const Cluster &c, const BBox2 &world_box)
{
  // the rotated box's bounds: its centre mapped, plus the half extents turned
  const Point2 half = (world_box[1] - world_box[0]) * 0.5f;
  const Point2 pc = to_pattern(c, world_box[0] + half) - g.org;
  const Point2 ext(fabsf(c.c) * half.x + fabsf(c.s) * half.y, fabsf(c.s) * half.x + fabsf(c.c) * half.y);
  // one cell of slack each side covers the rounding of the per-spot mapping
  int x0 = int(floorf((pc.x - ext.x) * g.cellRcp.x)) - 1, x1 = int(floorf((pc.x + ext.x) * g.cellRcp.x)) + 1;
  int y0 = int(floorf((pc.y - ext.y) * g.cellRcp.y)) - 1, y1 = int(floorf((pc.y + ext.y) * g.cellRcp.y)) + 1;
  const IPoint2 size(dm.getW(), dm.getH());
  const int ax = wrap_cell(x0, size.x, 1.f / size.x), ay = wrap_cell(y0, size.y, 1.f / size.y);
  const int bx = ax + x1 - x0, by = ay + y1 - y0;
  // a footprint across the repeat splits in up to four rects
  const IPoint2 xs[2] = {IPoint2(ax, min(bx, size.x - 1)), IPoint2(0, bx - size.x)};
  const IPoint2 ys[2] = {IPoint2(ay, min(by, size.y - 1)), IPoint2(0, by - size.y)};
  for (int j = 0; j < 2; j++)
    for (int i = 0; i < 2; i++)
      if (xs[i].y >= xs[i].x && ys[j].y >= ys[j].x)
      {
        const IBBox2 part(IPoint2(xs[i].x, ys[j].x), IPoint2(xs[i].y, ys[j].y));
        AnyL1Block cb;
        if (!occ)
          dm.enumerateL1Blocks(cb, part);
        if (occ ? occ->any(part) : cb.any)
          return true;
      }
  return false;
}
// lowbias32 (C. Wellons): constant shifts, so four spots hash in one vector
inline uint32_t spot_hash(uint32_t x)
{
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  return x ^ (x >> 16);
}
VECTORCALL inline vec4i spot_hash4(vec4i x)
{
  x = v_xori(x, v_srli(x, 16));
  x = v_muli(x, v_splatsi(0x7feb352d));
  x = v_xori(x, v_srli(x, 15));
  x = v_muli(x, v_splatsi(int(0x846ca68bu)));
  return v_xori(x, v_srli(x, 16));
}
// the cells of block spots i among 2^bits: a seed-keyed bijection, so no repeats and no stride lines them up
VECTORCALL inline vec4i block_cells4(vec4i i, uint32_t bseed, int bits)
{
  const vec4i mask = v_splatsi((1 << bits) - 1);
  const int half = bits >> 1;
  vec4i y = v_andi(v_muli(v_xori(i, v_splatsi(int(bseed))), v_splatsi(int(bseed >> 7) | 1)), mask);
  y = v_andi(v_muli(v_xori(y, v_srli_n(y, half)), v_splatsi(0x2C1B3C6D)), mask);
  y = v_andi(v_muli(v_xori(y, v_srli_n(y, half)), v_splatsi(0x297A2D39)), mask);
  return v_xori(y, v_srli_n(y, half));
}
// the dominant cluster of four spots at noise-space p: round the scaled noise, the rule of dominant_cluster;
// four spots in one LUT cell share its corners
VECTORCALL inline vec4i noise_clusters4(NoiseCache &nc, vec4f px, vec4f pz, int clusters)
{
  const vec4f fx = v_floor(px), fz = v_floor(pz);
  const vec4i cx = v_cvti_vec4i(fx), cz = v_cvti_vec4i(fz);
  const float spread = float(clusters - 1);
  if (v_check_xyzw_all_true(v_cast_vec4f(v_andi(v_cmp_eqi(cx, v_splat_xi(cx)), v_cmp_eqi(cz, v_splat_xi(cz))))))
  {
    nc.load(v_extract_xi(cx), v_extract_xi(cz));
    const vec4f n = noise_blend4(v_sub(px, fx), v_sub(pz, fz), nc);
    return v_cvti_vec4i(v_ceil(v_sub(v_mul(n, v_splats(spread)), V_C_HALF)));
  }
  alignas(16) float xs[4], zs[4];
  v_st(xs, px);
  v_st(zs, pz);
  return v_make_vec4i(int(ceilf(nc(Point2(xs[0], zs[0])) * spread - 0.5f)), int(ceilf(nc(Point2(xs[1], zs[1])) * spread - 0.5f)),
    int(ceilf(nc(Point2(xs[2], zs[2])) * spread - 0.5f)), int(ceilf(nc(Point2(xs[3], zs[3])) * spread - 0.5f)));
}
// the density cells of four spots at world x, z under cluster c: the rule of to_pattern(c, w) - g.org
VECTORCALL inline void plant_cells4(const Cluster &c, const PlantGrid &g, vec4f x, vec4f z, int *u, int *v)
{
  const vec4f qx = v_sub(v_add(v_mul(v_splats(c.c), x), v_mul(v_splats(c.s), z)), v_splats(g.org.x - c.ofs.x));
  const vec4f qz = v_sub(v_sub(v_mul(v_splats(c.c), z), v_mul(v_splats(c.s), x)), v_splats(g.org.y - c.ofs.y));
  v_sti(u, v_cvti_vec4i(v_floor(v_mul(qx, v_splats(g.cellRcp.x)))));
  v_sti(v, v_cvti_vec4i(v_floor(v_mul(qz, v_splats(g.cellRcp.y)))));
}
static constexpr int PLANT_BATCH = 32; // spots per batch of plant_world, a multiple of 4
// The world grid of blocks (near 64 metres, 16 to 256 density cells a side) is walked once, whatever the cluster count,
// and every spot seeds its own randoms, so the pass order changes no placement.
// keepBlock(local_box) = false skips a block where no spot can pass the mask; keep(pos) filters a spot by the mask,
// must set pos.y (it arrives 0) and may move pos in xz, but the cluster and the density bit read the grid position;
// spot(pos, seed) gets the kept pos, its cluster selected in walk.
template <class DensMap, class KeepBlock, class Keep, class Spot>
inline void plant_world(ClusterWalk &walk, const BBox2 &rect, const DensMap &dm, const LeafOccupancy *occ, const PlantGrid &g,
  float spots_per_cell, uint32_t rseed, KeepBlock &&keepBlock, Keep &&keep, Spot &&spot)
{
  const IPoint2 size(dm.getW(), dm.getH());
  int blk_cells = 16;
  while (blk_cells < 256 && blk_cells * min(g.cell.x, g.cell.y) < 64.f)
    blk_cells *= 2;
  const int blkShift = get_log2i(blk_cells);
  const Point2 blk = g.cell * float(blk_cells);
  const Point2 w0 = rect[0] + walk.eofs, w1 = rect[1] + walk.eofs;
  const int ix0 = (int)floorf(w0.x / blk.x), ix1 = (int)ceilf(w1.x / blk.x);
  const int iy0 = (int)floorf(w0.y / blk.y), iy1 = (int)ceilf(w1.y / blk.y);
  const float perBlock = spots_per_cell * float(blk_cells * blk_cells);
  const Point2 sizeRcp(1.f / size.x, 1.f / size.y);
  const float noiseRcp = 1.f / walk.noiseSize;
  NoiseCache nc;
  for (int iy = iy0; iy < iy1; iy++)
    for (int ix = ix0; ix < ix1; ix++)
    {
      const BBox2 wb(Point2(ix * blk.x, iy * blk.y), Point2((ix + 1) * blk.x, (iy + 1) * blk.y));
      const BBox2 lb(wb[0] - walk.eofs, wb[1] - walk.eofs);
      if (!keepBlock(lb))
        continue;
      // the block is dropped when the density map is empty under its footprint for every cluster it can show;
      // a block that shows one cluster skips the per-spot pick
      int kLo, kHi;
      walk.clusterRange(lb, kLo, kHi, nc);
      bool dense = false;
      for (int k = kLo; k <= kHi && !dense; k++)
        dense = density_under(dm, occ, g, walk.table[k], wb);
      if (!dense)
        continue;
      const uint32_t bseed = spot_hash(uint32_t(ix) * 0x8DA6B343u ^ uint32_t(iy) * 0xD8163841u ^ rseed);
      int n = int(perBlock);
      if ((spot_hash(bseed) >> 8) * (1.f / 16777216.f) < perBlock - float(n))
        n++;
      const bool single = kLo == kHi;
      const Cluster &c0 = walk.table[kLo];
      for (int i0 = 0; i0 < n; i0 += PLANT_BATCH)
      {
        const int m = min(n - i0, PLANT_BATCH);
        // four spots per vector: position, then the cluster (noise) or the density cell (one cluster)
        alignas(16) float bx[PLANT_BATCH], bz[PLANT_BATCH];
        alignas(16) int bk[PLANT_BATCH], bu[PLANT_BATCH], bv[PLANT_BATCH], bh[PLANT_BATCH];
        for (int j = 0; j < m; j += 4)
        {
          const vec4i iv = v_addi(v_splatsi(i0 + j), v_make_vec4i(0, 1, 2, 3));
          const vec4i h = spot_hash4(v_addi(v_splatsi(bseed), v_muli(iv, v_splatsi(0x9E3779B9))));
          const vec4i ci = block_cells4(iv, bseed, 2 * blkShift);
          const vec4f jx = v_mul(v_cvti_vec4f(v_andi(h, v_splatsi(0xFFFF))), v_splats(1.f / 65536.f));
          const vec4f jz = v_mul(v_cvti_vec4f(v_srli(h, 16)), v_splats(1.f / 65536.f));
          const vec4f x = v_mul(v_add(v_cvti_vec4f(v_addi(v_splatsi(ix * blk_cells), v_andi(ci, v_splatsi(blk_cells - 1)))), jx),
            v_splats(g.cell.x));
          const vec4f z =
            v_mul(v_add(v_cvti_vec4f(v_addi(v_splatsi(iy * blk_cells), v_srli_n(ci, blkShift))), jz), v_splats(g.cell.y));
          v_st(bx + j, x);
          v_st(bz + j, z);
          v_sti(bh + j, h);
          if (!single)
          {
            v_sti(bk + j, noise_clusters4(nc, v_mul(x, v_splats(noiseRcp)), v_mul(z, v_splats(noiseRcp)), walk.d->clusters));
            continue;
          }
          plant_cells4(c0, g, x, z, bu + j, bv + j);
        }
        for (int j = 0; j < m; j++)
        {
          const Point2 w(bx[j], bz[j]), local = w - walk.eofs;
          if (!walk.owns(rect, local))
            continue;
          // the cheaper test first: the density bit where the cluster is known, else the mask
          Point3 pos(local.x, 0.f, local.y);
          if (!single && !keep(pos))
            continue;
          const int kw = single ? kLo : bk[j];
          int u, v;
          if (single)
            u = bu[j], v = bv[j];
          else
          {
            const Point2 q = to_pattern(walk.table[kw], w) - g.org;
            u = int(floorf(q.x * g.cellRcp.x));
            v = int(floorf(q.y * g.cellRcp.y));
          }
          u = wrap_cell(u, size.x, sizeRcp.x);
          v = wrap_cell(v, size.y, sizeRcp.y);
          if (!dm.get(u, v))
            continue;
          if (single && !keep(pos))
            continue;
          walk.select(kw);
          spot(pos, int(spot_hash(uint32_t(bh[j]) ^ 0x5bd1e995u))); // hashed late: most spots fail above
        }
      }
    }
}

// the seed of one tiled spot in a de-tiled pass: no spot's randoms depend on the visit order
inline int tiled_spot_seed(uint32_t rseed, int tile_x, int tile_y, int k, int spot)
{
  return int(pcg_hash(rseed ^ pcg_hash(uint32_t(tile_x) * 0x8DA6B343u ^ uint32_t(tile_y) * 0xD8163841u ^ uint32_t(k) * 0xB5297A4Du ^
                                       uint32_t(spot) * 0x9E3779B9u)));
}
// the spots of a tiled group sorted by their x in the tile: a cluster pass reads only its range
struct TiledSpots
{
  dag::Vector<float> x;
  dag::Vector<int> idx;
  void build(dag::ConstSpan<TMatrix> tm)
  {
    idx.resize(tm.size());
    for (int i = 0; i < idx.size(); i++)
      idx[i] = i;
    eastl::sort(idx.begin(), idx.end(), [&](int a, int b) { return tm[a].getcol(3).x < tm[b].getcol(3).x; });
    x.resize(idx.size());
    for (int i = 0; i < idx.size(); i++)
      x[i] = tm[idx[i]].getcol(3).x;
  }
  int lowerBound(float x0) const { return int(eastl::lower_bound(x.begin(), x.end(), x0) - x.begin()); }
};
} // namespace land_detile
