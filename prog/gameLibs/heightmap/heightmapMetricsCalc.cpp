// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/heightmapMetricsCalc.h>
#include <heightmap/heightmapHandler.h>
#include <math/dag_marching_squares.h>
#include <math/dag_sdf_edt.h>
#include <perfMon/dag_perfTimer.h>
#include <supp/dag_alloca.h>
#include "heightmapMetricsErr.h"
#include "riverMasks/hmapCriticalPath.h"
#include "riverMasks/hmapLodMaskBuild.h"

// half texel disappeared

// + cache metrics error
// + calc metrics using triangulation
// + there are now 4 options with indexed base: two diamond and two regular (than 4 drawIndexed calls),
//   almost no cost whatsoever (2 bits per patch, 4 drawcalls instead of one).
// + water metrices change
// + also we store in 256 bit _exact_ triangulation for all quads per patch (and then sample them in VS).
// + support mirror
// + exactEdge: suport "parents" level
// + float accuracy on big levels
// + increase error with quad diagonal intersecting closest ridge angle (fixes zig-zags)
// + enhance silhoette changes (based on ridge, slope, and laplacians curvature).
// todo:
// * 'no morph render' support
// * store heightbounds only for higher level of hier
// * virtual texture and patches beyond normal data (can only be build dynamically)
// * better distance to patch
// * morph:support not exact edge
// * better morphing on edges if neighboor is also morphing. RN neighboors morphData is only present if neighboors are of the same lod
//   this is because coarser lod has two neighboors on each side, and since we use max function,
//   and we dont check finer lods at all (so its neighboor is 1).
//   What we could do, is make it's neighboor 0 if it is morphing, but then finer neighbor lod becomes "morphing patch" (in a certain
//   sense, it morphs on edge);
// * better "bad shore" detection - only when river is disappearing we should significantly increase error
// * occlusion only for levels that makes sense (auto some it is same occusion hzb pixels as previous)
// * support multi-res (tank + plane distances)
// * fix corners for exactEdges = false
// * uv tile to be passed to
// * compare perf
// * cleanup
// * do not allow neighboors to have more lod dsit than there is log2(dimension) (i.e. for dim = 16 max possible lod dist is 4, it is
// 'sew' lod
//   10 with 5, for example)
// * mirror: heightmap patches in positive direction do not mirror correctly,
//   as last vertex is at texSize + 1 (so it is mirrored).
//   to fix that we need to author hSize + 1 textures, so texture size is 2^n+1 (2^n patches).
//   Than we would correctly mirror in all directions, but we would need to obviously change uv to sample from such texture
// *? store heightmap_edges statically. it is not very big buffer, and it optimizes CPU part
// ? support displacement available dist
// ? sort front-to-back
// - compress metrics (quantized to 8 bit is probably enough, if we have per-lod multiple of errorScale)

namespace hmap_debug
{
Point3 debug_water_edges[4];
int debug_water_edges_count = 0;
Point3 debug_quad_points[4];
// static Point2 debug_point(15700.4, -15700.2);
// static Point2 debug_point(-30530.4, -21940.2);
static constexpr int DEBUG_QUAD_LEVEL = 8;
Point2 debug_point(4204.9, 4274.25);
// static Point2 debug_point(26684.4, -16397.2);
Point2 debug_point2(592.713, 59225.59);
static constexpr bool debug_position = false;

static bool debug_quad_level;
}; // namespace hmap_debug

using namespace hmap_debug;

void MetricsErrors::clear()
{
  data.clear();
  exactOrder.clear();
  order.clear();
  lodsRiverMasks.clear();
}

static bool debug_patch = false;

static int get_addressed(const HeightmapHandler &h, int x, int w)
{
  return h.isMirror() ? (x < 0 ? -x : (x >= w ? (w << 1) - 1 - x : x)) : clamp(x, 0, w - 1);
}

static float get_interpolated_heightmap(const HeightmapHandler &h, float x, float y)
{
  const CompressedHeightmap &c = h.getCompressedData();
  IPoint2 coord(floorf(x), floorf(y));
  Point2 coordF(x - coord.x, y - coord.y);
  int lx, ty, rx, by;
  lx = get_addressed(h, coord.x, h.getHeightmapSizeX());
  ty = get_addressed(h, coord.y, h.getHeightmapSizeY());
  rx = get_addressed(h, coord.x + 1, h.getHeightmapSizeX());
  by = get_addressed(h, coord.y + 1, h.getHeightmapSizeY());
  float vals[4];
  vals[0] = c.decodePixelUnsafe(lx, ty);
  vals[1] = c.decodePixelUnsafe(rx, ty);
  vals[2] = c.decodePixelUnsafe(lx, by);
  vals[3] = c.decodePixelUnsafe(rx, by);
  return lerp(lerp(vals[0], vals[1], coordF.x), lerp(vals[2], vals[3], coordF.x), coordF.y);
}

