// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBVH.h>
#include <daBVH/dag_bvhSerialization.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_bvhReencode.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <vecmath/dag_vecMath.h>
#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_hlsl_floatx.h>

#include "swRTbuffer.h"
#include <daBVH/swBVHDefine.hlsli>
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>

using namespace build_bvh;

namespace build_bvh
{
struct BLASData
{
  Tab<uint8_t> data;
  // Flat leaf table + per-model ranges derived from `data` at build time (only when
  // RenderSWRT::buildBlasLeafTables is set), so they can never disagree with the uploaded
  // snapshot. Empty ranges = tables not built, copyToGPU uploads nothing extra.
  Tab<uint32_t> leafTable, leafRanges;
  BLASData(uint32_t s) : data(s) {}
};
void destroy(BLASData *&data) { del_it(data); }
}; // namespace build_bvh

void RenderSWRT::clearBLASBuiltData()
{
  blasDataInfo.clear();
  blasDataInfo.shrink_to_fit();
  blasBoxes.clear();
  blasBoxes.shrink_to_fit();
  blasDimAsBox.clear();
  blasDimAsBox.shrink_to_fit();
  blasTotalSizeInfo.clear();
  blasTotalSizeInfo.shrink_to_fit();
}

void RenderSWRT::clearBLASSourceData()
{
  blasBytes.clear();
  blasBytes.shrink_to_fit();
  blasBytesVertsFp16 = -1; // buffer emptied -- the next non-box append re-locks the format
}
void RenderSWRT::clearBLASBuffers()
{
  bottomBuf.close();
  blasLeafTableBuf.close();
  blasLeafRangesBuf.close();
}

int RenderSWRT::addBoxModel(vec4f bmin, vec4f bmax)
{
  daSWRT::BuiltBLAS b;
  b.box.bmin = bmin;
  b.box.bmax = bmax;
  return addBuiltModel(eastl::move(b));
}

// Shared build tail; idx_copy is renumbered in place by leafOrderVertexFetch, hence the
// mutable copy owned by the callers. out arrives with box/dimAsBoxDist/vertsFp16 filled.
static daSWRT::BuiltBLAS build_blas_impl(daSWRT::BuiltBLAS &&out, Tab<uint32_t> &idxCopy, const vec4f *verts4, int vertex_count,
  bool fp16_verts)
{
  const int index_count = (int)idxCopy.size();
  dag::Vector<vec4f> orderedVerts;
  build_bvh::leafOrderVertexFetch(idxCopy.data(), (unsigned)index_count, verts4, (unsigned)vertex_count, orderedVerts);
  const vec4f *bverts = orderedVerts.data();
  const int bvertCount = (int)orderedVerts.size();

  // Per-worker transient build storage goes through framemem (thread-local stack allocator); destructors
  // run in LIFO order at function exit, matching framemem's stack semantics.
  const int faceCount = index_count / 3;
  Tab<build_bvh::QuadPrim> prims(framemem_ptr());
  int quadCount = 0, singleCount = 0;
  build_bvh::buildQuadPrims(prims, quadCount, singleCount, idxCopy.data(), faceCount, bverts);
  // buildQuadPrims drops duplicate-index (zero-area) faces; an all-degenerate mesh yields no prims.
  // Not a box case: registering the full AABB would cast phantom occlusion, so report no-geometry
  // (addBuiltModel yields -1, matching the collision feeder's no-model convention). This also keeps
  // the empty out.data away from the treeBytes subtraction below, which would underflow.
  if (prims.empty())
  {
    out.noGeometry = true;
    return out;
  }

  // Pair into double-quad leaves (4 tris/leaf). RT models carry no per-node identity (no tri_ref /
  // per-node filtering on the GPU), so pairing is unconstrained (vert_group = nullptr).
  dag::Vector<build_bvh::DoubleQuadPrim> dqPrims;
  build_bvh::buildDoubleQuadPrims(dqPrims, prims.data(), (int)prims.size(), bverts);

  Tab<bbox3f> primBoxes(framemem_ptr());
  primBoxes.resize(dqPrims.size());
  build_bvh::addDoubleQuadPrimitivesAABBList(primBoxes.data(), dqPrims.data(), (int)dqPrims.size(), bverts);

  Tab<bbox3f> nodes(framemem_ptr());
  int maxDepth = 0;
  const int root = build_bvh::create_bvh_node_sah(nodes, primBoxes.data(), (uint32_t)dqPrims.size(), 4, maxDepth);

  // writeDoubleQuadBLAS returns false (and clears out.data) when the [tree][verts] span would push a
  // leaf's apex base past the unsigned 24-bit range (~64 MB, only pathological meshes). Fall back to the
  // analytic box (isBox() == data.empty()); SWRT then traces this model's AABB.
  const bool builtBlas = build_bvh::writeDoubleQuadBLAS(out.data, out.box, nodes.data(), root, dqPrims.data(), (int)dqPrims.size(),
    reinterpret_cast<const uint8_t *>(bverts), (int)sizeof(vec4f), bvertCount);
  if (!builtBlas)
    return out;

  // writeDoubleQuadBLAS packs [tree bytes | bvertCount * 12 bytes of float3] -- subtracting the vertex
  // payload gives the tree-only span used by reencodeQuadBlasToFP16 and later by writeTLASLeaf via
  // BLASDataInfo::size. bvertCount is the reordered+dup'd count, not the source vertex_count.
  G_ASSERT(out.data.size() > (size_t)bvertCount * 12);
  out.treeBytes = (uint32_t)(out.data.size() - (size_t)bvertCount * 12);

  // FP16 re-encode runs here (per worker) rather than in copyToGPU: the function walks a
  // single model's tree in place and is fully model-local, so moving it to build time means
  // copyToGPU has no per-model loop and addBuiltModel can just append pre-encoded bytes.
  // It also compacts the vertex pool to blasGpuVertStride(fp16_verts) bytes/vert (8 when fp16), so trim
  // the now-unused tail; treeBytes is unchanged (the tree structure is preserved).
  build_bvh::reencodeQuadBlasToFP16(out.data.data(), 0, (int)out.treeBytes, bvertCount, BVH_BLAS_LEAF_SIZE, fp16_verts);
  out.data.resize((size_t)out.treeBytes + (size_t)bvertCount * build_bvh::blasGpuVertStride(fp16_verts));
  return out;
}

