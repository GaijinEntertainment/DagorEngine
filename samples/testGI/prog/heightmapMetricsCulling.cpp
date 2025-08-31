// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/lodGrid.h>
#include <heightmap/heightmapRenderer.h>
#include <heightmap/heightmapHandler.h>
#include <frustumCulling/frustumPlanes.h>
#include <memory/dag_framemem.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_hashedKeyMap.h>
#include <EASTL/bitvector.h>
#include <util/dag_conVar.h>
#include <generic/dag_relocatableFixedVector.h>
#include <drv/3d/dag_matricesAndPerspective.h>

// + cache metrics error
// + calc metrics using triangulation
// + there are now 4 options with indexed base: two diamond and two regular (than 4 drawIndexed calls),
//   almost no cost whatsoever (2 bits per patch, 4 drawcalls instead of one).
// todo:
// * water metrices change
// * do not allow neighboors to have more lod dsit than there is log2(dimension) (i.e. for dim = 16 max possible lod dist is 4, it is
// 'sew' lod
//   10 with 5, for example)
// * sort front-to-back
// * compress metrics (quantized to 8 bit is probably enough, if we have per-lod multiple of errorScale)
// * enhance silhoette changes (based on ridge, slope, and laplacians curvature).
// * increase error with quad diagonal intersecting closest ridge angle (fixes zig-zags)
// * support displacement dist
// * occlusion only for levels that makes sense (auto some it is same occusion hzb pixels as previous)
// compare perf
// cleanup
// mirror
// change patch triangulation to optimal based on max error metric.
// * also we can store in 256 bit _exact_ triangulation for all quads per patch (and then sample them in VS).
//   This option is a bit more costly, but memory-wise irrelevant (we would store like 64kb), the only question is the perf cost. To be
//   measured.

static inline uint32_t edge_tess_encode(int lod_difference) { return clamp(lod_difference, 0, 15); }

static inline float float_pack_edges_tess(int edgeTessOut[4])
{
  // Pack 4 x 4bit = 16 bit of information in a integer and then static_cast it as float. Shader unpacks edge tesselation.
  // Note: float has 23 bit of mantissa + 1 implicit bit. Thus, up to 24 bits can be packed using this trick.
  int packed_edge_tess = edgeTessOut[0] | (edgeTessOut[1] << 4) | (edgeTessOut[2] << 8) | (edgeTessOut[3] << 12);
  return float(packed_edge_tess);
}

struct HeightmapErrorRet
{
  float maxError = 0;
  float rms = 0;
  int cnt = 0;
  float getRMS() const { return cnt > 0 ? sqrtf(rms / cnt) : 0; }
  Point2 maxErrorPos = {-1, -1};
};

static float get_interpolated_heightmap(const HeightmapHandler &h, float x, float y)
{
  IPoint2 coord(floorf(x), floorf(y));
  Point2 coordF(x - coord.x, y - coord.y);
  const CompressedHeightmap &c = h.getCompressedData();
  float vals[4];
  // todo: add mirror support
  vals[0] = c.decodePixelUnsafe(clamp(coord.x, 0, h.getHeightmapSizeX() - 1), clamp(coord.y, 0, h.getHeightmapSizeY() - 1));
  vals[1] = c.decodePixelUnsafe(min(coord.x + 1, h.getHeightmapSizeX() - 1), clamp(coord.y, 0, h.getHeightmapSizeY() - 1));
  vals[2] = c.decodePixelUnsafe(clamp(coord.x, 0, h.getHeightmapSizeX() - 1), min(coord.y + 1, h.getHeightmapSizeY() - 1));
  vals[3] = c.decodePixelUnsafe(min(coord.x + 1, h.getHeightmapSizeX() - 1), min(coord.y + 1, h.getHeightmapSizeY() - 1));
  return lerp(lerp(vals[0], vals[1], coordF.x), lerp(vals[2], vals[3], coordF.x), coordF.y);
}

static Point2 debug_point(14450, 8898);

static constexpr bool debug_position = false;
static bool debug_patch = false, debug_quad = false;

static inline void add_error(HeightmapErrorRet &error, float triVal, float hmapVal, const Point2 &at, float water_level)
{
  float err = fabsf(triVal - hmapVal);
  if ((triVal < water_level && hmapVal > water_level) || (triVal > water_level && hmapVal < water_level))
    err += 2000; // todo: should be explicit value
  if (err > error.maxError)
  {
    error.maxErrorPos = at;
    error.maxError = err;
  }
  error.rms += err * err;
  error.cnt++;
}

