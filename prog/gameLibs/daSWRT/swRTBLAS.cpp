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
#include "shaders/swBVHDefine.hlsli"
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>

using namespace build_bvh;

namespace build_bvh
{
struct BLASData
{
  Tab<uint8_t> data;
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
}
void RenderSWRT::clearBLASBuffers() { bottomBuf.close(); }

int RenderSWRT::addBoxModel(vec4f bmin, vec4f bmax)
{
  daSWRT::BuiltBLAS b;
  b.box.bmin = bmin;
  b.box.bmax = bmax;
  return addBuiltModel(eastl::move(b));
}

daSWRT::BuiltBLAS RenderSWRT::buildBLAS(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist)
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

  if (build_bvh::checkIfIsBox(indices, index_count, verts4, vertex_count, out.box))
    return out; // isBox() == true, box populated

  // Per-worker transient storage goes through framemem (thread-local stack allocator).
  // Destructors run in LIFO order at function exit, which matches framemem's stack
  // semantics; no heap allocations happen for the inner builders.
  const int faceCount = index_count / 3;
  Tab<build_bvh::QuadPrim> prims(framemem_ptr());
  int quadCount = 0, singleCount = 0;
  build_bvh::buildQuadPrims(prims, quadCount, singleCount, indices, faceCount, verts4);

  Tab<bbox3f> primBoxes(framemem_ptr());
  primBoxes.resize(prims.size());
  build_bvh::addQuadPrimitivesAABBList(primBoxes.data(), prims.data(), (int)prims.size(), verts4);

  Tab<bbox3f> nodes(framemem_ptr());
  int maxDepth = 0;
  const int root = build_bvh::create_bvh_node_sah(nodes, primBoxes.data(), (uint32_t)prims.size(), 4, maxDepth);

  build_bvh::writeQuadBLAS(out.data, out.box, nodes.data(), root, prims.data(), (int)prims.size(),
    reinterpret_cast<const uint8_t *>(vertices), (int)sizeof(Point3_vec4), vertex_count);

  // writeQuadBLAS packs [tree bytes | vertex_count * 12 bytes of float3] -- subtracting the
  // vertex payload gives the tree-only span used by reencodeQuadBlasToFP16 and later by
  // writeTLASLeaf via BLASDataInfo::size.
  G_ASSERT(out.data.size() > (size_t)vertex_count * 12);
  out.treeBytes = (uint32_t)(out.data.size() - (size_t)vertex_count * 12);

  // FP16 re-encode runs here (per worker) rather than in copyToGPU: the function walks a
  // single model's tree in place and is fully model-local, so moving it to build time means
  // copyToGPU has no per-model loop and addBuiltModel can just append pre-encoded bytes.
  build_bvh::reencodeQuadBlasToFP16(out.data.data(), 0, (int)out.treeBytes, vertex_count, BVH_BLAS_LEAF_SIZE);
  return out;
}

int RenderSWRT::addModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist)
{
  G_ASSERT((index_count % 3) == 0);
  G_ASSERT_RETURN(vertex_count > 0 && index_count > 0, -1);
  return addBuiltModel(buildBLAS(indices, index_count, vertices, vertex_count, dim_as_box_dist));
}

int RenderSWRT::addBuiltModel(daSWRT::BuiltBLAS &&built)
{
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
  const int offsetBytes = (int)blasBytes.size();
  G_ASSERT(offsetBytes % 4 == 0);
  const uint32_t totalBytes = (uint32_t)built.data.size();
  blasBytes.insert(blasBytes.end(), built.data.begin(), built.data.end());

  blasDataInfo.push_back({offsetBytes / BVH_BLAS_ELEM_SIZE, built.treeBytes});
  blasDimAsBox.push_back(built.dimAsBoxDist > 0 ? clamp<int>(ceilf(built.dimAsBoxDist + 0.001f), 1, 65535) : 65535);
  blasBoxes.push_back(built.box);
  blasTotalSizeInfo.push_back(totalBytes);
  return modelId;
}

int RenderSWRT::addPreBuiltModel(bbox3f box, dag::Vector<uint8_t> &&blas_data, int verts_count, int tree_nodes_count, int prims_count,
  float dim_as_box_dist)
{
  G_UNUSED(tree_nodes_count);
  G_UNUSED(prims_count);
  // Validate everything BEFORE touching any state, so a rejected input leaves storage untouched.
  G_ASSERT_RETURN(!blas_data.empty() && (blas_data.size() % 4) == 0, -1);
  G_ASSERT_RETURN(verts_count > 0, -1);
  G_ASSERT_RETURN((int)blas_data.size() > verts_count * 12, -1);

  const uint32_t treeBytes = (uint32_t)(blas_data.size() - (size_t)verts_count * 12);
  // Caller handed us uint16-quantized bytes (the on-disk serialization format). Re-encode
  // to FP16 here so the rest of the pipeline stays on a single in-memory representation.
  build_bvh::reencodeQuadBlasToFP16(blas_data.data(), 0, (int)treeBytes, verts_count, BVH_BLAS_LEAF_SIZE);

  daSWRT::BuiltBLAS built;
  built.box = box;
  built.data = eastl::move(blas_data);
  built.treeBytes = treeBytes;
  built.dimAsBoxDist = dim_as_box_dist;
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
}

void RenderSWRT::copyToGPUAndDestroy(BLASData *d)
{
  copyToGPU(d);
  del_it(d);
}

build_bvh::BLASData *RenderSWRT::buildBottomLevelStructures(uint32_t /*workers*/)
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
  debug("wrote BVH BLAS (quad) pre-built, %d models, %d bytes", (int)blasDataInfo.size(), (int)blasBytes.size());
  return ret;
}
