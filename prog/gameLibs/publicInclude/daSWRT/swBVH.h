//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <daBVH/dag_swTLAS.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <dag/dag_vector.h>
#include <generic/dag_tab.h>

namespace build_bvh
{
struct SplitHelper;
struct BLASData;
struct TLASData;
void destroy(BLASData *&);
void destroy(TLASData *&);
} // namespace build_bvh
class ComputeShaderElement;

namespace daSWRT
{
// BoxesOnly: each leaf is its box (daSWRT/CLAUDE.md)
enum class BlasMode : uint8_t
{
  Full,
  BoxesOnly
};

// Self-contained, FP16-encoded BLAS chunk for one model. Produced by the thread-safe
// RenderSWRT::buildBLAS; consumed by RenderSWRT::addBuiltModel (which must run under the
// caller's mutex). Empty `data` marks a pure axis-aligned box (unless noGeometry) -- addBuiltModel
// routes it through the box path using `box` directly.
struct BuiltBLAS
{
  bbox3f box = {};
  // FP16 tree bytes, then the GPU vertex payload (12 B float3 or 8 B fp16 per `vertsFp16`). Verts are
  // reached through each leaf's relative base, NOT at data+treeBytes: a padded producer (the collision
  // chunk splice) 8-aligns the pool, so padding can sit between the tree and the vertex payload.
  dag::Vector<uint8_t> data;
  uint32_t treeBytes = 0; // tree-walk span in bytes (GPU traversal bound), not the vertex offset; 0: pure box or noGeometry
  float dimAsBoxDist = 0;
  // Vertex format of `data` (true = 8 B fp16, false = 12 B float3). For a Full build addBuiltModel asserts it matches
  // the target RenderSWRT::blasVertsFp16; a BoxesOnly build has no vertices.
  bool vertsFp16 = true;
  BlasMode mode = BlasMode::Full; // BoxesOnly: data is the tree alone
  // Every source face was degenerate, or a BoxesOnly build kept no leaf (empty data too): nothing traceable. Unlike
  // isBox() (empty data is the genuine-box sentinel), addBuiltModel returns -1 (no model) instead of registering the
  // AABB, which would cast phantom occlusion.
  bool noGeometry = false;
  // Every face is light transparent: the shadow walk (swTLAS.dshl, SWRT_SHADOW_SKIP_LEAF) skips the instance whole,
  // a box too. A mixed model carries it per leaf instead (QUAD_LEAF_USER_TRANSPARENT).
  bool lightTransparent = false;
  // A box model's leaf user value (QUAD_LEAF_USER_MASK), kept in its TLAS box leaf when RenderSWRT::tlasBoxUser.
  uint8_t boxUser = 0;
  bool isBox() const { return data.empty(); }
  bool compactToBoxLeaves(); // BoxesOnly; no leaf left: noGeometry, false
};

// shadow maps that hold the casters near a receiver, metres (0 distance = off); the ray starts past the farther one
struct ShadowCasterReach
{
  float csmDistance = 0, csmReach = 0;           // CascadeShadows::getMaxDistance (where the last cascade ends), getCasterReach
  float staticDistance = 0, staticTopHeight = 0; // the static map holds every caster below this world y
  float staticDepth = 10000;                     // and up to this far to the sun from the camera
};
} // namespace daSWRT

struct RenderSWRT
{
  void init();
  void close();
  ~RenderSWRT();

  // GPU BLAS vertex format for the models this instance serializes: true = 8 B (x float, y/z fp16),
  // false = 12 B float3. Must match BLAS_VERT_FP16 in the shaders that read bottomBuf. Runtime, not a
  // compile constant, so one build can serve shaders of either format. Set before adding models.
  bool blasVertsFp16 = true;

  daSWRT::BlasMode blasMode = daSWRT::BlasMode::Full; // locked on first append; change: clearAll, rebuild

  // Opt-in: buildBottomLevelStructures also derives the flat BLAS leaf table
  // (bvh_blas_leaf_table / bvh_blas_leaf_ranges) needed by geometry-driven consumers
  // (SDF leaf raster). Costs an O(tree bytes) walk per build plus two GPU buffers, so
  // callers without such a consumer leave it off. Set before buildBottomLevelStructures.
  bool buildBlasLeafTables = false;