static inline void add_error(HeightmapErrorRet &a, HeightmapErrorRet &b, float triVal, float hmapVal, const Point2 &at,
  float water_level)
{
  add_error(a, triVal, hmapVal, at, water_level);
  add_error(b, triVal, hmapVal, at, water_level);
}

static inline void add_error(HeightmapErrorRet &error, const HeightmapErrorRet &err)
{
  if (err.maxError > error.maxError)
  {
    error.maxErrorPos = err.maxErrorPos;
    error.maxError = err.maxError;
  }
  error.rms += err.rms;
  error.cnt += err.cnt;
}


static inline bool operator<(const HeightmapErrorRet &a, const HeightmapErrorRet &b)
{
  return a.maxError + a.getRMS() < b.maxError + b.getRMS();
}


static void calc_quad_error(const HeightmapHandler &h, float quad_size_x, float quad_size_y, float x, float y,
  HeightmapErrorRet &error_ltrb, HeightmapErrorRet &error_rtlb, const float *first, const float *second,
  float water_level) // two diagonals triangulation
{
  const float lt = first[0], rt = first[1], lb = second[0], rb = second[1];
  if (debug_position)
  {
    Point2 quadSz(quad_size_x * h.hmapCellSize, quad_size_x * h.hmapCellSize), lt = h.worldPosOfs + h.hmapCellSize * Point2(x, y);
    debug_quad = (BBox2(lt, lt + quadSz) & debug_point);
    if (debug_quad && !debug_patch)
    {
      debug("%d != %d quad position %fx%f (sz = %fx%f) lt = %@..%@", debug_quad, debug_patch, x, y, quad_size_x, quad_size_y, lt,
        quadSz);
      debug_quad = false;
    }
  }

  const Point2 c(x + quad_size_x * 0.5f, y + quad_size_y * 0.5f);
  float mid = get_interpolated_heightmap(h, c.x, c.y);
  if (c.x != floorf(c.x) || c.y != floorf(c.y))
  {
    add_error(error_rtlb, (rt + lb) * 0.5f, mid, c, water_level);
    add_error(error_ltrb, (lt + rb) * 0.5f, mid, c, water_level);
  }

  const float quad_w = 1.f / quad_size_x, quad_h = 1.f / quad_size_y;
  const int l = ceilf(x), t = ceilf(y);
  const int r = clamp<int>(floorf(x + quad_size_x), 0, h.getHeightmapSizeX()),
            b = clamp<int>(floorf(y + quad_size_y), 0, h.getHeightmapSizeY());
  // if (l == r || t == b || l != x || quad_size_y != floorf(quad_size_y))
  {
    add_error(error_ltrb, error_rtlb, (lt + lb) * 0.5f, get_interpolated_heightmap(h, c.x, c.y + quad_size_y * 0.5f), c, water_level);
    add_error(error_ltrb, error_rtlb, (lt + rt) * 0.5f, get_interpolated_heightmap(h, c.x + quad_size_x * 0.5f, c.y), c, water_level);
    add_error(error_ltrb, error_rtlb, (rt + rb) * 0.5f, get_interpolated_heightmap(h, c.x + quad_size_x, c.y + quad_size_y * 0.5f), c,
      water_level);
    add_error(error_ltrb, error_rtlb, (lb + rb) * 0.5f, get_interpolated_heightmap(h, c.x + quad_size_x * 0.5f, c.y + quad_size_y), c,
      water_level);
  }
  if (debug_position && debug_quad)
  {
    debug("quad position %fx%f (sz = %fx%f) c = %@ l,t %dx%d r,b = %dx%d", x, y, quad_size_x, quad_size_y, c, l, t, r, b);
  }

  // if (l < r && b < t)
  if (r == l || b == t)
    return;
  h.getCompressedData().iteratePixels(l, t, r - l, b - t, [&](int xi, int yi, uint16_t v) {
    float xPos = xi - x, yPos = yi - y;
    float xf = xPos * quad_w, yf = yPos * quad_h;

    const float triValueLTRB = (xf <= yf) ? lt * (1.0f - yf) + lb * (yf - xf) + rb * xf : lt * (1.0f - xf) + rt * (xf - yf) + rb * yf;

    const float triValueRTLB =
      (xf <= 1.0f - yf) ? lt * (1.0f - yf - xf) + lb * yf + rt * xf : rt * (1.0f - yf) + rb * (xf + yf - 1.0f) + lb * (1.0f - xf);

    add_error(error_rtlb, triValueRTLB, v, Point2(xi, yi), water_level);
    add_error(error_ltrb, triValueLTRB, v, Point2(xi, yi), water_level);
  });
}