static float acos_fast_unsafe(float inX)
{
  float x = fabsf(inX);
  float res = -0.156583f * x + (PI / 2);
  res *= sqrtf(fabsf(1.0f - x)); // x can be slightly bigger that 1
  return (inX >= 0) ? res : PI - res;
}

static void calc_quad_error(const HeightmapHandler &h, float quad_size_x, float quad_size_y, float x, float y,
  HeightmapErrorRet &error_ltrb, HeightmapErrorRet &error_rtlb, float lt, float rt, float lb, float rb, const float *prevRow,
  const float *curRow, const float *nextRow, const float *nextNextRow,
  float water_level) // two diagonals triangulation
{
  bool debug_quad = false;
  if (debug_position && debug_patch)
  {
    Point2 qPos = h.worldPosOfs + h.hmapCellSize * Point2(x, y);
    BBox2 qBox(qPos, qPos + Point2(quad_size_x, quad_size_y) * h.hmapCellSize);
    debug_quad = qBox & debug_point;
    if (debug_quad && debug_quad_level)
    {
      debug_quad_points[0] = Point3(qBox[0].x, lt * h.getHeightScale() / 65535. + h.getHeightMin(), qBox[0].y);
      debug_quad_points[1] = Point3(qBox[1].x, rt * h.getHeightScale() / 65535. + h.getHeightMin(), qBox[0].y);
      debug_quad_points[2] = Point3(qBox[0].x, lb * h.getHeightScale() / 65535. + h.getHeightMin(), qBox[1].y);
      debug_quad_points[3] = Point3(qBox[1].x, rb * h.getHeightScale() / 65535. + h.getHeightMin(), qBox[1].y);
    }
  }
  float midActualValue = (lt + rt + lb + rb) * 0.25f;
  const float avgRTLB = (rt + lb) * 0.5f, avgLTRB = (lt + rb) * 0.5f;
  if (quad_size_x <= 1.f && quad_size_y <= 1.f)
  {
    if (debug_position && debug_quad)
      debug("rtlb = %f ltrb = %f actual %f, lt = %f rt = %f lb = %f rb %f", (rt + lb) * 0.5f, (lt + rb) * 0.5f, midActualValue, lt, rt,
        lb, rb);
    // if (debug_position && debug_quad)
    add_error_no_rms(error_rtlb, avgRTLB, midActualValue, water_level);
    add_error_no_rms(error_ltrb, avgLTRB, midActualValue, water_level);
    if (error_rtlb.maxError == error_ltrb.maxError && avgRTLB != avgLTRB && prevRow)
    {
      // there is an ambiguity, both errors are same, although diagonals are  not.
      // There are few ways of resolution
      // 1 us bicubic sample for more "accurate" midValue
      // use gradients to find if it is ridge or valley, and prefer upper (for ridge) and lower for value
      // it seems gradients resolution is more visiually plausible (and also faster)
#if BICUBIC_RESOLUTION
      float biCenter =
        (-(-prevRow[-1] + 9.0f * prevRow[0] + 9.0f * prevRow[1] - prevRow[2]) +
          9.0f * (-curRow[-1] + 9.0f * lt + 9.0f * rt - curRow[2]) + 9.0f * (-nextRow[-1] + 9.0f * lb + 9.0f * rb - nextRow[2]) -
          (-nextNextRow[-1] + 9.0f * nextNextRow[0] + 9.0f * nextNextRow[1] - nextNextRow[2])) *
        1.f / 16.f;
      if (fabsf(avgRTLB - biCenter) > fabsf(avgLTRB - biCenter))
        error_rtlb.maxError = error_ltrb.maxError * 1.01f;
      else
        error_ltrb.maxError = error_rtlb.maxError * 1.01f;
#else
      float gradientDotSum = (rt - curRow[-1]) + (lb - prevRow[0])        // lt gradient, signs: +1,+1
                             - (curRow[2] - lt) + (rb - prevRow[1])       // rt gradient, signs: -1,+1
                             + (rb - nextRow[-1]) - (nextNextRow[0] - lt) // lb gradient, signs: +1,-1
                             + (nextRow[2] - lb) - (nextNextRow[1] - rt); // rb gradient, signs: -1,-1
      // gradientDotSum *= 0.25; should be, but we dont care about anything besides sign
      const bool isRidge = gradientDotSum >= 0; // on gradientSum == 0, we can also make a bicubic interpolation for center instead
      if ((avgRTLB < avgLTRB) == isRidge)       // on ridges prefer upper diagonal, on valleys down diag
        error_rtlb.maxError = error_ltrb.maxError * 1.01f;
      else
        error_ltrb.maxError = error_rtlb.maxError * 1.01f;
#endif
    }
    error_rtlb.rms = sqr(error_rtlb.maxError);
    error_ltrb.rms = sqr(error_ltrb.maxError);
    error_rtlb.cnt = error_ltrb.cnt = 1;
  }
  else
  {
    const Point2 c(x + quad_size_x * 0.5f, y + quad_size_y * 0.5f);
    const float quad_w = 1.f / quad_size_x, quad_h = 1.f / quad_size_y;
    const int l = ceilf(x), t = ceilf(y);
    const int r = min<int>((x + quad_size_x) + 1, h.getHeightmapSizeX()), b = min<int>((y + quad_size_y) + 1, h.getHeightmapSizeY());
    // if (l < r && b < t)
    if (debug_position && debug_quad)
    {
      debug("diag err_ltrb = %f %f, err_rtlb %f %f lt_rb<rt_lb = %d", error_ltrb.maxError, error_ltrb.getRMS(), error_rtlb.maxError,
        error_rtlb.getRMS(), error_ltrb < error_rtlb);
      debug("l %d r %d t = %d b = %d p=%f,%f sz %f,%f", l, r, t, b, x, y, quad_size_x, quad_size_y);
    }
    int cnt = (r - l) * (b - t); // we should include corners!
    h.getCompressedData().iteratePixels(l, t, r - l, b - t, [&](int xi1, int yi1, uint16_t v) {
      if ((xi1 == l || xi1 == r - 1) && (yi1 == t || yi1 == b - 1)) // corners are always correct, save some cycles
        return;
      float xi = xi1, yi = yi1;
      if (xi == c.x && yi == c.y)
        midActualValue = v;
      float xPos = xi - x, yPos = yi - y;
      float xf = xPos * quad_w, yf = yPos * quad_h;

      const float triValueLTRB =
        (xf <= yf) ? lt * (1.0f - yf) + lb * (yf - xf) + rb * xf : lt * (1.0f - xf) + rt * (xf - yf) + rb * yf;

      const float triValueRTLB =
        (xf <= 1.0f - yf) ? lt * (1.0f - yf - xf) + lb * yf + rt * xf : rt * (1.0f - yf) + rb * (xf + yf - 1.0f) + lb * (1.0f - xf);
      if (debug_position && debug_quad && quad_size_x == 2)
      {
        debug("%fx%f: triValueRTLB = %f triValueLTRB = %f, v = %f", xf, yf, triValueRTLB, triValueLTRB, v);
      }
      add_error(error_rtlb, triValueRTLB, v, water_level);
      add_error(error_ltrb, triValueLTRB, v, water_level);
    });
    error_rtlb.cnt = error_ltrb.cnt = cnt;
    if (error_rtlb.maxError == error_ltrb.maxError && error_rtlb.getRMSSq() == error_ltrb.getRMSSq())
    {
      // tie resolution, based on actual mid value
      if (fabsf(avgRTLB - midActualValue) < fabsf(avgLTRB - midActualValue))
      {
        error_ltrb.maxError = error_rtlb.maxError * 1.001f;
        error_ltrb.rms = error_rtlb.rms * 1.001f;
      }
      else
      {
        error_rtlb.maxError = error_ltrb.maxError * 1.001f;
        error_rtlb.rms = error_ltrb.rms * 1.001f;
      }
    }
  }
  // use marching quads to find best intersection angle (visible change of shore angle is worser than any other metric)
  // todo: detect ridges and use marching quads to find best intersection angle with 'ridge'
  error_ltrb.calcShore();
  error_rtlb.calcShore();
  Point2 waterContour[4];
  const int edges = march_square(x, y, x + quad_size_x, y + quad_size_y, lt, rt, lb, rb, midActualValue, water_level, waterContour);
  if (edges)
  {
    Point2 dir = normalize(waterContour[1] - waterContour[0]);
    float ltRbAngle = acos_fast_unsafe(fabsf(dir.x + dir.y) * sqrtf(0.5f));
    float rtLbAngle = acos_fast_unsafe(fabsf(dir.x - dir.y) * sqrtf(0.5f));
    if (edges == 2)
    {
      dir = normalize(waterContour[3] - waterContour[2]);
      ltRbAngle += acos_fast_unsafe(fabsf(dir.x + dir.y) * sqrtf(0.5f));
      rtLbAngle += acos_fast_unsafe(fabsf(dir.x - dir.y) * sqrtf(0.5f));
    }
    if (debug_position && debug_quad && debug_quad_level)
    {
      for (int i = 0; i < edges * 2; ++i)
        debug_water_edges[i] =
          Point3::xVy(h.worldPosOfs + h.hmapCellSize * waterContour[i], water_level * h.getHeightScale() / 65535. + h.getHeightMin());
      debug_water_edges_count = edges;
    }
    const bool waterContourAngleLtRbIsBetter = (ltRbAngle < rtLbAngle), ltRbIsBetter = error_ltrb < error_rtlb;
    if (ltRbIsBetter != waterContourAngleLtRbIsBetter)
    {
      if (waterContourAngleLtRbIsBetter)
      {
        error_rtlb.maxError = error_ltrb.maxError * 1.01f;
        error_rtlb.rms = error_ltrb.rms * 1.01f;
        error_rtlb.flags |= error_ltrb.flags & error_ltrb.SHORE;
      }
      else
      {
        error_ltrb.maxError = error_rtlb.maxError * 1.01f;
        error_ltrb.rms = error_rtlb.rms * 1.01f;
        error_ltrb.flags |= error_rtlb.flags & error_rtlb.SHORE;
      }
    }
  }
  if (debug_position && debug_quad)
  {
    debug("final err_ltrb = %f %f, err_rtlb %f %f lt_rb<rt_lb= %d", error_ltrb.maxError, error_ltrb.getRMS(), error_rtlb.maxError,
      error_rtlb.getRMS(), error_ltrb < error_rtlb);

    debug("quad = %f %f, err_rtlb %f %f lt_rb<rt_lb= %d shore %d %d", error_ltrb.maxError, error_ltrb.getRMS(), error_rtlb.maxError,
      error_rtlb.getRMS(), error_ltrb < error_rtlb, error_ltrb.isShore(), error_rtlb.isShore());
  }
}