  // Opt-in: each box TLAS leaf keeps its model's BuiltBLAS::boxUser (+4 B). Set before adding models.
  // A shader that reads a box hit's leaf user value needs it on and must define SWRT_TLAS_BOX_USER.
  bool tlasBoxUser = false;

  int addBoxModel(vec4f bmin, vec4f bmax, bool light_transparent = false);

  // Builds a BuiltBLAS from a raw index/vertex mesh. Detects pure axis-aligned boxes
  // automatically (the returned BuiltBLAS is isBox() in that case). Thread-safe: does not
  // touch any RenderSWRT state, safe to invoke from threadpool workers. For threshold
  // semantics of `dim_as_box_dist` see addBoxModel / addBuiltModel. Returns an empty
  // (isBox(), zero box) BuiltBLAS for invalid input (which, if passed to addBuiltModel,
  // registers a zero-volume box), and a noGeometry one when every face is degenerate or BoxesOnly keeps no leaf.
  static daSWRT::BuiltBLAS buildBLAS(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
    float dim_as_box_dist, bool fp16_verts, daSWRT::BlasMode mode);
  // 32-bit index entry: uint16 indices cannot address >65535 verts, so meshes above that
  // threshold must use this one (below it, prefer the 16-bit entry). Same contract as above
  // (thread-safe; empty BuiltBLAS on invalid input, noGeometry when every face is degenerate or BoxesOnly keeps no leaf -
  // both share the build tail), except box detection is skipped: a wide mesh cannot be an
  // 8-vert box.
  static daSWRT::BuiltBLAS buildBLAS(const uint32_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
    float dim_as_box_dist, bool fp16_verts, daSWRT::BlasMode mode);

  // Registers a BuiltBLAS into SWRT. Small, fast: appends bytes into the internal concat
  // buffer and pushes to the tracking arrays. Not thread-safe; parallel callers must
  // serialize with their own mutex. Moves `built` in.
  int addBuiltModel(daSWRT::BuiltBLAS &&built);

  // uint16-index convenience wrapper: buildBLAS + addBuiltModel (wide meshes pair the
  // uint32 buildBLAS with addBuiltModel directly). For parallel add, prefer calling
  // buildBLAS from worker threads and addBuiltModel under a mutex so the heavy build
  // stays outside the critical section.
  int addModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
    float threshold_to_dim_as_box = 0);

  inline void addInstance(int model_index, const mat43f &tm)
  {
    G_ASSERT(uint32_t(model_index) < blasDataInfo.size());
    instances.push_back(model_index);
    matrices.push_back(tm);
  }

  build_bvh::BLASData *buildBottomLevelStructures();
  build_bvh::TLASData *buildTopLevelStructures();
  build_bvh::TLASData *buildTopLevelStructures(build_bvh::TLASData *d); // reuses TLAS, no log messages
  void copyToGPU(build_bvh::BLASData *);
  void copyToGPUAndDestroy(build_bvh::BLASData *);
  void copyToGPUAndDestroy(build_bvh::TLASData *);
  void copyToGPUAndClear(build_bvh::TLASData *); // clear TLASData, so it can be reused without re-allocs
  static void destroy(build_bvh::BLASData *&);
  static void destroy(build_bvh::TLASData *&);

  void drawRT();
  void reserveAddInstances(int cnt);
  uint32_t currentInstancesCount() const { return instances.size(); }

  uint32_t getBLASCount() const { return blasDataInfo.size(); }
  uint32_t getBLASBytes() const { return blasBytes.size(); }

  void clearTLASSourceData(bool shrink_arrays = true);
  void clearBLASSourceData();
  void clearTLASBuffers();
  void clearBLASBuffers();
  void clearBLASBuiltData();
  void clearAll();

  static uint32_t getTileBufferSizeDwords(uint32_t w, uint32_t h);
  // the shadow target for a src_w x src_h half-res depth; quarter needs the checkerboard
  static IPoint2 getShadowTargetSize(int src_w, int src_h, bool checkerboard, bool quarter);
  // the quarter target holds the traced pixel's linear depth in .y, for swrt_quarter_upsample
  static uint32_t getShadowTargetFormat(bool checkerboard, bool quarter);
  enum
  {
    NO_CHECKER_BOARD = 0
  };
  // frame_no: real frame counter for the soft-sun jitter (checkerboad_frame is NO_CHECKER_BOARD without the checkerboard).
  // src_w, src_h: real source (half-res depth) size.
  void renderShadows(const Point3 &dir_to_sun, float sun_size, uint32_t frame_no, uint32_t checkerboad_frame, int src_w, int src_h,
    Sbuffer *shadow_mask_buf, BaseTexture *shadow_target, bool set_target_var, const daSWRT::ShadowCasterReach &caster_reach = {},
    const IPoint2 &full_res = {}); // the full-res size the half-res source is made from; 0 = 2 * (src_w, src_h)
  bool hasData() const { return bool(topBuf); }
  // 0 = off; software shadow rays of receivers farther from the camera than metres see casters as boxes sooner
  static void setShadowBoxDistRef(float metres);
  // hard shadows; needs a checkerboard frame; size the target with getShadowTargetSize/Format, read it with swrt_shadow_sample
  void setShadowQuarterRate(bool on) { shadowQuarter = on; }
  // < 0 = off; software shadow rays take each BLAS leaf as its box, from the larger of the caster reach and skip_dist metres
  static void setShadowBlasLeafBoxes(float skip_dist);