enum
{
  TRI_DIAMOND_FIRST_RTLB,
  TRI_DIAMOND_FIRST_LTRB,
  TRI_REGULAR_RTLB,
  TRI_REGULAR_LTRB,
  TRI_COUNT
};

static uint8_t calc_patch_error(const HeightmapHandler &h, float patch_size_texels_x, float patch_size_texels_y, float x, float y,
  uint16_t dim, HeightmapErrorRet &error, HeightmapErrorRet &best, int select_tri, float water_level)
{
  x *= patch_size_texels_x;
  y *= patch_size_texels_y;
  const float quad_size_x = float(patch_size_texels_x) / dim, quad_size_y = float(patch_size_texels_y) / dim;
  if (debug_position && debug_patch)
    debug("patch at %fx%f: dim %d quad_size_x = %fx%f", x, y, dim, quad_size_x, quad_size_y);
  dag::RelocatableFixedVector<float, 64, true, framemem_allocator, uint32_t, false> vertsHeight((dim + 1) * 2);
  int firstRowOfs = 0, secondRowOfs = dim + 1;
  for (int i = 0; i <= dim; i++)
    vertsHeight[i] = get_interpolated_heightmap(h, x + i * quad_size_x, y);

  HeightmapErrorRet errors[TRI_COUNT];
  for (int j = 0; j < dim; j++)
  {
    for (int i = 0; i <= dim; i++)
      vertsHeight[secondRowOfs + i] = get_interpolated_heightmap(h, x + i * quad_size_x, y + (j + 1) * quad_size_y);
    for (int i = 0; i < dim; i++)
    {
      HeightmapErrorRet errorLTRB, errorRTLB;
      calc_quad_error(h, quad_size_x, quad_size_y, x + i * quad_size_x, y + j * quad_size_y, errorLTRB, errorRTLB,
        &vertsHeight[firstRowOfs + i], &vertsHeight[secondRowOfs + i], water_level);
      add_error(best, (errorLTRB < errorRTLB) ? errorLTRB : errorRTLB);
      add_error(errors[TRI_DIAMOND_FIRST_RTLB], ((i + j) & 1) ? errorLTRB : errorRTLB);
      add_error(errors[TRI_DIAMOND_FIRST_LTRB], ((i + j) & 1) ? errorRTLB : errorLTRB);
      add_error(errors[TRI_REGULAR_RTLB], errorRTLB);
      add_error(errors[TRI_REGULAR_LTRB], errorLTRB);
    }
    eastl::swap(firstRowOfs, secondRowOfs);
  }

  if (uint32_t(select_tri) < uint32_t(TRI_COUNT))
  {
    error = errors[select_tri];
    return select_tri;
  }

  int bestTri = 0;
  error = errors[0];
  for (int i = 1; i < TRI_COUNT; ++i)
  {
    if (errors[i] < error)
      error = errors[bestTri = i];
  }
  return bestTri;
}

// todo: store exp8 error may be?
static inline uint32_t get_level_ofs(uint32_t rev_level) { return uint32_t((1u << (rev_level << 1u)) - 1u) / 3u; }
typedef uint16_t hmap_err_t;

