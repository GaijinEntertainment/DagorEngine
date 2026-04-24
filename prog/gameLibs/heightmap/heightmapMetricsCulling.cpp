// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/lodGrid.h>
#include <frustumCulling/frustumPlanes.h>
#include <heightmap/heightmapCullingQuality.h>
#include <memory/dag_framemem.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_hashedKeyMap.h>
#include <EASTL/bitvector.h>
#include <util/dag_convar.h>
#include <util/dag_bitArray.h>
#include <math/dag_bit_scale.h>
#include <math/integer/dag_IBBox2.h>
#include <heightmap/heightmapCulling.h>
#include <heightmap/heightmapMetricsCalc.h>
#include <perfMon/dag_perfTimer.h>

CONSOLE_INT_VAL("hmap", default_max_tesselation, 4, -4, 27);
CONSOLE_BOOL_VAL("hmap", allow_exact_edges, true);
CONSOLE_FLOAT_VAL_MINMAX("hmap", height_error, 0.004, 0, 1024);
CONSOLE_FLOAT_VAL_MINMAX("hmap", abs_height_error, 800, 0, 1024);
CONSOLE_FLOAT_VAL_MINMAX("hmap", displacement_feature_tile, 0.24, 0.01, 128);
CONSOLE_FLOAT_VAL_MINMAX("hmap", displacement_min_dist, 190, 0.01, 8192);
CONSOLE_FLOAT_VAL_MINMAX("hmap", displacement_falloff_range, 0.1, 0.0001, 2);

namespace hmap_debug
{
extern Point2 debug_point;
extern Point2 debug_point2;
}; // namespace hmap_debug

using namespace hmap_debug;


static vec4i_const even_bytes_mask = DECL_VECUINT4(0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF);
static __forceinline void decode_8_even_bytes(uint8_t *to, const uint8_t *from)
{
  v_stui_half((void *)to, v_packus16(v_andi(v_ldui((const int *)from), even_bytes_mask), v_zeroi()));
}

static inline uint32_t edge_tess_encode(int lod_difference, int max_lod_diff) { return clamp(lod_difference, 0, max_lod_diff); }

static inline float float_pack_edges_tess(int edgeTessOut[4])
{
  // Pack 4 x 4bit = 16 bit of information in a integer and then static_cast it as float. Shader unpacks edge tesselation.
  // Note: float has 23 bit of mantissa + 1 implicit bit. Thus, up to 24 bits can be packed using this trick.
  int packed_edge_tess = edgeTessOut[0] | (edgeTessOut[1] << 4) | (edgeTessOut[2] << 8) | (edgeTessOut[3] << 12);

  return float(packed_edge_tess);
}

static inline uint32_t uint_pack_edges_tess(int edgeTessOut[4])
{
  // Pack 4 x 4bit = 16 bit of information in a integer and then static_cast it as float. Shader unpacks edge tesselation.
  // Note: float has 23 bit of mantissa + 1 implicit bit. Thus, up to 24 bits can be packed using this trick.
  return edgeTessOut[0] | (edgeTessOut[1] << 4) | (edgeTessOut[2] << 8) | (edgeTessOut[3] << 12);
}

static constexpr bool debug_visual_position = false;


struct RecursiveContext
{
  const MetricsErrors &errors;
  LodGridCullData &cull_data;
  Frustum frustum;
  vec3f viewPos;
  vec4f XZMinysxzBbox;
  const Occlusion *use_occlusion = nullptr;
  Point2 lt;
  float hMin = 0, hMax = 1;
  float hDisplacementRange = 0;
  float hDisplacementMaxErr = 0;
  float hDecodeBMin = 0, hDecodeBMax = 1;
  float boundsHeightScale = 1;
  float errorHeightScale = 1;
  uint8_t minLevel = 0, maxLevel = 8;
  float heightErrorThreshold = 0, absHeightErrorThreshold = 0;
  bool scaleByDist = true;
  float patchSizeDisplacementErr = 0;
  float texelPatchSize = 0;
  float displacementDistScaleMul = 0, displacementDistScaleAdd = 0;
  float maxTesselationHeightErr = 0;

  uint8_t minLODUsed = 0xFF, maxLODUsed = 0;
  bool useMorph = true;
  void add_patch(int x, int y, uint8_t level, float err, float parent_err);
  void add_patch_no_morph(int x, int y, uint8_t level);
  enum
  {
    MAX_LEVELS = 28
  };
  carray<float, MAX_LEVELS> displacementErrors;

  void calc(int dim, float disp_meters_feature, float disp_min_dist, float disp_falloff_range)
  {
    // simulated uv_tile represents "statistical" tiling of displacement data (i.e. it can be untile
    // i.e. it can be infinitely untiled data, such as perlin noise, with octave 'size' of most significant octave as
    // disp_meters_feature
    patchSizeDisplacementErr = disp_meters_feature / dim;
    displacementDistScaleMul = -1.f / disp_falloff_range;
    displacementDistScaleAdd = 1.f + -disp_min_dist * displacementDistScaleMul;
  }
  struct Key
  {
    uint64_t v;
  };
  dag::Vector<Key, framemem_allocator> patches;
  typedef uint8_t morph_t;
  static const morph_t no_morph_val = 255;
  dag::Vector<morph_t, framemem_allocator> morphs;
  void reserve(uint32_t v)
  {
    patches.reserve(v);
    morphs.reserve(v);
  }
  enum
  {
    DIM_BITS = 29,
    DIM_SIGN_BIT = 1 << (DIM_BITS - 1),
    INT_COMPLEMENT_MASK = (~0u >> DIM_BITS) << DIM_BITS,
    DIM_MASK = (1 << DIM_BITS) - 1,
    LEVEL_OFS = DIM_BITS * 2,
    LEVEL_BITS = 64 - LEVEL_OFS
  };
  static Key encode_patch(int x, int y, uint8_t level);
  static void decode_patch(Key k, int &x, int &y, uint8_t &level);
};