protected:
  bool shadowQuarter = false;

  ComputeShaderElement *checkerboard_shadows_swrt_cs = nullptr, *shadows_swrt_cs = nullptr, *mask_shadow_swrt_cs = nullptr;
  ComputeShaderElement *quarter_shadows_swrt_cs = nullptr;
  void renderShadowFrustumTiles(int w, int h, const Point3 &dir_to_sun, float sun_size, int tile_size);
  build_bvh::TLASData *buildTopLevelStructuresInternal(build_bvh::TLASData *d, bool do_debug);
  void copyToGPUInternal(build_bvh::TLASData *d, bool do_del, bool do_debug);
  UniqueBufWithShaderVar bottomBuf;
  UniqueBufWithShaderVar topBuf;
  UniqueBufWithShaderVar topLeavesBuf;
  // flat per-model BLAS leaf table (1 dword/leaf): random access to leaf bodies for
  // geometry-driven consumers (SDF leaf raster) - the skip-tree has no O(1) enumeration
  UniqueBufWithShaderVar blasLeafTableBuf;
  UniqueBufWithShaderVar blasLeafRangesBuf;

  // Per-registered-model tracking, pushed from addBuiltModel/addBoxModel. Sizes stay in
  // lockstep; `blasDataInfo[i]` encodes {byte offset into blasBytes, tree byte count}, or
  // {BLAS_IS_BOX, 0} for pure box entries. `blasTotalSizeInfo[i]` is the full chunk size
  // (tree + vertex payload) for model i; 0 for pure boxes. Needed to locate and remove a
  // model's bytes from blasBytes -- blasDataInfo only gives tree span, not the full range.
  dag::Vector<build_bvh::BLASDataInfo> blasDataInfo;
  dag::Vector<uint16_t> blasDimAsBox;
  dag::Vector<uint8_t> blasLightTransparent;
  dag::Vector<uint8_t> blasBoxUser;
  dag::Vector<bbox3f> blasBoxes;
  dag::Vector<uint32_t> blasTotalSizeInfo;

  // Concatenated FP16-encoded BLAS bytes for all non-box models added since the last
  // clearBLASSourceData. buildBottomLevelStructures copies this into a BLASData* for GPU
  // upload; callers who want to resume adding and rebuild simply keep addBuiltModel'ing.
  dag::Vector<uint8_t> blasBytes;
  // Vertex format (blasVertsFp16 as 0/1) the current blasBytes were appended with; -1 when empty. Locks on
  // the first non-box addBuiltModel so a later blasVertsFp16 flip cannot mix 8 B and 12 B payloads in one
  // bottomBuf. Reset by clearBLASSourceData.
  int8_t blasBytesVertsFp16 = -1;
  int8_t blasBytesMode = -1;                             // blasMode, locked as blasBytesVertsFp16
  daSWRT::BlasMode gpuBlasMode = daSWRT::BlasMode::Full; // of bottomBuf

  dag::Vector<int> tlasLeavesOffsets;
  dag::Vector<mat43f> matrices;
  dag::Vector<int> instances;
  PostFxRenderer rt;
  bool inited = false;

  void build_tlas(Tab<bbox3f> &tlas_boxes, const dag::Vector<bbox3f> &blas_boxes, bool do_debug);
};