static void calc_lod_error(const HeightmapHandler &h, uint16_t level, uint16_t dim, hmap_err_t *dest_err, const hmap_err_t *higher_err,
  uint8_t *dest_order, int16_t select_tri, float water_level)
{
  const float szx = float(h.getHeightmapSizeX()) / (1 << level), szy = float(h.getHeightmapSizeY()) / (1 << level);
  HeightmapErrorRet best, actual;
  for (int y = 0, ye = 1 << level; y < ye; y++)
    for (int x = 0, xe = 1 << level; x < xe; x++)
    {
      // should_d = level == 2 && x == 1 && y == 1;
      Point2 patchSz(szx * h.hmapCellSize, szy * h.hmapCellSize), lt = h.worldPosOfs + mul(patchSz, Point2(x, y));
      debug_patch = debug_position && (BBox2(lt, lt + patchSz) & debug_point);
      HeightmapErrorRet error;
      const uint8_t order = calc_patch_error(h, szx, szy, x, y, dim, error, best, select_tri, water_level);
      add_error(actual, error);
      if (dest_order)
        *(dest_order++) = order;
      if (dest_err)
      {
        auto cErr = error.maxError; // + error.rms
        hmap_err_t writeErr;

        if (eastl::is_same<hmap_err_t, float>())
          writeErr = cErr;
        else
          writeErr = clamp<uint32_t>(cErr + 0.5f, 0, eastl::numeric_limits<hmap_err_t>::max());
        if (higher_err)
        {
          uint32_t higherAt = (y << (level + 1)) + x;
          writeErr = max(writeErr, max(higher_err[higherAt], higher_err[higherAt + 1]));
          higherAt += 1 << (level + 1);
          writeErr = max(writeErr, max(higher_err[higherAt], higher_err[higherAt + 1]));
        }
        *(dest_err++) = writeErr;
      }
      if (debug_position && debug_patch)
        debug("%d:%dx%d error %d (%g) order %d %@", level, x, y, error.maxError, dest_err ? dest_err[-1] : -1, order,
          BBox2(lt, lt + patchSz));
    }
  debug("lod %d: patch_size %@ %@ error %f (%f) order %d best %f (%f)", level, szx, szy, actual.maxError, actual.getRMS(), select_tri,
    best.maxError, best.getRMS());
}
static struct Errors
{
  dag::Vector<hmap_err_t> data;
  dag::Vector<uint8_t> order;
  uint16_t maxErrLevel = 0;
  uint16_t maxOrderLevel = 0;
  int16_t selectTri = 0;
  uint32_t minLevelOfs = 0;
} errors;

void debug_clear_errors()
{
  errors.data.clear();
  errors.order.clear();
}

static void recalc_lod_errors(const HeightmapHandler &h, uint16_t min_level, uint16_t max_err_level, uint16_t max_order_level,
  uint16_t dim, int16_t select_tri, float water_level)
{
  errors.minLevelOfs = get_level_ofs(min_level);
  errors.data.resize(get_level_ofs(max_err_level) - errors.minLevelOfs);
  errors.order.resize(get_level_ofs(max_order_level) - errors.minLevelOfs);
  errors.maxErrLevel = max_err_level;
  errors.maxOrderLevel = max_order_level;
  errors.selectTri = select_tri;
  // for (int i = min_level, ie = max(max_order_level, max_err_level); i < ie; ++i)
  const hmap_err_t *prevErr = nullptr;
  for (int i = max(max_order_level, max_err_level) - 1, ie = min_level; i >= ie; --i)
  {
    const uint32_t ofs = get_level_ofs(i) - errors.minLevelOfs, sz = (1 << (i + i));
    const uint32_t prevOfs = get_level_ofs(i + 1) - errors.minLevelOfs, prevSz = (1 << (i + i + 2));
    // debug("level %d ofs %d (%d)", i, ofs, 1 << (i + i));
    hmap_err_t *err = ofs + sz <= errors.data.size() ? errors.data.data() + ofs : nullptr;
    calc_lod_error(h, i, dim, err, prevErr, ofs + sz <= errors.order.size() ? errors.order.data() + ofs : nullptr, select_tri,
      water_level);
    prevErr = err;
  }
}

CONSOLE_INT_VAL("hmap", selectTri, -1, -1, TRI_COUNT - 1);
static void calc_lod_errors(const HeightmapHandler &h, uint16_t min_level, uint16_t max_level, uint16_t dim, float water_level)
{
  int res = max(h.getHeightmapSizeX(), h.getHeightmapSizeY()) / dim;
  max_level = min<uint32_t>(max_level, get_bigger_log2(res));
  if (errors.maxErrLevel == max_level && errors.data.size() == get_level_ofs(max_level) - get_level_ofs(min_level) &&
      errors.order.size() == get_level_ofs(max_level + 1) - get_level_ofs(min_level) && errors.selectTri == selectTri)
    return;
  debug("levels %d..%d dim %d res = %d, select %d, water level = %f", min_level, max_level, dim, res, selectTri, water_level);
  recalc_lod_errors(h, min_level, max_level, max_level + 1, dim, selectTri, (water_level - h.getHeightMin()) / h.getHeightScaleRaw());
}
#define USE_HASHMASP 0
struct RecursiveContext
{
  LodGridCullData &cull_data;
  const HeightmapHeightCulling *heightCulling = nullptr;
  Frustum frustum;
  vec3f viewPos;
  const Occlusion *use_occlusion = nullptr;
  Point2 lt;
  float hMin = 0, hMax = 1;
  float errorScale = 1;
  int minLevel = 0, maxLevel = 8;
  float heightErrorThresholdSq = 0, absHeightErrorThreshold = 0, sizeErrorThresholdSq = 0;
  bool scaleByDist = true;
  uint8_t minLODUsed = 0xFF, maxLODUsed = 0;
  void add_patch(int x, int y, uint8_t level, uint8_t order);
  struct Key
  {
    uint64_t v;
    bool operator==(Key k) const;
  };
  struct Hasher
  {
    __forceinline uint32_t hash(const Key &k) { return wyhash64_const(uint64_t(k.v), _wyp[0]); }
  };
  static constexpr Key emptyKey = Key{0};
#if USE_HASHMASP
  HashedKeySet<Key, emptyKey, Hasher, framemem_allocator> patches;
#else
  dag::Vector<Key, framemem_allocator> patches;
#endif