daSWRT::BuiltBLAS RenderSWRT::buildBLAS(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist, bool fp16_verts)
{
  daSWRT::BuiltBLAS out;
  G_ASSERT((index_count % 3) == 0);
  // Empty input becomes a zero-volume box if handed to addBuiltModel. The assert flags the
  // misuse but we still return a defined value so the caller's parallel_for cannot
  // miscompile on it.
  G_ASSERT_RETURN(vertex_count > 0 && index_count > 0, out);

  const vec4f *verts4 = (const vec4f *)vertices;
  out.box = build_bvh::calcBox(verts4, vertex_count);
  out.dimAsBoxDist = dim_as_box_dist;
  out.vertsFp16 = fp16_verts;

  if (build_bvh::checkIfIsBox(indices, index_count, verts4, vertex_count, out.box))
    return out; // isBox() == true, box populated

  // SAH-leaf-order renumber + shared window-block over-spread dup (build_bvh, see dag_bvhBuild.h) so
  // every triangle's vertex offsets fit the quad leaf's signed 13-bit fields. Indices are uint32 so the
  // dup tail can exceed 65536; the leaf stores a 24-bit apex byte base, while the per-vertex window is
  // kept in range by the window dedup. orderedVerts is the BLAS vert array.
  Tab<uint32_t> idxCopy(framemem_ptr());
  idxCopy.resize(index_count);
  for (int i = 0; i < index_count; ++i)
    idxCopy[i] = indices[i];
  return build_blas_impl(eastl::move(out), idxCopy, verts4, vertex_count, fp16_verts);
}

daSWRT::BuiltBLAS RenderSWRT::buildBLAS(const uint32_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist, bool fp16_verts)
{
  daSWRT::BuiltBLAS out;
  G_ASSERT((index_count % 3) == 0);
  G_ASSERT_RETURN(vertex_count > 0 && index_count > 0, out);

  const vec4f *verts4 = (const vec4f *)vertices;
  out.box = build_bvh::calcBox(verts4, vertex_count);
  out.dimAsBoxDist = dim_as_box_dist;
  out.vertsFp16 = fp16_verts;

  // no box detection: this entry exists for wide (>65535 vert) meshes, which cannot be 8-vert boxes
  Tab<uint32_t> idxCopy(framemem_ptr());
  idxCopy.resize(index_count);
  memcpy(idxCopy.data(), indices, index_count * sizeof(uint32_t)); // leafOrderVertexFetch renumbers in place
  return build_blas_impl(eastl::move(out), idxCopy, verts4, vertex_count, fp16_verts);
}