enum
{
  NOT_ALIGNED = 0,
  ALIGNED = 1,
  ALIGNED_AND_LAST = 2
};

static inline void get_heights(const HeightmapHandler &h, float x_, float y_, float quad_size_x, uint8_t is_aligned,
  dag::Span<float> to)
{
  int x = x_, y = y_;
  y = get_addressed(h, y, h.getHeightmapSizeY());
  if (is_aligned == ALIGNED_AND_LAST)
  {
    const int xs = max(x, 0), xe = x + to.size();
    const int w = min(h.getHeightmapSizeX() - xs, xe - xs);
    h.getCompressedData().iteratePixels(xs, y, w, 1, [&](int xi, int, uint16_t v) { to[xi - x] = v; });
    if (x < 0)
    {
      G_ASSERT(x == -1);
      to[0] = h.getCompressedData().decodePixelUnsafe(get_addressed(h, x, h.getHeightmapSizeX()), y);
    }

    for (int xi = xs + w; xi < xe; xi++)
      to[xi - x] = h.getCompressedData().decodePixelUnsafe(get_addressed(h, xi, h.getHeightmapSizeX()), y);
  }
  else if (is_aligned)
  {
    const int step = quad_size_x;
    int xi = x;
    G_ASSERT(-xi <= (int)to.size() * step);
    const int xe = xi + to.size() * step;
    int i = 0;
    for (; xi < 0; xi += step, ++i)
      to[i] = h.getCompressedData().decodePixelUnsafe(get_addressed(h, xi, h.getHeightmapSizeX()), y);
    for (int xie = min(xe, h.getHeightmapSizeX()); xi < xie; xi += step, ++i)
      to[i] = h.getCompressedData().decodePixelUnsafe(xi, y);
    for (; xi < xe; xi += step, ++i)
      to[i] = h.getCompressedData().decodePixelUnsafe(get_addressed(h, xi, h.getHeightmapSizeX()), y);
  }
  else
  {
    float xi = x_;
    for (int i = 0; i < to.size(); i++, xi += quad_size_x)
      to[i] = get_interpolated_heightmap(h, xi, y_);
  }
}