  enum
  {
    DIM_BITS = 28,
    DIM_SIGN_BIT = 1 << (DIM_BITS - 1),
    INT_COMPLEMENT_MASK = (~0u >> DIM_BITS) << DIM_BITS,
    DIM_MASK = (1 << DIM_BITS) - 1,
    LEVEL_OFS = DIM_BITS * 2,
    LEVEL_BITS = 5,
    LEVEL_MASK = (1 << LEVEL_BITS) - 1,
    FLIP_OFS = LEVEL_OFS + LEVEL_BITS
  };
  static constexpr uint64_t HASH_KEY_MASK = (uint64_t(1) << uint64_t(FLIP_OFS)) - uint64_t(1);
  static Key encode_patch(int x, int y, uint8_t level, uint8_t best_flip);
  static void decode_patch(Key k, int &x, int &y, uint8_t &level, uint8_t &best_flip);
};

bool RecursiveContext::Key::operator==(Key k) const { return (v & HASH_KEY_MASK) == (k.v & HASH_KEY_MASK); }

inline RecursiveContext::Key RecursiveContext::encode_patch(int x, int y, uint8_t level, uint8_t best_flip)
{
  return Key{uint64_t(x & DIM_MASK) | (uint64_t(y & DIM_MASK) << uint64_t(DIM_BITS)) | (uint64_t(level + 1) << uint64_t(LEVEL_OFS)) |
             (uint64_t(best_flip) << uint64_t(FLIP_OFS))};
}

inline void RecursiveContext::decode_patch(Key k, int &x, int &y, uint8_t &level, uint8_t &best_flip)
{
  level = ((k.v >> uint64_t(LEVEL_OFS)) & LEVEL_MASK) - 1;
  best_flip = (k.v >> uint64_t(FLIP_OFS));
  x = k.v & DIM_MASK;
  x |= (x & DIM_SIGN_BIT) ? INT_COMPLEMENT_MASK : 0;
  y = (k.v >> uint64_t(DIM_BITS)) & DIM_MASK;
  y |= (y & DIM_SIGN_BIT) ? INT_COMPLEMENT_MASK : 0;
}


#if USE_HASHMASP
template <class Cb>
static __forceinline void iterate_patches(const RecursiveContext &c, const Cb &cb)
{
  c.patches.iterate(cb);
}

inline void RecursiveContext::add_patch(int x, int y, uint8_t level, uint8_t order)
{
  patches.emplace_new(encode_patch(x, y, level, order));
}
#else
inline void RecursiveContext::add_patch(int x, int y, uint8_t level, uint8_t order)
{
  patches.emplace_back(encode_patch(x, y, level, order));
}
template <class Cb>
static __forceinline void iterate_patches(const RecursiveContext &c, const Cb &cb)
{
  for (auto key : c.patches)
    cb(key);
}
#endif