inline RecursiveContext::Key RecursiveContext::encode_patch(int x, int y, uint8_t level)
{
  return Key{uint64_t(x & DIM_MASK) | (uint64_t(y & DIM_MASK) << uint64_t(DIM_BITS)) | (uint64_t(level) << uint64_t(LEVEL_OFS))};
}

inline void RecursiveContext::decode_patch(Key k, int &x, int &y, uint8_t &level)
{
  level = (k.v >> uint64_t(LEVEL_OFS));
  x = k.v & DIM_MASK;
  x |= (x & DIM_SIGN_BIT) ? INT_COMPLEMENT_MASK : 0;
  y = (k.v >> uint64_t(DIM_BITS)) & DIM_MASK;
  y |= (y & DIM_SIGN_BIT) ? INT_COMPLEMENT_MASK : 0;
}

static inline uint32_t encode_max_uint4(uint8_t m, uint8_t x, uint8_t y, uint8_t z, uint8_t w)
{
  return max(m, x) | (max(m, y) << 8) | (max(m, z) << 16) | (max(m, w) << 24);
}

static const struct MorphScaleLevels
{
  carray<float, 1 + RecursiveContext::MAX_LEVELS> mul;
  MorphScaleLevels()
  {
    for (int level = 0; level < mul.size(); ++level)
      mul[level] = RecursiveContext::no_morph_val * (1.f + 32.f * powf(1.5f, -level));
  }
  const float operator[](int i) const { return mul[i]; }
} morph_scale_level;


inline void RecursiveContext::add_patch_no_morph(int x, int y, uint8_t level) { patches.emplace_back(encode_patch(x, y, level)); }
inline void RecursiveContext::add_patch(int x, int y, uint8_t level, float err, float parent_err)
{
  morph_t morph;
  if (err >= heightErrorThreshold)
  {
    morph = no_morph_val;
  }
  else
  {
    float morphT = (parent_err - heightErrorThreshold) / (parent_err - err);
    // morph = morph * morph; // to make it even smoother
    morph = min<int>(no_morph_val, int(morphT * morph_scale_level[level] + 0.5f)); // so bigger patches morphs faster
  }
  morphs.emplace_back(morph);
  patches.emplace_back(encode_patch(x, y, level));
}

template <class Cb>
static __forceinline void iterate_patches(const RecursiveContext &c, const Cb &cb)
{
  for (auto key : c.patches)
    cb(key);
}
template <class Cb>
static __forceinline void iterate_patches_morph(const RecursiveContext &c, const Cb &cb)
{
  if (c.useMorph)
  {
    auto morph = c.morphs.begin();
    for (auto i = c.patches.begin(), e = c.patches.end(); i != e; ++i, ++morph)
      cb(*i, *morph);
  }
  else
    for (auto key : c.patches)
      cb(key, RecursiveContext::no_morph_val);
}

