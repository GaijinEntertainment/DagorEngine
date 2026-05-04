//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
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
// Self-contained, FP16-encoded BLAS chunk for one model. Produced by the thread-safe
// RenderSWRT::buildBLAS; consumed by RenderSWRT::addBuiltModel (which must run under the
// caller's mutex). Empty `data` marks a pure axis-aligned box -- addBuiltModel routes it
// through the box path using `box` directly.
struct BuiltBLAS
{
  bbox3f box = {};
  dag::Vector<uint8_t> data; // FP16-encoded tree bytes followed by FP32 vertex payload
  uint32_t treeBytes = 0;    // size of the tree portion; 0 iff data is empty (pure box)
  float dimAsBoxDist = 0;
  bool isBox() const { return data.empty(); }
};
} // namespace daSWRT

struct RenderSWRT
{
  void init();
  void close();
  ~RenderSWRT();

  int addBoxModel(vec4f bmin, vec4f bmax);

  // Builds a BuiltBLAS from a raw index/vertex mesh. Detects pure axis-aligned boxes
  // automatically (the returned BuiltBLAS is isBox() in that case). Thread-safe: does not
  // touch any RenderSWRT state, safe to invoke from threadpool workers. For threshold
  // semantics of `dim_as_box_dist` see addBoxModel / addBuiltModel. Returns an empty
  // (isBox(), zero box) BuiltBLAS for invalid input (which, if passed to addBuiltModel,
  // registers a zero-volume box).
  static daSWRT::BuiltBLAS buildBLAS(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
    float dim_as_box_dist = 0);

  // Registers a BuiltBLAS into SWRT. Small, fast: appends bytes into the internal concat
  // buffer and pushes to the tracking arrays. Not thread-safe; parallel callers must
  // serialize with their own mutex. Moves `built` in.
  int addBuiltModel(daSWRT::BuiltBLAS &&built);

  // Convenience wrapper: buildBLAS + addBuiltModel. For parallel add, prefer calling
  // buildBLAS from worker threads and addBuiltModel under a mutex so the heavy build
  // stays outside the critical section.
  int addModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
    float threshold_to_dim_as_box = 0);

  // Legacy entry for callers that still produce externally-serialized uint16-quantized BLAS
  // bytes (see build_bvh::writeQuadBLAS). Re-encodes to FP16 and registers. New code should
  // build a BuiltBLAS directly and use addBuiltModel. Validation runs before any side
  // effects; rejected input leaves SWRT state unchanged.
  int addPreBuiltModel(bbox3f box, dag::Vector<uint8_t> &&blas_data, int verts_count, int tree_nodes_count, int prims_count,
    float dim_as_box_dist = 0);

  inline void addInstance(int model_index, const mat43f &tm)
  {
    G_ASSERT(uint32_t(model_index) < blasDataInfo.size());
    instances.push_back(model_index);
    matrices.push_back(tm);
  }

  build_bvh::BLASData *buildBottomLevelStructures(uint32_t workers);
  build_bvh::TLASData *buildTopLevelStructures();
  void copyToGPU(build_bvh::BLASData *);
  void copyToGPUAndDestroy(build_bvh::BLASData *);
  void copyToGPUAndDestroy(build_bvh::TLASData *);
  static void destroy(build_bvh::BLASData *&);
  static void destroy(build_bvh::TLASData *&);

  void drawRT();
  void reserveAddInstances(int cnt);
  uint32_t currentInstancesCount() const { return instances.size(); }
  void clearTLASSourceData();
  void clearBLASSourceData();
  void clearTLASBuffers();
  void clearBLASBuffers();
  void clearBLASBuiltData();
  void clearAll();

  static uint32_t getTileBufferSizeDwords(uint32_t w, uint32_t h);
  enum
  {
    NO_CHECKER_BOARD = 0
  };
  void renderShadows(const Point3 &dir_to_sun, float sun_size, uint32_t checkerboad_frame, Sbuffer *shadow_mask_buf,
    BaseTexture *shadow_target, bool set_target_var);
  bool hasData() const { return bool(topBuf); }

protected:
  ComputeShaderElement *checkerboard_shadows_swrt_cs = nullptr, *shadows_swrt_cs = nullptr, *mask_shadow_swrt_cs = nullptr;
  void renderShadowFrustumTiles(int w, int h, const Point3 &dir_to_sun, float sun_size);
  UniqueBufHolder bottomBuf;
  UniqueBufHolder topBuf;
  UniqueBufHolder topLeavesBuf;

  // Per-registered-model tracking, pushed from addBuiltModel/addBoxModel. Sizes stay in
  // lockstep; `blasDataInfo[i]` encodes {byte offset into blasBytes, tree byte count}, or
  // {BLAS_IS_BOX, 0} for pure box entries. `blasTotalSizeInfo[i]` is the full chunk size
  // (tree + vertex payload) for model i; 0 for pure boxes. Needed to locate and remove a
  // model's bytes from blasBytes -- blasDataInfo only gives tree span, not the full range.
  dag::Vector<build_bvh::BLASDataInfo> blasDataInfo;
  dag::Vector<uint16_t> blasDimAsBox;
  dag::Vector<bbox3f> blasBoxes;
  dag::Vector<uint32_t> blasTotalSizeInfo;

  // Concatenated FP16-encoded BLAS bytes for all non-box models added since the last
  // clearBLASSourceData. buildBottomLevelStructures copies this into a BLASData* for GPU
  // upload; callers who want to resume adding and rebuild simply keep addBuiltModel'ing.
  dag::Vector<uint8_t> blasBytes;

  dag::Vector<int> tlasLeavesOffsets;
  dag::Vector<mat43f> matrices;
  dag::Vector<int> instances;
  eastl::unique_ptr<ShaderMaterial> mat;
  ShaderElement *elem = 0;
  PostFxRenderer rt;
  bool inited = false;

  void build_tlas(Tab<bbox3f> &tlas_boxes, const dag::Vector<bbox3f> &blas_boxes);
};