bool debug_patches;
void recursive_split(RecursiveContext &ctx, int level_x, int level_y, uint8_t lev, float patchSize, bool test_frustum,
  const Point2 *parent_ht_min_max)
{
  ctx.maxLODUsed = max(ctx.maxLODUsed, lev);
  Point2 htMinMax;
  Point2 origin = ctx.lt + Point2(level_x, level_y) * patchSize;
  int heightLod = 0;
  if (!parent_ht_min_max)
  {
    heightLod = ctx.heightCulling->getLod(patchSize);
    ctx.heightCulling->getMinMax(heightLod, origin, patchSize, htMinMax.x, htMinMax.y);
  }
  else
    htMinMax = *parent_ht_min_max;
  vec4f patch = v_make_vec4f(origin.x, origin.y, origin.x + patchSize, origin.y + patchSize);
  vec3f bmin = v_perm_xayb(patch, v_splats(htMinMax.x)), bmax = v_perm_zcwd(patch, v_splats(htMinMax.y));
  vec4f center2 = v_add(bmax, bmin);
  vec4f extent2 = v_sub(bmax, bmin);
  if (test_frustum)
  {
    auto vis = ctx.frustum.testBoxExtent(center2, extent2);
    if (vis == Frustum::OUTSIDE)
      return;
    if (vis == Frustum::INSIDE)
      test_frustum = false;
  }
  if (ctx.use_occlusion && ctx.use_occlusion->isOccludedBoxExtent2(center2, extent2))
    return;

  const uint32_t dataOfs = get_level_ofs(lev) - errors.minLevelOfs + (level_y << lev) + level_x;
  if (lev >= ctx.maxLevel)
  {
    ctx.minLODUsed = min(ctx.minLODUsed, lev);
    // debug("level %d order %d: %d", lev, dataOfs < errors.order.size(), dataOfs < errors.order.size() ? errors.order[dataOfs] : 0);
    ctx.add_patch(level_x, level_y, lev, dataOfs < errors.order.size() ? errors.order[dataOfs] : 0);
    return;
  }
  if (lev > ctx.minLevel)
  {
    float distScaleSq = 1, distSq = 1;
    // BBox2 box(origin, origin + Point2(patchSize, patchSize));
    if (ctx.scaleByDist)
    {
      // more gradual distance
      float yPos = v_extract_y(ctx.viewPos), yDist;
      if (htMinMax.x > yPos)
        yDist = htMinMax.x - yPos;
      if (htMinMax.y < yPos)
        yDist = yPos - htMinMax.y;
      else
        yDist = 0;

      float xzDist =
        max(0.f, length(origin + 0.5f * Point2(patchSize, patchSize) - Point2(v_extract_x(ctx.viewPos), v_extract_z(ctx.viewPos))) -
                   0.5f * sqrtf(2.f) * patchSize);
      distSq = xzDist * xzDist + yDist * yDist;
      // debug("level %d %dx%d, dist %f =  %f %f box %@", lev, level_x, level_y, distSq, xzDist, yDist, box);

      // distSq = v_extract_x(v_distance_sq_to_bbox_x(bmin, bmax, ctx.viewPos));
      distScaleSq = 1.f / max(1e-25f, distSq);
    }
    // todo: check if patch has displacement
    const hmap_err_t err = dataOfs < errors.data.size() ? errors.data[dataOfs] : 0;

    const float patchHeightError = err * ctx.errorScale + ctx.heightCulling->getDisplacementRange() / max(1, lev - errors.maxErrLevel);
    const float heightErrorSq = patchHeightError * patchHeightError * distScaleSq;
    // const float patchSizeErrorSq = patchSize * patchSize * distScaleSq;

    if (debug_position && BBox2(origin, origin + Point2(patchSize, patchSize)) & debug_point)
      debug("level %d: error patchHeightError %dx%d %d %@ dist %f threshold %d = %g<%g", lev, level_x, level_y, err, patchHeightError,
        sqrtf(distSq), heightErrorSq <= ctx.heightErrorThresholdSq, heightErrorSq, ctx.heightErrorThresholdSq);
    if (debug_patches)
      debug("level %d: error patchHeightError %dx%d %d %@ dist %f threshold %d = %g<%g", lev, level_x, level_y, err, patchHeightError,
        sqrtf(distSq), heightErrorSq <= ctx.heightErrorThresholdSq, heightErrorSq, ctx.heightErrorThresholdSq);
    if (heightErrorSq <= ctx.heightErrorThresholdSq && patchHeightError <= ctx.absHeightErrorThreshold) // && patchSizeErrorSq <=
                                                                                                        // ctx.sizeErrorThresholdSq)
    {
      ctx.minLODUsed = min(ctx.minLODUsed, lev);
      ctx.add_patch(level_x, level_y, lev, dataOfs < errors.order.size() ? errors.order[dataOfs] : 0);
      return;
    }
  }

  float subPatchSize = patchSize * 0.5;
  for (int i = 0; i < 4; ++i)
    recursive_split(ctx, (level_x << 1) + (i & 1), (level_y << 1) + ((i & 2) >> 1), lev + 1, subPatchSize, test_frustum,
      heightLod ? nullptr : &htMinMax);
}

