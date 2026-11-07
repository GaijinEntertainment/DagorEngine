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
  daSWRT::BlasMode mode = daSWRT::BlasMode::Full;
  BLASData(uint32_t s) : data(s) {}
};
void destroy(BLASData *&data) { del_it(data); }
}; // namespace build_bvh

static ShaderVariableInfo bvh_blas_boxes_onlyVarId("bvh_blas_boxes_only", true);

void RenderSWRT::clearBLASBuiltData()
{
  blasDataInfo.clear();
  blasDataInfo.shrink_to_fit();
  blasBoxes.clear();
  blasBoxes.shrink_to_fit();
  blasDimAsBox.clear();
  blasDimAsBox.shrink_to_fit();
  blasLightTransparent.clear();
  blasLightTransparent.shrink_to_fit();
  blasBoxUser.clear();
  blasBoxUser.shrink_to_fit();
  blasTotalSizeInfo.clear();
  blasTotalSizeInfo.shrink_to_fit();
}

void RenderSWRT::clearBLASSourceData()
{
  blasBytes.clear();
  blasBytes.shrink_to_fit();
  blasBytesVertsFp16 = -1; // buffer emptied -- the next non-box append re-locks the format
  blasBytesMode = -1;
}
void RenderSWRT::clearBLASBuffers()
{
  bottomBuf.close();
  blasLeafTableBuf.close();
  blasLeafRangesBuf.close();
  gpuBlasMode = daSWRT::BlasMode::Full;
}

int RenderSWRT::addBoxModel(vec4f bmin, vec4f bmax, bool light_transparent)
{
  daSWRT::BuiltBLAS b;
  b.box.bmin = bmin;
  b.box.bmax = bmax;
  b.lightTransparent = light_transparent;
  return addBuiltModel(eastl::move(b));
}

bool daSWRT::BuiltBLAS::compactToBoxLeaves()
{
  const int boxTreeBytes = build_bvh::compactQuadBlasToBoxLeaves(data.data(), (int)treeBytes);
  treeBytes = max(boxTreeBytes, 0);
  data.resize(treeBytes);
  noGeometry = treeBytes == 0;
  return !noGeometry;
}

// Shared build tail; idx_copy is renumbered in place by leafOrderVertexFetch, hence the
// mutable copy owned by the callers. out arrives with box/dimAsBoxDist/vertsFp16/mode filled.
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
  // the now-unused tail; Full keeps treeBytes, BoxesOnly compacts the tree and drops the vertex pool.
  const bool boxesOnly = out.mode == daSWRT::BlasMode::BoxesOnly;
  build_bvh::reencodeQuadBlasToFP16(out.data.data(), 0, (int)out.treeBytes, boxesOnly ? 0 : bvertCount, BVH_BLAS_LEAF_SIZE,
    fp16_verts && !boxesOnly);
  if (boxesOnly)
  {
    out.compactToBoxLeaves();
    return out;
  }
  out.data.resize((size_t)out.treeBytes + (size_t)bvertCount * build_bvh::blasGpuVertStride(fp16_verts));
  return out;
}

daSWRT::BuiltBLAS RenderSWRT::buildBLAS(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist, bool fp16_verts, daSWRT::BlasMode mode)
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
  out.mode = mode;

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
  float dim_as_box_dist, bool fp16_verts, daSWRT::BlasMode mode)
{
  daSWRT::BuiltBLAS out;
  G_ASSERT((index_count % 3) == 0);
  G_ASSERT_RETURN(vertex_count > 0 && index_count > 0, out);

  const vec4f *verts4 = (const vec4f *)vertices;
  out.box = build_bvh::calcBox(verts4, vertex_count);
  out.dimAsBoxDist = dim_as_box_dist;
  out.vertsFp16 = fp16_verts;
  out.mode = mode;

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
  return addBuiltModel(buildBLAS(indices, index_count, vertices, vertex_count, dim_as_box_dist, blasVertsFp16, blasMode));
}

