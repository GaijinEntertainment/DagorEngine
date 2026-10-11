//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <EASTL/vector.h>
#include <generic/dag_span.h>
#include <render/occlusion/parallelOcclusionRasterizer.h>
#include <vecmath/dag_vecMath.h>

class CollisionResource;
struct CollisionNode;
struct RenderSWRT;
namespace daSWRT
{
struct BuiltBLAS;
enum class BlasBoxEncoding : uint8_t;
} // namespace daSWRT

// Shared friend-access point for consuming CollisionResource mesh/convex node geometry
// from external rasterizer/BVH feeders. Static methods are split across gameLibs:
//   collisionOccluders     -- implements addRasterizationTasks
//   collisionSwrtFeeder    -- implements buildSwrtBLAS and buildSwrtBLASFromCollisionResource
class CollisionGeometryFeeder
{
public:
  // Skips nodes with any PhysMat::lightTransparent material (glass must not occlude).
  static void addRasterizationTasks(const CollisionResource &coll_res, mat44f_cref worldviewproj,
    eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &out_tasks, uint32_t triangles_partition, bool allow_convex);

  // node_filter: optional predicate on a phys_mat_id, routed through the resource's material-filtered
  // face enumeration -- so its granularity is that pair's: all-or-nothing per node for a
  // single-material node, per face for a fused node.
  //              When unbound, every TRACEABLE mesh/convex node with non-empty indices is emitted.
  using PhysMatFilter = eastl::fixed_function<sizeof(void *) * 2, bool(int16_t phys_mat_id)>;
  // The QUAD_LEAF_USER_BITS value a face's BLAS leaf carries, by its phys_mat_id or PHYSMAT_INVALID, for a traced
  // hit to read back, plus QUAD_LEAF_USER_TRANSPARENT for a light-transparent face: its leaf takes the W2 flag,
  // and a model whose faces all carry it (a box too) is flagged whole, so both are skipped by a shadow walk
  // built with SWRT_SHADOW_SKIP_LEAF. Unbound, a leaf keeps its node's palette index, which a hit on joined nodes cannot resolve.
  // shortcut: one value table for all models - GPU phys mat ids need a per-model palette buffer and leaf_user given its index.
  using PhysMatLeafUser = eastl::fixed_function<sizeof(void *) * 2, uint8_t(int16_t phys_mat_id)>;

  // Xray GPU upload helper. Invokes cb with raw pointers valid only during the call.
  // In future BVH impl, verts_ptr may reference transient decoded scratch.
  using NodeMeshConsumer = eastl::fixed_function<sizeof(void *) * 4,
    void(const void *verts_ptr, int vert_count, int vert_elem_size, const void *indices_ptr, int index_count, int index_elem_size)>;
  static void withNodeMeshData(const CollisionResource &coll_res, int node_id, const NodeMeshConsumer &cb);

  // What the chunk splice says about the bytes it produced (filled on success).
  struct ChunkSpliceInfo
  {
    daSWRT::BlasBoxEncoding boxEncoding{}; // the byte format of built's tree boxes: the producer names it
  };
  // Joins the chunks of node_indices into one model in built, without the SWRT runtime: box = the union of the chunk frames
  // in resource space (through getNodeTm for a non-IDENT node), data = the stackless trees back to back, then the fp16 verts
  // at align8(treeBytes). A lone 8-vert node gives the box model when its kept faces close an axis-aligned box
  // with one transparency and either no leaf value or, with box_user, one shared value (BuiltBLAS::boxUser; pass swrt.tlasBoxUser).
  // Otherwise false for a node without a chunk, a corrupt chunk (logerr), a leaf base past 24 bits, or a mat_pred carving every leaf.
  // mat_pred drops the leaves of rejected materials (unbound = keep all); leaf_user restamps each leaf by its palette material.
  // The caller owns the fp16 requirement, scores dimAsBoxDist and registers the result.
  static bool buildSwrtChunkSpliceBLAS(const CollisionResource &coll_res, dag::ConstSpan<int> node_indices, daSWRT::BuiltBLAS &built,
    ChunkSpliceInfo &out_info, const PhysMatFilter &mat_pred = {}, const PhysMatLeafUser &leaf_user = {}, bool box_user = false);

  // Build a SWRT BLAS from a CollisionResource into built: splice the chunks of every traceable mesh/convex node
  // (buildSwrtChunkSpliceBLAS) and score it, or false on empty/invalid input. Touches no shared state,
  // so workers build in parallel and register under the caller's lock with addBuiltModel, which takes fp16 verts only.
  // dim_as_box_min/max: distance range (metres) for the runtime "treat-as-box" shadow
  // optimization. The actual per-BLAS distance is interpolated from a voxel-rasterized box-
  // resemblance score: a near-box mesh gets `dim_as_box_min` (aggressive: collapses to AABB
  // even up close), a non-box mesh gets `dim_as_box_max` (conservative: only collapses far
  // away). Pass max==min to disable scoring and pin every mesh to a single distance. A lone
  // 8-vert axis-aligned box bypasses this entirely as a box model.
  // leaf_user and box_user: as in buildSwrtChunkSpliceBLAS.
  static bool buildSwrtBLAS(const CollisionResource &coll_res, const PhysMatFilter &node_filter, float dim_as_box_min,
    float dim_as_box_max, const PhysMatLeafUser &leaf_user, bool box_user, daSWRT::BuiltBLAS &built);
  // buildSwrtBLAS registered in swrt (fp16 verts only): the model id (>= 0), or -1. The build is re-entrant, the registration not.
  static int buildSwrtBLASFromCollisionResource(RenderSWRT &swrt, const CollisionResource &coll_res, const PhysMatFilter &node_filter,
    float dim_as_box_min, float dim_as_box_max, const PhysMatLeafUser &leaf_user = {});
};
