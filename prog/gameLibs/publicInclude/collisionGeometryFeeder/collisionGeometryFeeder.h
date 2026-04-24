//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <EASTL/vector.h>
#include <dag/dag_vector.h>
#include <math/dag_Point3.h>
#include <render/occlusion/parallelOcclusionRasterizer.h>
#include <vecmath/dag_vecMath.h>

class CollisionResource;
struct RenderSWRT;

// Shared friend-access point for consuming CollisionResource mesh/convex node geometry
// from external rasterizer/BVH feeders. Static methods are split across gameLibs:
//   collisionOccluders     -- implements addRasterizationTasks
//   collisionSwrtFeeder    -- implements buildSwrtBLASFromCollisionResource
class CollisionGeometryFeeder
{
public:
  static void addRasterizationTasks(const CollisionResource &coll_res, mat44f_cref worldviewproj,
    eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &out_tasks, uint32_t triangles_partition, bool allow_convex);

  // node_filter: optional predicate on node phys_mat_id; when bound, a node is included only if filter returns true.
  //              When unbound, every TRACEABLE mesh/convex node with non-empty indices is emitted.
  using PhysMatFilter = eastl::fixed_function<sizeof(void *) * 2, bool(int16_t phys_mat_id)>;

  // Xray GPU upload helper. Invokes cb with raw pointers valid only during the call.
  // In future BVH impl, verts_ptr may reference transient decoded scratch.
  using NodeMeshConsumer = eastl::fixed_function<sizeof(void *) * 4,
    void(const void *verts_ptr, int vert_count, int vert_elem_size, const void *indices_ptr, int index_count, int index_elem_size)>;
  static void withNodeMeshData(const CollisionResource &coll_res, int node_id, const NodeMeshConsumer &cb);

  // Caller-owned scratch for buildSwrtBLASFromCollisionResource. Reuse across calls to avoid
  // reallocation. All vectors are cleared at the start of each build; bring your own thread-local
  // instance if running builds in parallel.
  struct BuildSwrtBLASScratch
  {
    dag::Vector<Point3_vec4> verts;
    dag::Vector<uint32_t> indices; // uint32 to support CollisionResources with > 65536 verts
    dag::Vector<bbox3f> primBoxes;
  };

  // Build a SWRT BLAS directly from a CollisionResource and hand it to swrt via addPreBuiltModel
  // (or addBoxModel if the flattened mesh collapses to a pure box).
  // Avoids the flat-indices+verts round-trip through RenderSWRT::addModel by running the daBVH
  // SAH builder inside this function and moving the resulting tree/prims into SWRT.
  // Returns the SWRT model id (>= 0) on success, -1 on empty/invalid input. Re-entrant for the
  // BLAS build phase (scratch is caller-owned); the final hand-off to swrt (either addBoxModel or
  // addPreBuiltModel) mutates shared RenderSWRT state and is not thread-safe, so concurrent
  // callers must serialize that step.
  static int buildSwrtBLASFromCollisionResource(RenderSWRT &swrt, const CollisionResource &coll_res, const PhysMatFilter &node_filter,
    float dim_as_box_dist, BuildSwrtBLASScratch &scratch);
};