static uint8_t calc_patch_error(uint8_t *exact_order, const HeightmapHandler &h, float patch_size_texels_x, float patch_size_texels_y,
  float x, float y, uint16_t dim, HeightmapErrorRet &error, HeightmapErrorRet &best, float water_level)
{
  x *= patch_size_texels_x;
  y *= patch_size_texels_y;
  const float quad_size_x = float(patch_size_texels_x) / dim, quad_size_y = float(patch_size_texels_y) / dim;
  const bool isAligned = x == floorf(x) && y == floorf(y) && quad_size_x == floorf(quad_size_x) && quad_size_y == floorf(quad_size_x);
  const uint8_t alignedType = isAligned ? (quad_size_x == 1 ? ALIGNED_AND_LAST : ALIGNED) : NOT_ALIGNED;
  if (debug_position && debug_patch)
    debug("patch at (%fx%f)*quad = %fx%f: dim %d quad_size_x = %fx%f", x / quad_size_x, y / quad_size_y, x, y, dim, quad_size_x,
      quad_size_y);
  const bool useGradients = alignedType == ALIGNED_AND_LAST && quad_size_x == quad_size_y;
  const uint32_t w = useGradients ? dim + 3 : dim + 1;
  float *vertsHeight = (float *)alloca(w * (useGradients ? 4 : 2) * sizeof(float));
  int curPtr = 1;
  uint32_t rowOfs[4] = {0 + 1, w + 1, w * 2 + 1, w * 3 + 1};

  float startX = x;
  float startY = y;
  if (useGradients)
  {
    startY = y + quad_size_y;
    startX = x - quad_size_x;
    get_heights(h, startX, y - quad_size_y, quad_size_x, alignedType, dag::Span(vertsHeight, w));
    get_heights(h, startX, y, quad_size_x, alignedType, dag::Span(vertsHeight + w, w));
    get_heights(h, startX, startY, quad_size_x, alignedType, dag::Span(vertsHeight + w * 2, w));
  }
  else
    get_heights(h, startX, y, quad_size_x, alignedType, dag::Span(vertsHeight, w));
  HeightmapErrorRet errors[TRI_COUNT];

  for (int j = 0, bitI = 0; j < dim; j++)
  {
    startY += quad_size_y;
    float *prevLine, *curLine, *nextLine, *nextNextLine;
    if (useGradients)
    {
      prevLine = vertsHeight + rowOfs[(curPtr + 3) & 3];
      curLine = vertsHeight + rowOfs[curPtr];
      nextLine = vertsHeight + rowOfs[(curPtr + 1) & 3];
      nextNextLine = vertsHeight + rowOfs[(curPtr + 2) & 3];
      get_heights(h, startX, startY, quad_size_x, alignedType, dag::Span(nextNextLine - 1, w));
      curPtr = (curPtr + 1) & 3;
    }
    else
    {
      curLine = vertsHeight + (curPtr ? 0 : w);
      nextLine = vertsHeight + (curPtr ? w : 0);
      prevLine = nextNextLine = nextLine; // to avoid incrementing nullptr
      get_heights(h, startX, startY, quad_size_x, alignedType, dag::Span(nextLine, w));
      curPtr = 1 - curPtr;
    }
    for (int i = 0; i < dim; i++, bitI++, prevLine++, curLine++, nextLine++, nextNextLine++)
    {
      HeightmapErrorRet errorLTRB, errorRTLB;
      calc_quad_error(h, quad_size_x, quad_size_y, x + i * quad_size_x, y + j * quad_size_y, errorLTRB, errorRTLB, curLine[0],
        curLine[1], nextLine[0], nextLine[1], useGradients ? prevLine : nullptr, curLine, nextLine, nextNextLine, water_level);
      const bool preferLtRb = (errorLTRB < errorRTLB);
      if (!preferLtRb)
        exact_order[bitI >> 3] |= 1u << (bitI & 7u);
      add_error(best, preferLtRb ? errorLTRB : errorRTLB);
      add_error(errors[TRI_DIAMOND_FIRST_RTLB], ((i + j) & 1) ? errorLTRB : errorRTLB);
      add_error(errors[TRI_DIAMOND_FIRST_LTRB], ((i + j) & 1) ? errorRTLB : errorLTRB);
      add_error(errors[TRI_REGULAR_RTLB], errorRTLB);
      add_error(errors[TRI_REGULAR_LTRB], errorLTRB);
    }
  }

#if 0
  if (uint32_t(select_tri) < uint32_t(TRI_COUNT))
  {
    error = errors[select_tri];
    return select_tri;
  }
#endif

  int bestTri = 0;
  error = errors[0];
  for (int i = 1; i < TRI_COUNT; ++i)
  {
    if (errors[i] < error)
      error = errors[bestTri = i];
  }
  return bestTri;
}