CONSOLE_FLOAT_VAL_MINMAX("hmap", size_error, 200, 0, 1024);
CONSOLE_FLOAT_VAL_MINMAX("hmap", height_error, 0.07, 0, 1024);
CONSOLE_FLOAT_VAL_MINMAX("hmap", abs_height_error, 196, 0, 1024);
CONSOLE_INT_VAL("hmap", maxLevel, 9, 2, 128);
CONSOLE_INT_VAL("hmap", minLevel, 1, 0, 8);

void cull_lod_grid3(LodGridCullData &cull_data, float hMin, float hMax, const Frustum &frustum, const BBox2 &clip,
  const Occlusion *use_occlusion, const HeightmapHeightCulling *heightCulling, float waterLevel, vec3f vp, float scale_by_dist,
  int dim)
{
  G_UNUSED(use_occlusion);
  G_UNUSED(waterLevel);
  scale_by_dist *= scale_by_dist;
  RecursiveContext ctx = {cull_data, heightCulling, frustum, vp, use_occlusion, clip[0], hMin, hMax, (hMax - hMin) / 65535.f, minLevel,
    max<int>(minLevel, maxLevel), height_error * height_error / (scale_by_dist > 0 ? scale_by_dist : 1), abs_height_error,
    size_error * size_error / (scale_by_dist > 0 ? scale_by_dist : 1), scale_by_dist > 0};
  // debug("start");
  float patchSize = min(clip.width().x, clip.width().y);
  recursive_split(ctx, 0, 0, 0, patchSize, true, nullptr);
  if (ctx.patches.size() == 0)
    return;
    // todo: sort front to back. In simplest by distance from viewer
#if USE_HASHMASP
  auto findNeighboorLod = [&](int x, int y, int xo, int yo, uint8_t level) {
    x += xo, y += yo;
    for (int lod = level; lod >= ctx.minLODUsed; --lod, x /= 2, y /= 2)
    {
      if (ctx.patches.has_key(ctx.encode_patch(x, y, lod, 0)))
        return uint8_t(lod);
    }
    return level;
  };
#else
  // no hashmap lookups, but requires 3*N iterations of all patches
  dag::Vector<IBBox2, framemem_allocator> lodBoxes;
  dag::Vector<uint32_t, framemem_allocator> lodOfs;
  eastl::bitvector<framemem_allocator> lodsMap;

  if (ctx.maxLODUsed != ctx.minLODUsed)
  {
    lodBoxes.resize(ctx.maxLODUsed - ctx.minLODUsed + 1);
    lodOfs.resize(lodBoxes.size());
    iterate_patches(ctx, [&](auto key) {
      int x, y;
      uint8_t level, ofs;
      ctx.decode_patch(key, x, y, level, ofs);
      lodBoxes[level - ctx.minLODUsed] += IPoint2(x, y);
    });
    int totalLodsSz = 0;
    for (int i = 0, ie = lodBoxes.size(); i < ie; ++i)
    {
      if (!lodBoxes[i].isEmpty())
      {
        auto w = lodBoxes[i].width();
        lodOfs[i] = totalLodsSz;
        totalLodsSz += (w.x + 1) * (w.y + 1);
      }
    }
    lodsMap.resize(totalLodsSz);
    iterate_patches(ctx, [&](auto key) {
      int x, y;
      uint8_t level, ofs;
      ctx.decode_patch(key, x, y, level, ofs);
      const int lodI = level - ctx.minLODUsed;
      auto box = lodBoxes[lodI];
      lodsMap.set(lodOfs[lodI] + (x - box[0].x) + (y - box[0].y) * (box[1].x - box[0].x + 1), true);
    });
  }

  auto findNeighboorLod = [&](int x, int y, int xo, int yo, uint8_t level) {
    x += xo, y += yo;
    for (int lod = level; lod >= ctx.minLODUsed; --lod, x /= 2, y /= 2)
    {
      const int lodI = lod - ctx.minLODUsed;
      auto box = lodBoxes[lodI];
      if (!(box & IPoint2(x, y)))
        continue;
      if (lodsMap.test(lodOfs[lodI] + x - box[0].x + (y - box[0].y) * (box[1].x - box[0].x + 1), false))
        return uint8_t(lod);
    }
    return level;
  };
#endif
  int neighboors[4] = {0, 0, 0, 0};
  auto samePatchTess = float_pack_edges_tess(neighboors);
  iterate_patches(ctx, [&](auto key) {
    int x, y;
    uint8_t level, ofs;
    ctx.decode_patch(key, x, y, level, ofs);
    const float levelPatch = patchSize / (1 << level);
    const float levelPatchSize = levelPatch / dim;
    // Point2 lt(clip[0].x + levelPatch * x, clip[0].y + levelPatch * y);
    // BBox2 pBox(lt, lt + Point2(levelPatch, levelPatch));
    // bool isHigh = pBox&Point2(-18777, 12132);
    // bool isLow = pBox&Point2(-18777, 12488);
    //  neighboors are in pairs (as long as it is same lod), so we can reduce hash lookups, by immediately update neighboor bits
    //  however, if we are using plain maps, it is very low cost of such lookup
    auto patchEdges = samePatchTess;
    if (level != ctx.minLODUsed) // min lod can have no 'worser' tesselation at neighboorhod
    {
      neighboors[0] = edge_tess_encode(level - findNeighboorLod(x, y, -1, 0, level));
      neighboors[1] = edge_tess_encode(level - findNeighboorLod(x, y, +1, 0, level));
      neighboors[2] = edge_tess_encode(level - findNeighboorLod(x, y, 0, -1, level));
      neighboors[3] = edge_tess_encode(level - findNeighboorLod(x, y, 0, +1, level));
      patchEdges = float_pack_edges_tess(neighboors);
    }
    // if (isLow || isHigh)
    //   debug("ctx.minLODUsed = %d patch %d:%dx%d %@ neighboors above %d below %d ", ctx.minLODUsed, level, x,y, pBox, neighboors[2],
    //   neighboors[3]);
    G_STATIC_ASSERT(LodGridCullData::ADD_TRICNT + 1 == TRI_COUNT);
    G_STATIC_ASSERT(LodGridCullData::DIAMOND_FIRST_LTRB + 1 == TRI_DIAMOND_FIRST_LTRB);
    G_STATIC_ASSERT(LodGridCullData::REGULAR_RTLB + 1 == TRI_REGULAR_RTLB);
    G_STATIC_ASSERT(LodGridCullData::REGULAR_LTRB + 1 == TRI_REGULAR_LTRB);
    G_STATIC_ASSERT(TRI_DIAMOND_FIRST_RTLB == 0);
    const Point4 encodedPatch(levelPatchSize, patchEdges, clip[0].x + levelPatch * x, clip[0].y + levelPatch * y);
    if (ofs > TRI_DIAMOND_FIRST_RTLB)
      cull_data.additionalTriPatches[ofs - 1].emplace_back(encodedPatch);
    else
      cull_data.patches.emplace_back(encodedPatch);
  });
}