static void recursive_split(RecursiveContext &ctx, int global_x, int global_y, uint8_t lev, float patchSize, bool test_frustum,
  const Point2 &parent_ht_min_max, float parentErr = 1.f)
{
  ctx.maxLODUsed = max(ctx.maxLODUsed, lev);
  Point2 htMinMax;
  Point2 origin = ctx.lt + Point2(global_x, global_y) * patchSize;
  const int local_x = global_x & ((1 << lev) - 1);
  const int local_y = global_y & ((1 << lev) - 1);
  const bool mirror_x = (global_x >> lev) & 1, mirror_y = (global_y >> lev) & 1;
  const int eff_x = mirror_x ? (((1 << lev) - 1) - local_x) : local_x;
  const int eff_y = mirror_y ? (((1 << lev) - 1) - local_y) : local_y;
  HeightBounds hb;
  if (ctx.errors.getBoundsSafe(lev, eff_x, eff_y, hb))
    htMinMax = Point2(hb.hMin * ctx.boundsHeightScale + ctx.hDecodeBMin, hb.hMax * ctx.boundsHeightScale + ctx.hDecodeBMax);
  else
    htMinMax = parent_ht_min_max;
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
  if (v_check_xyzw_any_true(v_cmp_gt(v_perm_xycd(patch, v_neg(patch)), ctx.XZMinysxzBbox)))
    return;
  if (ctx.use_occlusion && ctx.use_occlusion->isOccludedBoxExtent2(center2, extent2))
    return;

  if (!ctx.useMorph && lev >= ctx.maxLevel)
  {
    ctx.minLODUsed = min(ctx.minLODUsed, lev);
    // debug("level %d order %d: %d", lev, dataOfs < ctx.errors.order.size(), dataOfs < ctx.errors.order.size() ?
    // ctx.errors.order[dataOfs] : 0);
    ctx.add_patch_no_morph(global_x, global_y, lev);
    return;
  }
  float heightError = 1e28;
  if (lev >= ctx.minLevel)
  {
    const bool d1 = debug_visual_position && (BBox2(origin, origin + Point2(patchSize, patchSize)) & debug_point);
    const bool d2 = debug_visual_position && (BBox2(origin, origin + Point2(patchSize, patchSize)) & debug_point2);
    float distScale = 1, dist = 1, displacementScale = 0;
    float radius = 0;
    if (ctx.scaleByDist)
    {
      // more gradual distance
      radius = sqrtf(0.5f) * patchSize;
      if (patchSize > ctx.texelPatchSize)
        radius = max(0.5f * (htMinMax.y - htMinMax.x), radius);

      const Point3_vec4 htCenter(origin.x + 0.5f * patchSize, 0.5f * (htMinMax.y + htMinMax.x), origin.y + 0.5f * patchSize);
      float viewPosY = v_extract_y(ctx.viewPos);
      const float xzDist = fabsf(v_extract_x(ctx.viewPos) - htCenter.x) + fabsf(v_extract_z(ctx.viewPos) - htCenter.z);
      viewPosY = max(viewPosY, htCenter.y - xzDist);

      dist = v_extract_x(v_length3_x(v_sub(v_perm_xbzw(ctx.viewPos, v_splats(viewPosY)), v_ld(&htCenter.x))));
      dist = dist - radius;
      distScale = 1.f / max(.01f, dist);
      displacementScale = clamp(dist * ctx.displacementDistScaleMul + ctx.displacementDistScaleAdd, 0.f, 1.f);
    }
    const hmap_err_t err = ctx.errors.getErrorUnsafe(lev, eff_x, eff_y);
    const float patchHeightRealError = err * ctx.errorHeightScale;
    const float patchHeightError =
      patchHeightRealError + displacementScale * min(ctx.patchSizeDisplacementErr * patchSize, ctx.hDisplacementMaxErr);
    heightError = patchHeightError * distScale;
    // const float patchSizeErrorSq = patchSize * patchSize * distScaleSq;
    if (debug_visual_position && (d2 || d1))
      debug("point %s level %d %dx%d (eff %dx%d): error patchHeightError %d %@ dist %f + %f threshold %d = %g<%g htMinMax = %@ hb "
            "%f %f pos %@",
        d1 ? "d1" : "d2", lev, global_x, global_y, eff_x, eff_y, err, patchHeightError, dist, radius,
        heightError <= ctx.heightErrorThreshold, heightError, ctx.heightErrorThreshold, htMinMax, hb.hMin, hb.hMax, ctx.viewPos);
    const bool isAcceptable = patchHeightError <= ctx.absHeightErrorThreshold;
    if ((heightError <= ctx.heightErrorThreshold && isAcceptable) || lev >= ctx.maxLevel)
    {
      ctx.maxTesselationHeightErr = max(ctx.maxTesselationHeightErr, patchHeightRealError);
      ctx.minLODUsed = min(ctx.minLODUsed, lev);
      ctx.add_patch(global_x, global_y, lev, heightError, parentErr);
      return;
    }
    if (!isAcceptable)
      heightError = 1e28f;
  }

  float subPatchSize = patchSize * 0.5f;
  const int mask = v_signmask(v_cmp_gt(v_add(ctx.viewPos, ctx.viewPos), center2));
  const int primary = ((mask & 1) | ((mask >> 1) & 2));
  int child_global_x = global_x << 1;
  int child_global_y = global_y << 1;
  uint8_t child_lev = lev + 1;
  for (int i = 0; i < 4; ++i)
  {
    // this is poor man's sort - just traverse children in best order (based on 2d distance to viewer)
    const int order = primary ^ i;
    recursive_split(ctx, child_global_x + (order & 1), child_global_y + ((order & 2) >> 1), child_lev, subPatchSize, test_frustum,
      htMinMax, heightError);
  }
}


inline void recursive_split_root(RecursiveContext &ctx, float tileSize)
{
  TIME_PROFILE_UNIQUE_EVENT_DEV("recursive_split_root");
  vec3f tile_bmin = v_make_vec4f(ctx.lt.x, ctx.hMin, ctx.lt.y, 0);
  vec3f tile_bmax = v_make_vec4f(ctx.lt.x + tileSize, ctx.hMax, ctx.lt.y + tileSize, 0);
  if (!ctx.frustum.testBoxB(tile_bmin, tile_bmax))
    return;
  recursive_split(ctx, 0, 0, 0, tileSize, true, Point2(ctx.hMin, ctx.hMax));
}


inline void recursive_split_root_mirror(RecursiveContext &ctx, float tileSize)
{
  TIME_PROFILE_UNIQUE_EVENT_DEV("recursive_split_root_mirror");
  bbox3f fBox;
  ctx.frustum.calcFrustumBBox(fBox);
  // Expand conservatively
  alignas(16) int xzBox[4];
  vec4f xzXZ = v_div(v_sub(v_perm_xzac(fBox.bmin, fBox.bmax), v_perm_xyxy(v_ldu_half(&ctx.lt.x))), v_splats(tileSize));
  vec4i xzXZi = v_permi_xycd(v_cvt_floori(xzXZ), v_cvt_ceili(xzXZ));
  v_sti(xzBox, xzXZi);


  for (int tile_j = xzBox[1]; tile_j < xzBox[3]; ++tile_j)
  {
    for (int tile_i = xzBox[0]; tile_i < xzBox[2]; ++tile_i)
    {
      Point2 tileBase = ctx.lt + Point2(tile_i, tile_j) * tileSize;
      vec3f tile_bmin = v_make_vec4f(tileBase.x, ctx.hMin, tileBase.y, 0);
      vec3f tile_bmax = v_make_vec4f(tileBase.x + tileSize, ctx.hMax, tileBase.y + tileSize, 0);
      auto vis = ctx.frustum.testBox(tile_bmin, tile_bmax);
      if (vis == Frustum::OUTSIDE)
        continue;

      recursive_split(ctx, tile_i, tile_j, 0, tileSize, vis != Frustum::INSIDE, Point2(ctx.hMin, ctx.hMax));
    }
  }
}