int RenderSWRT::addBuiltModel(daSWRT::BuiltBLAS &&built)
{
  if (built.noGeometry)
    return -1; // all faces degenerate: no model (callers already handle -1 as "no SWRT model")

  const int modelId = (int)blasDataInfo.size();
  G_ASSERT(modelId == (int)blasBoxes.size());
  G_ASSERT(modelId == (int)blasDimAsBox.size());
  G_ASSERT(modelId == (int)blasTotalSizeInfo.size());
  G_ASSERT(modelId == (int)blasLightTransparent.size());
  G_ASSERT(modelId == (int)blasBoxUser.size());

  if (built.isBox())
  {
    blasDataInfo.push_back({BLAS_IS_BOX, 0});
    blasDimAsBox.push_back(0);
    blasBoxes.push_back(built.box);
    blasTotalSizeInfo.push_back(0);
    blasLightTransparent.push_back(built.lightTransparent ? 1 : 0);
    blasBoxUser.push_back(built.boxUser);
    return modelId;
  }

  G_ASSERT((built.data.size() % 4) == 0);
  G_ASSERT(built.treeBytes > 0 && built.treeBytes <= built.data.size());
  // one bottomBuf holds one layout
  G_ASSERTF_RETURN(built.mode == blasMode && (blasBytes.empty() || blasBytesMode == (int)built.mode), -1,
    "BuiltBLAS mode %d, RenderSWRT::blasMode %d, BLAS bytes appended as %d", (int)built.mode, (int)blasMode, (int)blasBytesMode);
  const bool boxesOnly = built.mode == daSWRT::BlasMode::BoxesOnly;
  // The bytes were serialized for built.vertsFp16; the shader decodes the whole bottomBuf with one stride
  // (blasVertsFp16), so every appended model must share that format. Enforce at runtime, not just in dev:
  // a wrong-format upload would silently misread the vertex pool (wrong stride and decode).
  G_ASSERTF_RETURN(boxesOnly || built.vertsFp16 == blasVertsFp16, -1, "BuiltBLAS vertsFp16=%d but RenderSWRT::blasVertsFp16=%d",
    (int)built.vertsFp16, (int)blasVertsFp16);
  // The format also freezes on the first non-box append: a later blasVertsFp16 flip would leave the
  // already-appended bytes in the old layout while the shader reads the buffer with the new stride.
  G_ASSERTF_RETURN(boxesOnly || blasBytes.empty() || blasBytesVertsFp16 == (int)built.vertsFp16, -1,
    "blasVertsFp16 flipped to %d after BLAS bytes were appended as %d", (int)built.vertsFp16, (int)blasBytesVertsFp16);
  const int offsetBytes = (int)blasBytes.size();
  G_ASSERT(offsetBytes % 4 == 0);
  const uint32_t totalBytes = (uint32_t)built.data.size();
  blasBytes.insert(blasBytes.end(), built.data.begin(), built.data.end());
  blasBytesVertsFp16 = (int8_t)built.vertsFp16; // lock the buffer's vertex format (see the guards above)
  blasBytesMode = (int8_t)built.mode;

  blasDataInfo.push_back({offsetBytes / BVH_BLAS_ELEM_SIZE, built.treeBytes});
  blasDimAsBox.push_back(built.dimAsBoxDist > 0 ? clamp<int>(ceilf(built.dimAsBoxDist + 0.001f), 1, 65535) : 65535);
  blasBoxes.push_back(built.box);
  blasTotalSizeInfo.push_back(totalBytes);
  blasLightTransparent.push_back(built.lightTransparent ? 1 : 0);
  blasBoxUser.push_back(0);
  return modelId;
}

void RenderSWRT::destroy(BLASData *&d) { del_it(d); }

void RenderSWRT::copyToGPU(BLASData *d)
{
  if (!d || d->data.empty())
    return;
  const int boxesOnly = d->mode == daSWRT::BlasMode::BoxesOnly ? 1 : 0;
  if (ShaderGlobal::is_var_assumed(bvh_blas_boxes_onlyVarId) &&
      ShaderGlobal::get_interval_assumed_value(bvh_blas_boxes_onlyVarId) != boxesOnly)
  {
    LOGERR_ONCE("SWRT: the shader dump assumes bvh_blas_boxes_only %d, the BLAS is built with %d; no BLAS upload",
      ShaderGlobal::get_interval_assumed_value(bvh_blas_boxes_onlyVarId), boxesOnly);
    clearBLASBuffers();
    return;
  }
  ShaderGlobal::set_int(bvh_blas_boxes_onlyVarId, boxesOnly);
  gpuBlasMode = d->mode;
  // FP16 re-encode already happened per-model in buildBLAS, so this is a straight byte-for-byte upload.
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
  if (blasBytesMode == (int8_t)daSWRT::BlasMode::BoxesOnly)
    ret->mode = daSWRT::BlasMode::BoxesOnly;
  G_ASSERTF(!buildBlasLeafTables || ret->mode == daSWRT::BlasMode::Full, "SWRT: the BLAS leaf tables need the leaf triangles");
  if (buildBlasLeafTables && ret->mode == daSWRT::BlasMode::Full)
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
  uint32_t treeBytes = 0;
  for (const build_bvh::BLASDataInfo &bi : blasDataInfo)
    treeBytes += bi.size;
  debug("wrote BVH BLAS (quad) pre-built, %d models, %d bytes: %u tree, %u vertex, %s", (int)blasDataInfo.size(),
    (int)blasBytes.size(), treeBytes, (uint32_t)blasBytes.size() - treeBytes,
    ret->mode == daSWRT::BlasMode::BoxesOnly ? "boxes only" : "full");
  return ret;
}