int RenderSWRT::addModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist)
{
  G_ASSERT((index_count % 3) == 0);
  G_ASSERT_RETURN(vertex_count > 0 && index_count > 0, -1);
  return addBuiltModel(buildBLAS(indices, index_count, vertices, vertex_count, dim_as_box_dist, blasVertsFp16));
}

int RenderSWRT::addBuiltModel(daSWRT::BuiltBLAS &&built)
{
  if (built.noGeometry)
    return -1; // all faces degenerate: no model (callers already handle -1 as "no SWRT model")

  const int modelId = (int)blasDataInfo.size();
  G_ASSERT(modelId == (int)blasBoxes.size());
  G_ASSERT(modelId == (int)blasDimAsBox.size());
  G_ASSERT(modelId == (int)blasTotalSizeInfo.size());

  if (built.isBox())
  {
    blasDataInfo.push_back({BLAS_IS_BOX, 0});
    blasDimAsBox.push_back(0);
    blasBoxes.push_back(built.box);
    blasTotalSizeInfo.push_back(0);
    return modelId;
  }

  G_ASSERT((built.data.size() % 4) == 0);
  G_ASSERT(built.treeBytes > 0 && built.treeBytes <= built.data.size());
  // The bytes were serialized for built.vertsFp16; the shader decodes the whole bottomBuf with one stride
  // (blasVertsFp16), so every appended model must share that format. Enforce at runtime, not just in dev:
  // a wrong-format upload would silently misread the vertex pool (wrong stride and decode).
  G_ASSERTF_RETURN(built.vertsFp16 == blasVertsFp16, -1, "BuiltBLAS vertsFp16=%d but RenderSWRT::blasVertsFp16=%d",
    (int)built.vertsFp16, (int)blasVertsFp16);
  // The format also freezes on the first non-box append: a later blasVertsFp16 flip would leave the
  // already-appended bytes in the old layout while the shader reads the buffer with the new stride.
  G_ASSERTF_RETURN(blasBytes.empty() || blasBytesVertsFp16 == (int)built.vertsFp16, -1,
    "blasVertsFp16 flipped to %d after BLAS bytes were appended as %d", (int)built.vertsFp16, (int)blasBytesVertsFp16);
  const int offsetBytes = (int)blasBytes.size();
  G_ASSERT(offsetBytes % 4 == 0);
  const uint32_t totalBytes = (uint32_t)built.data.size();
  blasBytes.insert(blasBytes.end(), built.data.begin(), built.data.end());
  blasBytesVertsFp16 = (int8_t)built.vertsFp16; // lock the buffer's vertex format (see the guards above)

  blasDataInfo.push_back({offsetBytes / BVH_BLAS_ELEM_SIZE, built.treeBytes});
  blasDimAsBox.push_back(built.dimAsBoxDist > 0 ? clamp<int>(ceilf(built.dimAsBoxDist + 0.001f), 1, 65535) : 65535);
  blasBoxes.push_back(built.box);
  blasTotalSizeInfo.push_back(totalBytes);
  return modelId;
}

int RenderSWRT::addPreBuiltModel(bbox3f box, dag::Vector<uint8_t> &&blas_data, int verts_count, int tree_nodes_count, int prims_count,
  float dim_as_box_dist)
{
  // tree_nodes_count is not part of the byte check below: writeDoubleQuadBLAS suppresses the root
  // node's box and interleaves 16-byte nodes with 28-byte leaves, so node count is not a clean multiple.
  G_UNUSED(tree_nodes_count);
  // Provenance contract: blas_data is freshly serialized in-memory by the current builder
  // (CollisionResource CPU build -> SWRT, e.g. collisionSwrtFeeder), not an older on-disk BLAS read
  // back. So although these are uint16 serialization-format bytes, the leaf layout is always the
  // current 28-byte double-quad and the size checks below suffice. If a persisted/old-version BLAS
  // load path is ever added, gate the leaf format/version here before reencodeQuadBlasToFP16 walks it.
  // Validate everything BEFORE touching any state, so a rejected input leaves storage untouched.
  G_ASSERT_RETURN(!blas_data.empty() && (blas_data.size() % 4) == 0, -1);
  G_ASSERT_RETURN(verts_count > 0 && prims_count > 0, -1);
  // [tree][float3 verts] layout: the tree holds prims_count 28-byte leaves (one per prim) plus the
  // internal nodes, so the buffer is at least the leaves plus the float3 vertex pool. Lower bound, not
  // exact: internal-node bytes vary, so this only catches a grossly truncated buffer.
  G_ASSERT_RETURN((int64_t)blas_data.size() >= (int64_t)verts_count * 12 + (int64_t)prims_count * BVH_BLAS_LEAF_SIZE, -1);

  const uint32_t treeBytes = (uint32_t)(blas_data.size() - (size_t)verts_count * 12);
  // Caller handed us uint16-quantized bytes (the on-disk serialization format). Re-encode
  // to FP16 here so the rest of the pipeline stays on a single in-memory representation. This also
  // compacts verts to blasGpuVertStride(blasVertsFp16) bytes/vert, so trim the tail (treeBytes unchanged).
  build_bvh::reencodeQuadBlasToFP16(blas_data.data(), 0, (int)treeBytes, verts_count, BVH_BLAS_LEAF_SIZE, blasVertsFp16);
  blas_data.resize((size_t)treeBytes + (size_t)verts_count * build_bvh::blasGpuVertStride(blasVertsFp16));

  daSWRT::BuiltBLAS built;
  built.box = box;
  built.data = eastl::move(blas_data);
  built.treeBytes = treeBytes;
  built.dimAsBoxDist = dim_as_box_dist;
  built.vertsFp16 = blasVertsFp16; // reencoded just above into this instance's format
  return addBuiltModel(eastl::move(built));
}