void cull_lod_grid3(const MetricsErrors &errors, LodGridCullData &cull_data, const Frustum &frustum, const Occlusion *use_occlusion,
  float hMin, float hMax, const BBox2 &clip, float maxUpwardDisplacement, float maxDownwardDisplacement, bool mirror, vec3f vp,
  const vec4f *bbox2, const HeightmapMetricsQuality &q)
{
  const bool exactEdges = allow_exact_edges && (q.quality & q.EXACT_EDGES), useMorph = exactEdges && (q.quality & q.USE_MORPH);
  cull_data.eraseAll();
  cull_data.useHWTesselation = false;
  const int dimBits = get_log2i(errors.dim);

  float hDisplacementRange = maxUpwardDisplacement + maxDownwardDisplacement;
  float hDisplacementMaxErr = max(maxUpwardDisplacement, maxDownwardDisplacement);

  int minLevel = 0;
  const int maxResShift = errors.maxResShift;
  if (q.minRelativeTexelTess != q.invalid_tess)
    minLevel = clamp(maxResShift + q.minRelativeTexelTess, 0, RecursiveContext::MAX_LEVELS - 1);
  minLevel = max<int>(errors.minLevel, minLevel);

  const int maxTess = q.maxRelativeTexelTess != q.invalid_tess ? q.maxRelativeTexelTess : default_max_tesselation;
  int maxLevel = clamp<int>(maxResShift + maxTess, minLevel, RecursiveContext::MAX_LEVELS);

  const float curRelErr = q.heightRelativeErr > 0 ? q.heightRelativeErr : height_error;
  const float curAbsErr = q.maxAbsHeightErr > 0 ? q.maxAbsHeightErr : abs_height_error;

  // todo: pass that as argument
  vec4f XZMinysxzBbox;
  if (bbox2)
    XZMinysxzBbox = *bbox2;
  else if (mirror)
    XZMinysxzBbox = v_splats(FLT_MAX);
  else
    XZMinysxzBbox = v_make_vec4f(clip[1].x, clip[1].y, -clip[0].x, -clip[0].y);

  RecursiveContext ctx = {errors, cull_data, frustum, vp, XZMinysxzBbox, use_occlusion, clip[0], hMin - maxDownwardDisplacement,
    hMax + maxUpwardDisplacement, hDisplacementRange, hDisplacementMaxErr, errors.boundsHeightOffset - maxDownwardDisplacement,
    errors.boundsHeightOffset + maxUpwardDisplacement, errors.boundsHeightScale, errors.errorHeightScale, uint8_t(minLevel),
    uint8_t(max<int>(minLevel, maxLevel)), curRelErr / (q.distanceScale > 0 ? q.distanceScale : 1), curAbsErr, q.distanceScale > 0};

  float patchSize = min(clip.width().x, clip.width().y);
  ctx.texelPatchSize = patchSize / (1 << maxResShift);

  const float dispTile = q.displacementMetersRepeat > 0.f ? q.displacementMetersRepeat : displacement_feature_tile,
              dispDist = q.displacementDist > 0.f ? q.displacementDist : displacement_min_dist,
              falloff = q.displacementFalloff > 0.f ? q.displacementFalloff : displacement_falloff_range;
  ctx.calc(errors.dim, dispTile, dispDist, falloff * dispDist); // fixme: passing global settings
  // debug("start");
  ctx.reserve(cull_data.patches.capacity());
  ctx.useMorph = useMorph && exactEdges;
  if (mirror)
    recursive_split_root_mirror(ctx, patchSize);
  else
    recursive_split_root(ctx, patchSize);
  if (ctx.patches.size() == 0)
    return;
  // maximum possible error on current tesselation vs ideal is: displacementRange + error in tesselation + error in error calculation
  // (0.5f of one quantization step) if you need error at BEST possible tesselation vs ideal, it is just hDisplacementRange +
  // h.getHeightScaleRaw(), you don't need to calculate anything;
  cull_data.maxDisplacementErrAtMaxLod = ctx.maxTesselationHeightErr + hDisplacementRange + ctx.errorHeightScale;
  // todo: sort front to back. In simplest by distance from viewer
  TIME_PROFILE_UNIQUE_EVENT_DEV("cull_lod_grid3");
  // TIME_PROFILE(cull_lod_grid3);

  enum
  {
    NO_MORPH_NO_EDGE,
    NO_MORPH_EDGE,
    MORPH_EDGE,
    MORPH_NO_EDGE,
    PCNT_COUNT
  };
  uint32_t patchCount[PCNT_COUNT] = {0, 0, 0, 0}; // noMorphNoEdge, noMorph, morphNoParent, morphParented
  // no hashmap lookups, but requires 3*N iterations of all patches
  dag::Vector<IBBox2, framemem_allocator> lodBoxes;
  dag::Vector<uint32_t, framemem_allocator, false> lodOfs;
  Bitarray lodsMap(framemem_ptr());
  dag::Vector<RecursiveContext::morph_t, framemem_allocator, false> morphMap;

  if (ctx.maxLODUsed != ctx.minLODUsed || ctx.useMorph) // todo: should be || ctx.morphingPatched > 0
  {
    // TIME_PROFILE(prepare_patches);
    lodBoxes.resize(ctx.maxLODUsed - ctx.minLODUsed + 1);
    lodOfs.resize(lodBoxes.size());
    TIME_PROFILE_UNIQUE_EVENT_DEV("prepare_patches");
    iterate_patches(ctx, [&](auto key) {
      int x, y;
      uint8_t level;
      ctx.decode_patch(key, x, y, level);
      lodBoxes[level - ctx.minLODUsed] += IPoint2(x, y);
    });
    int totalLodsSz = 0;
    for (int i = 0, ie = lodBoxes.size(); i < ie; ++i)
    {
      if (!lodBoxes[i].isEmpty())
      {
        lodBoxes[i][1] = lodBoxes[i].width() + IPoint2(1, 1);
        lodOfs[i] = totalLodsSz;
        totalLodsSz += lodBoxes[i][1].y * lodBoxes[i][1].x;
      }
    }
    lodsMap.resize(totalLodsSz);
    morphMap.resize(totalLodsSz);
    iterate_patches_morph(ctx, [&](auto key, RecursiveContext::morph_t morph) {
      int x, y;
      uint8_t level;
      ctx.decode_patch(key, x, y, level);
      const int lodI = level - ctx.minLODUsed;
      auto box = lodBoxes[lodI];
      const int xt = x - box[0].x, yt = y - box[0].y;
      const int index = lodOfs[lodI] + xt + yt * box[1].x;
      lodsMap.set(index);
      const bool hasMorph = morph < RecursiveContext::no_morph_val;
      int morphingType = level >= errors.maxOrderLevel + dimBits - 1 ? NO_MORPH_NO_EDGE : NO_MORPH_EDGE;
      if (hasMorph)
        morphingType = morphingType == NO_MORPH_NO_EDGE ? MORPH_NO_EDGE : MORPH_EDGE;
      patchCount[morphingType]++;
      morphMap[index] = morph;
    });
  }
  auto findNeighboorLod = [&](int x, int y, int xo, int yo, uint8_t level, int lastLod, RecursiveContext::morph_t &morph) {
    x += xo, y += yo;
    morph = RecursiveContext::no_morph_val;
    for (int lod = level; lod > lastLod; --lod, x >>= 1, y >>= 1)
    {
      const int lodI = lod - ctx.minLODUsed;
      auto box = lodBoxes[lodI];
      const int xt = x - box[0].x, yt = y - box[0].y;
      if (xt < 0 || xt >= box[1].x || yt < 0 || yt >= box[1].y)
        continue;
      const int index = lodOfs[lodI] + xt + yt * box[1].x;
      if (lodsMap.get(index))
      {
        if (lod == level)
          morph = morphMap[index];
        return uint8_t(lod);
      }
    }
    return level;
  };
  int neighboors[4] = {0, 0, 0, 0};
  auto samePatchTess = uint_pack_edges_tess(neighboors);

  const uint32_t shiftPerPatch = dimBits + dimBits - 3;
  const uint32_t bytesPerPatch = 1 << shiftPerPatch;
  const uint32_t exactEdgesBytes = !errors.exactOrder.empty() && exactEdges ? bytesPerPatch : 0;
  const uint32_t byteShiftPerPatch = exactEdgesBytes ? shiftPerPatch : 0;
  cull_data.exact_edges = exactEdgesBytes != 0;

  G_ASSERT(!exactEdgesBytes || dimBits >= 3);
  G_ASSERTF((exactEdgesBytes & 3) == 0, "exactEdgesBytes = %d", exactEdgesBytes); // parents

  uint32_t patchesAt[PCNT_COUNT];
  enum
  {
    EDGES_NO_MORPH,
    EDGES_MORPH,
    EDGES_CNT
  };
  uint32_t edgesAt[EDGES_CNT];
  if (exactEdgesBytes)
  {
    cull_data.patches.resize(ctx.patches.size());
    cull_data.edges_at = patchCount[NO_MORPH_NO_EDGE];
    cull_data.morph_at = patchCount[NO_MORPH_NO_EDGE] + patchCount[NO_MORPH_EDGE];
    const uint32_t morphCount = ctx.patches.size() - cull_data.morph_at;

    G_ASSERTF(morphCount == patchCount[MORPH_NO_EDGE] + patchCount[MORPH_EDGE], "morphCount = %d = %d-%d sameparentedges %d",
      morphCount, ctx.patches.size(), cull_data.morph_at, patchCount[MORPH_NO_EDGE]);

    cull_data.edgesData.resize(
      (patchCount[NO_MORPH_EDGE] + patchCount[MORPH_EDGE]) * exactEdgesBytes + patchCount[MORPH_EDGE] * exactEdgesBytes / 4);
    cull_data.morphData.resize(morphCount);
    patchesAt[0] = 0;
    edgesAt[EDGES_NO_MORPH] = 0;
    edgesAt[EDGES_MORPH] = patchCount[NO_MORPH_EDGE];

    for (int i = 1; i < PCNT_COUNT; ++i)
      patchesAt[i] = patchesAt[i - 1] + patchCount[i - 1];
    cull_data.morph_no_edges_at = patchesAt[MORPH_NO_EDGE];
  }
  else
  {
    cull_data.patches.reserve(ctx.patches.size());
  }
  uint32_t exactEdgesParentsAt = (patchCount[NO_MORPH_EDGE] + patchCount[MORPH_EDGE]) * exactEdgesBytes;
  int worstPatchDiff = 0;
  G_ASSERTF((get_level_ofs(errors.maxOrderLevel) - errors.minLevelOfs) * bytesPerPatch == errors.exactOrder.size(),
    "%d - %d + %d == %d", get_level_ofs(errors.maxOrderLevel) * bytesPerPatch, errors.minLevelOfs * bytesPerPatch, bytesPerPatch,
    errors.exactOrder.size());
  G_ASSERT(errors.exactOrder.size() >= exactEdgesBytes);
  const int dim = errors.dim;
  G_ASSERT(errors.dim >= 8 && dim == (1 << dimBits));
  // G_ASSERTF(dim <= 32, "not implemented");

  iterate_patches_morph(ctx, [&](auto key, RecursiveContext::morph_t morph) {
    int x, y;
    uint8_t level;
    ctx.decode_patch(key, x, y, level);

    int local_x = x & ((1 << level) - 1);
    int local_y = y & ((1 << level) - 1);
    bool mirror_x = (x >> level) & 1, mirror_y = (y >> level) & 1;
    int eff_x = mirror_x ? (((1 << level) - 1) - local_x) : local_x;
    int eff_y = mirror_y ? (((1 << level) - 1) - local_y) : local_y;

    uint8_t order = 0;
    const bool hasMorph = exactEdgesBytes && morph < RecursiveContext::no_morph_val;

    const uint32_t orderLevel = min<int>(errors.maxOrderLevel - 1, level);
    const uint32_t lastEdgeLevelDiff = level - orderLevel;
    const uint32_t orderX = eff_x >> lastEdgeLevelDiff, orderY = eff_y >> lastEdgeLevelDiff;
    uint32_t orderOfs = get_level_ofs(orderLevel) - errors.minLevelOfs;
    G_FAST_ASSERT(int(orderOfs) >= 0);
    orderOfs += (orderY << orderLevel) + orderX;
    int morphingType = 0;
    uint8_t parentBit = 2; // for validation
    if (byteShiftPerPatch) // byteShiftPerPatch can't be zero. That means 8 bits per patch, and it is now square of anything!
    {
      morphingType = level >= errors.maxOrderLevel + dimBits - 1 ? NO_MORPH_NO_EDGE : NO_MORPH_EDGE;
      if (hasMorph)
        morphingType = morphingType == NO_MORPH_NO_EDGE ? MORPH_NO_EDGE : MORPH_EDGE;
      bool compressedParent = false;

      // G_ASSERTF(dataOfs < errors.exactOrder.size(), "dataOfs = %d < %d level = %d", dataOfs, errors.exactOrder.size(), level);
      auto edgesTo = cull_data.edgesData.data() + (edgesAt[hasMorph] << byteShiftPerPatch);
      if (level < orderLevel + dimBits) // fixed edge dir
      {
        G_ASSERTF(((edgesAt[hasMorph] + 1) << byteShiftPerPatch) <= cull_data.edgesData.size(),
          "(edgesAt[%d]=%d + 1)<<%d > cull_data.edgesData.size() = %d", hasMorph, edgesAt[hasMorph], byteShiftPerPatch,
          cull_data.edgesData.size());
        edgesAt[hasMorph]++;
      }
      G_ASSERT((orderOfs << byteShiftPerPatch) + exactEdgesBytes <= errors.exactOrder.size());
      auto from = errors.exactOrder.data() + (orderOfs << byteShiftPerPatch);
      if (level < errors.maxOrderLevel)
      {
        if (dimBits == 3)
          *(uint64_t *)edgesTo = *(uint64_t *)from;
        else if (dimBits == 4)
          memcpy(edgesTo, from, 32);
        else
          memcpy(edgesTo, from, exactEdgesBytes);
      }
      else
      {
        compressedParent = true;

        const uint32_t subLevel = min<int>(lastEdgeLevelDiff, dimBits);
        const uint32_t subMask = (1 << subLevel) - 1;

        const uint32_t subLevelShift = lastEdgeLevelDiff - subLevel;
        const uint32_t subX = (eff_x >> subLevelShift) & subMask, subY = (eff_y >> subLevelShift) & subMask;
        G_ASSERTF(subX < errors.dim && subY < errors.dim, "%dx%d < %d level%d, lastEdgeLevelDiff = %d", subX, subY, errors.dim, level,
          lastEdgeLevelDiff);

        if (level >= orderLevel + dimBits) // fixed edge dir
        {
          const uint32_t bitFrom = subX + (subY << dimBits);
          parentBit = (from[bitFrom >> 3] >> (bitFrom & 7)) & 1;
        }
        else if (lastEdgeLevelDiff == 1)
        {
          if (dimBits == 3)
          {
            from += subY << 2;
            uint8_t shiftR = subX << 2;
            edgesTo[0] = edgesTo[1] = bit_duplicate_lower_4bits_to8(uint8_t(from[0] >> shiftR));
            edgesTo[2] = edgesTo[3] = bit_duplicate_lower_4bits_to8(uint8_t(from[1] >> shiftR));
            edgesTo[4] = edgesTo[5] = bit_duplicate_lower_4bits_to8(uint8_t(from[2] >> shiftR));
            edgesTo[6] = edgesTo[7] = bit_duplicate_lower_4bits_to8(uint8_t(from[3] >> shiftR));
          }
          else if (dimBits == 4)
          {
            from += (subY << 4) + subX;
            uint16_t *to = (uint16_t *)edgesTo;
            for (int j = 0, je = 8; j < je; ++j, to += 2, from += 2)
              to[0] = to[1] = bit_duplicate_lower_8bits_to16(*from);
          }
          else
          {
            from += subY << (dimBits + dimBits - 4);
            from += subX << (dimBits - 4);
            bit_upscale_2x2(edgesTo, from, 1 << dimBits);
          }
        }
        else if (lastEdgeLevelDiff == 2)
        {
          if (dimBits == 3)
          {
            from += subY << 1;
            uint8_t shiftR = subX << 1;
            edgesTo[0] = edgesTo[1] = edgesTo[2] = edgesTo[3] = bit_quadruplicate_lower_2bits_to8(uint8_t(from[0] >> shiftR));
            edgesTo[4] = edgesTo[5] = edgesTo[6] = edgesTo[7] = bit_quadruplicate_lower_2bits_to8(uint8_t(from[1] >> shiftR));
          }
          else if (dimBits == 4)
          {
            // from += subY << 3;
            const uint16_t *from16 = ((const uint16_t *)from);
            from16 += (subY << 2);
            uint16_t *to = (uint16_t *)edgesTo;
            uint8_t shiftR = subX << 2;
            for (int i = 0; i < 4; ++i, to += 4, ++from16)
              to[0] = to[1] = to[2] = to[3] = bit_quadruplicate_lower_4bits_to16(from16[0] >> shiftR);
          }
          else
          {
            from += subY << (dimBits + dimBits - 5);
            from += subX << (dimBits - 5);
            bit_upscale_4x4(edgesTo, from, 1 << dimBits);
          }
        }
        else if (lastEdgeLevelDiff >= 3)
        {
          // dim > 8, obviously, and we need multiplicate one bit to at least one byte (or more bytes)
          if (dimBits == 4)
          {
            int bitNo = (subX << 1) + (subY << 5);
            auto to = edgesTo;
            for (int i = 0; i != 2; ++i)
            {
              // replicate each bit 8 times
              to[0] = -((from[(bitNo >> 3)] >> (bitNo & 7)) & 1);
              ++bitNo;
              to[1] = -((from[(bitNo >> 3)] >> (bitNo & 7)) & 1);
              bitNo += 15; // add rest of stride in bits
              memcpy(to + 2, to, 2);
              memcpy(to + 4, to, 4);
              memcpy(to + 8, to, 8);
              to += 16;
            }
          }
          else
          {
            const int quadrantSize = dim >> lastEdgeLevelDiff;
            // debug("level =%d > errors.maxOrderLevel = %d + dimBits = %d, diff = %d", level, errors.maxOrderLevel, dimBits,
            // lastEdgeLevelDiff);
            int bitNo = (subX << (dimBits - lastEdgeLevelDiff)) + (subY << (dimBits + dimBits - lastEdgeLevelDiff)); // fixme!

            const int replicatedBytes = 1 << (lastEdgeLevelDiff - 3);
            auto to = edgesTo;
            for (int i = 0; i != quadrantSize; ++i)
            {
              auto row = to;
              for (int j = 0; j != quadrantSize; ++j, ++bitNo)
              {
                // replicate each bit 8 times
                uint8_t replicatedBit = -((from[(bitNo >> 3)] >> (bitNo & 7)) & 1);
                // replicate each byte requored number of times (zoom level)
                memset(to, replicatedBit, replicatedBytes);
                to += replicatedBytes;
              }
              // replicate each row (1<<lastEdgeLevelDiff) times
              for (int i = 1, ie = (1 << lastEdgeLevelDiff); i != ie; ++i, to += (dim >> 3))
                memcpy(to, row, dim >> 3);
              bitNo += dim - quadrantSize; // add rest of stride in bits
            }
          }
        }
      }
      if (morphingType == MORPH_EDGE) // otherwise parentFlip is same as flip anyway
      {
        auto to = cull_data.edgesData.data() + exactEdgesParentsAt;
        G_ASSERTF(level <= errors.maxOrderLevel || compressedParent, "level = %d, errors.maxOrderLevel = %d, compressedParent = %d",
          level, errors.maxOrderLevel, compressedParent);
        if (level > errors.maxOrderLevel + dimBits)
        {
          G_ASSERT(parentBit < 2);
          if (dimBits == 3)
            *(uint16_t *)to = -int16_t(parentBit);
          else if (dimBits == 4)
            *(uint64_t *)to = -int64_t(parentBit);
          else
            memset(edgesTo, -parentBit, exactEdgesBytes >> 2);
        }
        else if (level <= errors.maxOrderLevel)
        {
          // extract parent data from source parent info.
          // otherwise we have already done that for edges information (by re-duplicating bits)
          const uint32_t parentLevel = min(errors.maxOrderLevel, level) - 1;
          const uint32_t parentEdgeLevelDiff = level - parentLevel;
          const uint32_t parentX = eff_x >> parentEdgeLevelDiff, parentY = eff_y >> parentEdgeLevelDiff;
          uint32_t parentOfs = get_level_ofs(parentLevel) - errors.minLevelOfs;
          G_FAST_ASSERT(int(parentOfs) >= 0);
          parentOfs += (parentY << parentLevel) + parentX;
          G_FAST_ASSERT(uint32_t(parentOfs) < uint32_t(errors.exactOrder.size()));
          parentOfs <<= byteShiftPerPatch;
          auto from = errors.exactOrder.data() + parentOfs;

          if (dimBits == 3) // dim == 8
            *(uint16_t *)(cull_data.edgesData.data() + exactEdgesParentsAt) =
              bit_upscale_4x4_to_8x8(*(uint64_t *)(from), eff_x & 1, eff_y & 1);
          else if (dimBits == 4)
          {
            from += (eff_x & 1) + ((eff_y & 1) << 4);
            decode_8_even_bytes(to, from);
          }
          else
          {
            const int dim = 1 << dimBits;
            const int parentByteSize = dim >> 4, sourceByteSize = dim >> 3, halfBytesShift = dimBits - 4;
            from += ((eff_x & 1) << halfBytesShift) + ((eff_y & 1) << (dimBits + halfBytesShift));

            for (int j = 0, je = 1 << (dimBits - 2); j != je; ++j, to += parentByteSize, from += sourceByteSize)
              memcpy(to, from, parentByteSize);
          }
        }
        else // we have already extracted data from parents to current edge
        {
          if (dimBits == 3) // dim == 3
          {
            // a bit more optimal than a loop below, plus loop below would encode two rows in one byte
            *(uint16_t *)to = bit_downsample8x8(*(uint64_t *)edgesTo);
          }
          else if (dimBits == 4) // if dim == 16
          {
            // if dim >= 16 than half of it is 8, so it is byte aligned
            const uint16_t *from = (const uint16_t *)edgesTo;
            for (int j = 0, je = 8; j != je; ++j, to++, from += 2)
              *to = bit_downsample_line16_to8(*from);
          }
          else
          {
            uint16_t *writeTo = (uint16_t *)to;
            const uint32_t *from = (const uint32_t *)edgesTo;
            for (int j = 0, je = dim >> 1; j != je; ++j)
            {
              for (int i = 0, ie = dim >> 5; i != ie; ++i, writeTo++, from++)
                *writeTo = bit_downsample_line32_to16(*from);
              from += (dim >> 5);
            }
          }
        }
        exactEdgesParentsAt += exactEdgesBytes >> 2;
      }
    }
    else
    {
      order = errors.order[orderOfs];
      uint8_t parentOrder = 0;
      if (level < errors.maxOrderLevel && level > 0 && morph < RecursiveContext::no_morph_val)
      {
        uint32_t parentOrderOfs = get_level_ofs(level - 1) - errors.minLevelOfs + ((orderY >> 1) << orderLevel) + (orderX >> 1);
        parentOrder = errors.order[parentOrderOfs];
      }
    }

    auto patchEdges = samePatchTess;
    RecursiveContext::morph_t morphL = 0, morphR = 0, morphT = 0, morphB = 0;
    if (level != ctx.minLODUsed || ctx.useMorph) // min lod can have no 'worser' tesselation at neighboorhod
    {
      int nl;
      nl = level - findNeighboorLod(x, y, -1, 0, level, ctx.minLODUsed - 1, morphL);
      worstPatchDiff = max(worstPatchDiff, nl);
      neighboors[0] = edge_tess_encode(nl, dimBits);
      nl = level - findNeighboorLod(x, y, +1, 0, level, ctx.minLODUsed - 1, morphR);
      worstPatchDiff = max(worstPatchDiff, nl);
      neighboors[1] = edge_tess_encode(nl, dimBits);
      nl = level - findNeighboorLod(x, y, 0, -1, level, ctx.minLODUsed - 1, morphT);
      worstPatchDiff = max(worstPatchDiff, nl);
      neighboors[2] = edge_tess_encode(nl, dimBits);
      nl = level - findNeighboorLod(x, y, 0, +1, level, ctx.minLODUsed - 1, morphB);
      worstPatchDiff = max(worstPatchDiff, nl);
      neighboors[3] = edge_tess_encode(nl, dimBits);
      patchEdges = uint_pack_edges_tess(neighboors);
    }
    patchEdges |= (morph << 16);

    patchEdges |= ((parentBit & 1) << 24);
    if (hasMorph)
    {
      uint32_t mAt = patchesAt[morphingType] - cull_data.morph_at;
      G_ASSERTF(mAt < cull_data.morphData.size(), "%%d - %d total %d", patchesAt[morphingType], cull_data.morph_at,
        cull_data.morphData.size());
      cull_data.morphData[mAt] = encode_max_uint4(morph, morphL, morphR, morphT, morphB);
    }

    G_STATIC_ASSERT(LodGridCullData::ADD_TRICNT + 1 == TRI_COUNT);
    G_STATIC_ASSERT(LodGridCullData::DIAMOND_FIRST_LTRB + 1 == TRI_DIAMOND_FIRST_LTRB);
    G_STATIC_ASSERT(LodGridCullData::REGULAR_RTLB + 1 == TRI_REGULAR_RTLB);
    G_STATIC_ASSERT(LodGridCullData::REGULAR_LTRB + 1 == TRI_REGULAR_LTRB);
    G_STATIC_ASSERT(TRI_DIAMOND_FIRST_RTLB == 0);
    const float levelPatch = patchSize / (1 << level);
    const float levelPatchSize = levelPatch / dim;
    const Point4 encodedPatch(levelPatchSize, bitwise_cast<float>(patchEdges), clip[0].x + levelPatch * x, clip[0].y + levelPatch * y);
    // const Point4 encodedPatch(bitwise_cast<float>(int(level)), bitwise_cast<float>(patchEdges), bitwise_cast<float>(x),
    // bitwise_cast<float>(y));
    if (exactEdgesBytes)
    {
      cull_data.patches[patchesAt[morphingType]++] = encodedPatch;
    }
    else
    {
      if (order > TRI_DIAMOND_FIRST_RTLB)
        cull_data.additionalTriPatches[order - 1].emplace_back(encodedPatch);
      else
        cull_data.patches.emplace_back(encodedPatch);
    }
  });
  if (exactEdgesBytes)
  {
    G_ASSERTF(cull_data.morph_at == patchesAt[NO_MORPH_EDGE], "%d == %d", cull_data.morph_at, patchesAt[NO_MORPH_EDGE],
      cull_data.patches.size());
    G_ASSERT(patchesAt[PCNT_COUNT - 1] == ctx.patches.size());
  }

  G_ASSERTF(exactEdgesParentsAt == cull_data.edgesData.size(), "%d == %d", exactEdgesParentsAt, cull_data.edgesData.size());
  G_UNUSED(worstPatchDiff);
#if DAGOR_DBGLEVEL > 0
  if (worstPatchDiff > dimBits)
  {
    LOGWARN_ONCE("crack/t-junction appeared, lod diff is %d, max us %d. To be fixed (adjust filter, or sew in runtime)",
      worstPatchDiff, dimBits);
  }
#endif
  // G_ASSERT(exactEdgesAt == cull_data.patches.size() * exactEdgesBytes);
}