static Point3 getClippedOrigin(const BBox3 &worldBox, const Point3 &origin_pos)
{
  Point3 clippedOrigin;
  clippedOrigin.x = min(max(worldBox[0].x, origin_pos.x), worldBox[1].x);
  clippedOrigin.y = min(max(worldBox[0].y, origin_pos.y), worldBox[1].y);
  clippedOrigin.z = min(max(worldBox[0].z, origin_pos.z), worldBox[1].z);
  return clippedOrigin;
}

void frustumCulling(LodGridCullData &cull_data, const Point3 &world_pos, float water_level, const Frustum &frustum,
  const Occlusion *occlusion, const HeightmapHandler &handler, const TMatrix4 &proj)
{
  if (!frustum.testBoxB(handler.vecbox.bmin, handler.vecbox.bmax))
  {
    cull_data.eraseAll();
    return;
  }
  const HeightmapRenderer &renderer = handler.getRenderer();
  calc_lod_errors(handler, minLevel, maxLevel, renderer.getDim(), water_level);
  TIME_PROFILE(htmap_frustum_cull2);
  if (!handler.heightmapHeightCulling.get())
    return;
  float projScale = max(fabsf(proj._11), fabsf(proj._22)), projZ = proj._34;
  cull_lod_grid3(cull_data, handler.worldBox[0].y, handler.worldBox[1].y, frustum, handler.worldBox2, occlusion,
    handler.heightmapHeightCulling.get(), water_level, v_ldu(&world_pos.x), projZ > 0 ? projScale : 0, renderer.getDim());
}