static inline HeightBounds combine(const HeightBounds &a, const HeightBounds &b)
{
  HeightBounds h;
  h.hMin = min(a.hMin, b.hMin);
  h.hMax = max(a.hMax, b.hMax);
  return h;
}

static void calc_lod_error(const HeightmapHandler &h, uint16_t level, uint16_t dim, hmap_err_t *dest_err, const hmap_err_t *higher_err,
  const hmap_err_t *highest_err, uint8_t *dest_order, uint8_t *exact_order, int si, float water_level, hmap_err_t shore_error,
  const LodsRiverMasks &river_masks)
{
  const float szx = float(h.getHeightmapSizeX()) / (1 << level), szy = float(h.getHeightmapSizeY()) / (1 << level);
  HeightmapErrorRet best, actual;
  const uint32_t bytesPerPatch = exact_order ? ((dim * dim) >> 3) : 0;
  if (exact_order)
    memset(exact_order, 0, (bytesPerPatch << (level + level)));
  hmap_err_t minErr = 65535, maxErr = 0;
  debug_quad_level = level == DEBUG_QUAD_LEVEL;
  const int dimLod = get_log2i(dim);
  int riverPatches = 0;

  for (int y = 0, ye = 1 << level; y < ye; y++)
    for (int x = 0, xe = 1 << level; x < xe; x++, ++si)
    {
      // should_d = level == 2 && x == 1 && y == 1;
      Point2 p(x, y);
      Point2 patchSz(szx * h.hmapCellSize, szy * h.hmapCellSize), lt = h.worldPosOfs + mul(patchSz, p);
      debug_patch = debug_position && (BBox2(lt, lt + patchSz) & debug_point);
      HeightmapErrorRet quadOrderErr, exactOrderErr;
      const uint8_t order = calc_patch_error(exact_order, h, szx, szy, p.x, p.y, dim, quadOrderErr, exactOrderErr, water_level);
      if (exact_order)
        exact_order += bytesPerPatch;
      add_error(actual, quadOrderErr);
      add_error(best, exactOrderErr);

      auto &useErr = exact_order ? exactOrderErr : quadOrderErr;
      if (dest_order)
        *(dest_order++) = order;
      if (dest_err)
      {
        auto cErr = useErr.maxError;                      //+ rms?
        if (river_masks.isRiverCriticalPath(x, y, level)) // if (useErr.isShore())
        {
          ++riverPatches;
          cErr += shore_error;
        }

        // todo:
        hmap_err_t writeErr;
        if (eastl::is_same<hmap_err_t, float>())
          writeErr = cErr;
        else
          writeErr = clamp<uint32_t>(cErr + 0.5f, 0, eastl::numeric_limits<hmap_err_t>::max());
#if 1
        if (higher_err)
        {
          uint32_t higherAt = ((y << 1) << (level + 1)) + (x << 1);
          hmap_err_t err = max(higher_err[higherAt], higher_err[higherAt + 1]);
          higherAt += 1 << (level + 1);
          err = max(err, max(higher_err[higherAt], higher_err[higherAt + 1]));
          writeErr = max<hmap_err_t>(writeErr, err);
        }
#endif
#if 1
        // this is to save from cracks/t-junctions on error discontinuity
        // which otherwise have to be fixed in runtime (with sewing)
        if (highest_err)
        {
          uint32_t highestLod = (level + dimLod);
          uint32_t higherAt = ((y << dimLod) << highestLod) + (x << dimLod);
          hmap_err_t err = 0;
          for (int i = 0; i < dim; ++i)
          {
            if (x > 0)
              err = max(err, highest_err[higherAt - 1 + (i << highestLod)]);
            if (x < xe - 1)
              err = max(err, highest_err[higherAt + dim + (i << highestLod)]);
            if (y > 0)
              err = max(err, highest_err[higherAt - (1 << highestLod) + i]);
            if (y < ye - 1)
              err = max(err, highest_err[higherAt + (dim << highestLod) + i]);
          }
          writeErr = max<hmap_err_t>(writeErr, err);
        }
#endif
        *(dest_err++) = writeErr;
        minErr = min(minErr, writeErr);
        maxErr = max(maxErr, writeErr);
      }
      if (debug_position && debug_patch)
        debug("%d:%dx%d error %d (%g) order %d %@", level, x, y, useErr.maxError, dest_err ? dest_err[-1] : -1, order,
          BBox2(lt, lt + patchSz));
    }
  debug("lod %d: patch_size %@ %@ error %f (%f) best %f (%f) minErr = %@ / %@, rivers %d", level, szx, szy, actual.maxError,
    actual.getRMS(), best.maxError, best.getRMS(), minErr, maxErr, riverPatches);
}

