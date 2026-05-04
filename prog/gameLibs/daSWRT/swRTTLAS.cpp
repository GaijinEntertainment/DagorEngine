// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBVH.h>
#include <daBVH/dag_swTLAS.h>
#include <util/dag_stlqsort.h>
#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_carray.h>

#include "swCommon.h"
#include "swRTbuffer.h"
#include "shaders/swBVHDefine.hlsli"
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>

namespace build_bvh
{

struct TLASData
{
  Tab<uint8_t> data;
  TLASData(uint32_t s) : data(s) {}
};
void destroy(TLASData *&data) { del_it(data); }

} // namespace build_bvh

using namespace build_bvh;

void RenderSWRT::clearTLASSourceData()
{
  G_STATIC_ASSERT(sizeof(TLASNode) == TLAS_NODE_ELEM_SIZE);
  G_STATIC_ASSERT(sizeof(TLASLeaf) % 4 == 0);
  matrices.clear();
  matrices.shrink_to_fit();
  instances.clear();
  instances.shrink_to_fit();
  tlasLeavesOffsets.clear();
  tlasLeavesOffsets.shrink_to_fit();
}

void RenderSWRT::clearTLASBuffers()
{
  topBuf.close();
  topLeavesBuf.close();
}

void RenderSWRT::build_tlas(Tab<bbox3f> &tlas_boxes, const dag::Vector<bbox3f> &blas_boxes)
{
  int64_t reft = profile_ref_ticks();
  /*
  for leaf:
    min.w = instance id
    max.w = model id
  for node:
    min.w = -1 - left node id
    max.w = -1 - right node id
  */

  int n = (int)matrices.size();
  Tab<bbox3f> leaves(framemem_ptr());
  leaves.resize(n);

  for (int i = 0; i < n; i++)
  {
    mat44f tm;
    v_mat43_transpose_to_mat44(tm, matrices[i]);
    int modelId = instances[i];
    v_bbox3_init(leaves[i], tm, blas_boxes[modelId]);
    leaves[i].bmin = v_perm_xyzd(leaves[i].bmin, v_cast_vec4f(v_splatsi(i)));
    leaves[i].bmax = v_perm_xyzd(leaves[i].bmax, v_cast_vec4f(v_splatsi(modelId)));
  }

  tlas_boxes.reserve(leaves.size() * 2);
  int maxDepth = 0;
  int root = create_bvh_node_sah(tlas_boxes, leaves.begin(), leaves.size(), 4, maxDepth);
  debug("built BVH TLAS tree depth %d in %dus, %d instances, %d nodes", maxDepth, profile_time_usec(reft), matrices.size(),
    tlas_boxes.size());
  G_ASSERTF(maxDepth <= BVH_MAX_TLAS_DEPTH, "%d tlas depth too big, we should rebuild tree with better balancing", maxDepth);
  G_VERIFY(root == 0);
}

void RenderSWRT::destroy(TLASData *&d) { del_it(d); }

void RenderSWRT::copyToGPUAndDestroy(TLASData *d)
{
  if (!d)
    return;
  if (VariableMap::isVariablePresent(get_shader_variable_id("bvh_top_leaves", true)))
  {
    ensure_buf_size_and_update(topLeavesBuf, (const uint8_t *)tlasLeavesOffsets.data(),
      tlasLeavesOffsets.size() * sizeof(*tlasLeavesOffsets.begin()), "bvh_top_leaves");
  }

  ensure_buf_size_and_update(topBuf, d->data, "bvh_top_structures");
  del_it(d);
}

build_bvh::TLASData *RenderSWRT::buildTopLevelStructures()
{
  if (blasDataInfo.empty() || matrices.empty())
    return nullptr;

  // N leaves + (N - 1) intermediate nodes + stop node
  Tab<bbox3f> tlasBoxes(framemem_ptr());
  build_tlas(tlasBoxes, blasBoxes);

  G_ASSERTF(tlasBoxes.size() >= matrices.size(), "%d > %d", tlasBoxes.size(), matrices.size());
  // Allocate max size (all non-box); actual size may be smaller if boxes exist.
  // writeTLASTreeRoot will count boxes and the leavesOffset assert uses <=.
  const uint32_t maxTlasSize = (tlasBoxes.size() + 1) * sizeof(build_bvh::TLASNode) + matrices.size() * sizeof(build_bvh::TLASLeaf);

  int64_t reft = profile_ref_ticks();

  TLASData *ret = new TLASData{maxTlasSize};
  uint8_t *bvhData = ret->data.begin();
  uint32_t numBoxLeaves;
  {
    build_bvh::WriteTLASTreeInfo info = {};
    tlasLeavesOffsets.clear();
    if (topLeavesBuf)
    {
      tlasLeavesOffsets.resize(matrices.size());
      info.tlasLeavesOffsets = &tlasLeavesOffsets;
    }
    info.nodes = tlasBoxes.begin();
    info.bboxData = bvhData;
    info.tms = matrices.begin();
    info.blasData = blasDataInfo.begin();
    info.blasBoxes = blasBoxes.begin();
    info.boxDist = blasDimAsBox.begin();

    info.nodesSize = (tlasBoxes.size() + 1) * sizeof(build_bvh::TLASNode);
    info.leavesOffset = info.nodesSize;
    info.leavesSize = maxTlasSize;

    build_bvh::writeTLASTreeRoot(info);
    G_ASSERTF(info.nodesOffset == info.nodesSize, "dataOffset=%d nodesSize=%d", info.nodesOffset, info.nodesSize);
    G_ASSERTF(info.leavesOffset <= info.leavesSize, "leavesOffset=%d leavesSize=%d", info.leavesOffset, info.leavesSize);
    numBoxLeaves = info.numBoxLeaves;
  }

  const uint32_t tlasSize = (tlasBoxes.size() + 1) * sizeof(build_bvh::TLASNode) + numBoxLeaves * build_bvh::TLASLeaf::BOX_SIZE +
                            (matrices.size() - numBoxLeaves) * sizeof(build_bvh::TLASLeaf);
  G_ASSERT(ret->data.size() >= tlasSize);
  ret->data.resize(tlasSize); // trim to actual size

  debug("write BVH TLAS in %dus of %d bytes, %d instances (%d boxes)", profile_time_usec(reft), tlasSize, matrices.size(),
    numBoxLeaves);
  return ret;

  // G_ASSERT(build_bvh::max_tlas_depth <= BVH_MAX_TLAS_DEPTH);
}

void RenderSWRT::reserveAddInstances(int cnt)
{
  matrices.reserve(matrices.size() + cnt);
  instances.reserve(instances.size() + cnt);
}