void RenderSWRT::destroy(BLASData *&d) { del_it(d); }

void RenderSWRT::copyToGPU(BLASData *d)
{
  if (!d || d->data.empty())
    return;
  // FP16 re-encode already happened per-model in buildBLAS / addPreBuiltModel, so this is a
  // straight byte-for-byte upload.
  ensure_buf_size_and_update(bottomBuf, d->data, "bvh_bottom_structures");
  if (!d->leafRanges.empty())
  {
    ensure_buf_size_and_update(blasLeafTableBuf, (const uint8_t *)d->leafTable.data(), data_size(d->leafTable), "bvh_blas_leaf_table");
    ensure_buf_size_and_update(blasLeafRangesBuf, (const uint8_t *)d->leafRanges.data(), data_size(d->leafRanges),
      "bvh_blas_leaf_ranges");
  }
}

void RenderSWRT::copyToGPUAndDestroy(BLASData *d)
{
  copyToGPU(d);
  del_it(d);
}

build_bvh::BLASData *RenderSWRT::buildBottomLevelStructures()
{
  // With addBuiltModel doing the concat+FP16-encode at add time, the "build" step is
  // effectively a handoff: copy the accumulated bytes into a BLASData* so the caller can
  // upload and destroy. We copy rather than move so repeat buildBottomLevelStructures calls
  // after further addBuiltModel stay coherent -- blasDataInfo offsets remain valid across
  // builds because blasBytes keeps all ranges.
  if (blasDataInfo.empty())
    return nullptr;
  auto *ret = new build_bvh::BLASData{0};
  ret->data.assign(blasBytes.begin(), blasBytes.end());
  if (buildBlasLeafTables)
  {
    // flat leaf table for geometry-driven consumers: [ranges: model count, then per model
    // {blasStart, leafStart, leafCount, 0} ascending by blasStart] + [table: leaf body byte
    // offsets]. Derived here, where blasDataInfo and the snapshot are in sync by construction.
    Tab<uint32_t> &leafTable = ret->leafTable, &leafRanges = ret->leafRanges;
    leafRanges.push_back(0);
    for (const build_bvh::BLASDataInfo &bi : blasDataInfo)
    {
      if (bi.isBox())
        continue;
      const uint32_t start = (uint32_t)bi.start, treeEnd = start + bi.size;
      const uint32_t leafStart = leafTable.size();
      for (uint32_t o = start; o < treeEnd;)
      {
        uint32_t skip;
        memcpy(&skip, &ret->data[o + 12], sizeof(skip));
        o += BVH_BLAS_NODE_SIZE;
        if (skip & QUAD_LEAF_FLAG)
        {
          leafTable.push_back(o);
          o += BVH_BLAS_LEAF_SIZE - BVH_BLAS_NODE_SIZE;
        }
      }
      leafRanges.push_back(start);
      leafRanges.push_back(leafStart);
      leafRanges.push_back(leafTable.size() - leafStart);
      leafRanges.push_back(0);
    }
    leafRanges[0] = (leafRanges.size() - 1) / 4;
    if (leafTable.empty())
      leafTable.push_back(0);
  }
  debug("wrote BVH BLAS (quad) pre-built, %d models, %d bytes", (int)blasDataInfo.size(), (int)blasBytes.size());
  return ret;
}