static inline bool not_equal(const HeightBounds &a, const HeightBounds &b) { return a.hMin != b.hMin || a.hMax != b.hMax; }

void MetricsErrors::updateHeightBounds(const HeightmapHandler &h, const IBBox2 &texel_box, bool force_update)
{
  if (heightBounds.empty())
    return;
  const int topPatchW = h.getHeightmapSizeX() >> maxBoundsLevel, topPatchH = h.getHeightmapSizeY() >> maxBoundsLevel;
  const IPoint2 lt(texel_box[0].x / topPatchW, texel_box[0].y / topPatchH);
  const IPoint2 rb(texel_box[1].x / topPatchW, texel_box[1].y / topPatchH);
  auto heightBoundTopLevel = heightBounds.data() + get_level_ofs(maxBoundsLevel);
  IBBox2 changedTopBox = force_update ? IBBox2{lt, rb} : IBBox2();
  for (int j = lt.y, y = j * topPatchH; j <= rb.y; ++j, y += topPatchH)
  {
    HeightBounds *ohb = heightBoundTopLevel + (j << maxBoundsLevel) + lt.x;
    for (int i = lt.x, x = i * topPatchW; i <= rb.x; ++i, x += topPatchW, ohb++)
    {
      HeightBounds hb = {65535, 0};
      // this assumes clamping, not mirroring. in fact mirror will behave exactly same, as last row/column is same as before last,
      // which is already part of bounds
      h.getCompressedData().iteratePixels(x, y, min(topPatchW + 1, h.getHeightmapSizeX() - x),
        min(topPatchH + 1, h.getHeightmapSizeY() - y), [&](int, int, uint16_t v) {
          hb.hMin = min(hb.hMin, v);
          hb.hMax = max(hb.hMax, v);
        });

      if (!force_update && not_equal(*ohb, hb))
        changedTopBox += IPoint2(i, j);
      *ohb = hb;
    }
  }

  for (int i = maxBoundsLevel - 1; i >= 0; --i)
  {
    if (changedTopBox.isEmpty())
      break;
    const IBBox2 changedBox = {changedTopBox[0] >> 1, changedTopBox[1] >> 1};
    auto heightBoundLevel = heightBounds.data() + get_level_ofs(i) + (changedBox[0].y << i) + changedBox[0].x;
    auto heightBoundHigherLevel = heightBounds.data() + get_level_ofs(i + 1) + (changedBox[0].y << (i + 2)) + (changedBox[0].x << 1);
    const int higherStride = 1 << (i + 1);
    const int strideReminder = (1 << i) - (changedBox[1].x - changedBox[0].x + 1);
    changedTopBox = force_update ? changedBox : IBBox2();
    for (int y = changedBox[0].y; y <= changedBox[1].y; y++, heightBoundHigherLevel += higherStride)
    {
      for (int x = changedBox[0].x; x <= changedBox[1].x; x++, heightBoundLevel++, heightBoundHigherLevel += 2)
      {
        HeightBounds hb = combine(combine(heightBoundHigherLevel[0], heightBoundHigherLevel[1]),
          combine(heightBoundHigherLevel[higherStride], heightBoundHigherLevel[higherStride + 1]));
        if (!force_update && not_equal(*heightBoundLevel, hb))
          changedTopBox += IPoint2(x, y);
        *heightBoundLevel = hb;
      }
      heightBoundLevel += strideReminder;
      heightBoundHigherLevel += strideReminder << 1;
    }
  }
}

void MetricsErrors::recalc_lod_errors(const HeightmapHandler &h, uint16_t min_level, uint16_t max_err_level, uint16_t max_order_level,
  uint16_t dim_, float water_level, float shore_error_meters_at_best)
{
  dim = dim_;
  minLevel = min_level;
  minLevelOfs = get_level_ofs(min_level);
  data.resize(get_level_ofs(max_err_level) - minLevelOfs);
  order.resize(get_level_ofs(max_order_level) - minLevelOfs);
  const uint32_t bytesPerPatch = dim * dim / 8;
  exactOrder.resize((get_level_ofs(max_order_level) - minLevelOfs) * bytesPerPatch);

  static constexpr int maxCullingLevel = 10;
  maxBoundsLevel = min<int>(get_log2i(max(h.getHeightmapSizeX(), h.getHeightmapSizeY()) / dim), maxCullingLevel);
  heightBounds.resize(get_level_ofs(maxBoundsLevel + 1));

  maxErrLevel = max_err_level;
  maxOrderLevel = max_order_level;
  szX = h.getHeightmapSizeX();
  szY = h.getHeightmapSizeY();
  G_ASSERT((dim * dim) % 8 == 0);
  // for (int i = min_level, ie = max(max_order_level, max_err_level); i < ie; ++i)
  const hmap_err_t *prevErr = nullptr;
  int64_t reft = profile_ref_ticks();
  const int dimLod = get_log2i(dim);
  G_ASSERT(get_level_ofs(max_order_level) - minLevelOfs == order.size());
  for (int is = max(max_order_level, max_err_level), ie = min_level, i = is - 1; i >= ie; --i)
  {
    const uint32_t highestOfs = get_level_ofs(i + dimLod) - minLevelOfs, highestSz = (1 << (i + i + dimLod + dimLod));
    const uint32_t levelOfs = get_level_ofs(i), sz = (1 << (i + i));
    const uint32_t ofs = levelOfs - minLevelOfs;
    G_ASSERT(ofs + sz <= order.size());
    // debug("level %d ofs %d (%d)", i, ofs, 1 << (i + i));
    hmap_err_t *err = ofs + sz <= data.size() ? data.data() + ofs : nullptr;
    auto pOrder = ofs + sz <= order.size() ? order.data() + ofs : nullptr;

    int64_t reft = profile_ref_ticks();
    const float shoreMeters = shore_error_meters_at_best * (is - i);
    const hmap_err_t fixedShoreError = shoreMeters / h.getHeightScaleRaw();
    calc_lod_error(h, i, dim, err, prevErr, highestOfs + highestSz <= data.size() ? data.data() + highestOfs : nullptr, pOrder,
      pOrder ? exactOrder.data() + ofs * bytesPerPatch : nullptr, ofs, water_level, fixedShoreError, lodsRiverMasks);
    prevErr = err;
    debug("level %d in %fms hasErr=%d hasHighErr= %d, shore error %f (%d)", i, profile_time_usec(reft) / 1000.f, bool(err),
      highestOfs + highestSz <= data.size(), shoreMeters, fixedShoreError);
  }

  updateHeightBounds(h, IBBox2{{0, 0}, {h.getHeightmapSizeX() - 1, h.getHeightmapSizeY() - 1}}, true);

  debug("all levels %d in %fms", max(max_order_level, max_err_level) - min_level, profile_time_usec(reft) / 1000.f);
  debug("exactOrder %d bytes err %d bytes bounds bytes %d", exactOrder.size(), data.size() * sizeof(hmap_err_t),
    heightBounds.size() * sizeof(HeightBounds));
}

void MetricsErrors::calc_lod_errors(const HeightmapHandler &h, uint16_t min_level, uint16_t max_level, uint16_t dim_,
  float water_level, float shore_error_at_best_lod)
{
  int res = max(h.getHeightmapSizeX(), h.getHeightmapSizeY()) / dim_;
  uint8_t maxResShiftValue = get_bigger_log2(res);
  max_level = min<uint32_t>(max_level, maxResShiftValue);
  if (maxErrLevel == max_level && data.size() == get_level_ofs(max_level) - get_level_ofs(min_level) &&
      order.size() == get_level_ofs(max_level + 1) - get_level_ofs(min_level) && dim == dim_)
    return;
  debug("levels %d..%d dim %d res = %d, water level = %f (encoded = %f) encoding %f %f + %f", min_level, max_level, dim_, res,
    water_level, (water_level - h.getHeightMin()) / h.getHeightScaleRaw(), h.getHeightScaleRaw() * 65535., h.getHeightScale(),
    h.getHeightMin());
  debug("worldSize %@ worldPosOfs = %@, clip %@", h.worldSize, h.worldPosOfs, h.worldBox2);
  maxResShift = maxResShiftValue;
  boundsHeightScale = h.getHeightScaleRaw();
  boundsHeightOffset = h.getHeightMin();
  errorHeightScale = h.getHeightScaleRaw();

  // Build LodsRiverMasks from heightmap water connectivity
  {
    const int w = max(h.getHeightmapSizeX(), h.getHeightmapSizeY());
    const float encodedWaterLevel = (water_level - h.getHeightMin()) / h.getHeightScaleRaw();

    int bitmask_words = (w * w + 31) / 32;
    dag::Vector<uint32_t> water_mask(bitmask_words, 0);

    bool hasWater = false;
    h.getCompressedData().iteratePixels(0, 0, h.getHeightmapSizeX(), h.getHeightmapSizeY(), [&](int x, int y, uint16_t v) {
      int idx = y * w + x;
      if ((float)v < encodedWaterLevel)
      {
        water_mask[idx >> 5] |= 1u << (idx & 31);
        hasWater = true;
      }
    });

    if (!hasWater)
    {
      lodsRiverMasks.clear();
    }
    else
    {
      dag::Vector<float> dist;
      dist.resize_noinit(w * w);
      int64_t t0 = profile_ref_ticks();
      sdf::build_edt(water_mask.data(), w, dist.data());
      debug("river masks: sdf::build_edt %fms", profile_time_usec(t0) / 1000.f);

      dag::Vector<uint32_t> critical_path;
      t0 = profile_ref_ticks();
      static constexpr float SDF_CONTOUR_DIST = 64.0f;
      static constexpr int SDF_MIN_BRANCH_LEN = 50;
      static constexpr int SDF_MIN_COMP_SIZE = 30;
      int path_count = hmap_sdf_extract_critical_path(make_span_const(dist), make_span_const(water_mask), w, SDF_CONTOUR_DIST,
        SDF_MIN_BRANCH_LEN, SDF_MIN_COMP_SIZE, critical_path);
      debug("river masks: critical_path %fms, %d pixels", profile_time_usec(t0) / 1000.f, path_count);

      t0 = profile_ref_ticks();
      hmap_sdf_build_lod_masks(critical_path.data(), w, get_log2i(dim_), lodsRiverMasks);
      debug("river masks: build_lod_masks %fms, finestLod=%d", profile_time_usec(t0) / 1000.f, lodsRiverMasks.finestLod);
    }
  }

  recalc_lod_errors(h, min_level, max_level, max_level + 1, dim_, (water_level - h.getHeightMin()) / errorHeightScale,
    shore_error_at_best_lod);
}
