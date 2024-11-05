// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_enumerate.h>
#include <perfMon/dag_statDrv.h>
#include <render/cables.h>

#include "bvh_context.h"

namespace bvh
{
Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);
}

namespace bvh::cables
{

static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE;

static constexpr int vertex_stride = sizeof(float) * 3 + sizeof(uint32_t);
static constexpr int index_stride = sizeof(uint32_t);

static MeshMetaAllocator::AllocId metaAllocId = -1;

void init() {}

void teardown() {}

void init(ContextId context_id)
{
  G_ASSERT(metaAllocId == -1);
  metaAllocId = context_id->allocateMetaRegion();
  MeshMeta &meta = context_id->meshMetaAllocator.get(metaAllocId);

  meta.markInitialized();
  meta.setIndexBit(4);
  meta.materialType |= MeshMeta::bvhMaterialRendinst;
  meta.texcoordNormalColorOffsetAndVertexStride = 0xFFFF0000U | ((sizeof(float) * 3) << 8) | vertex_stride;
}

void teardown(ContextId context_id)
{
  if (context_id->cableVertices)
    context_id->releaseBuffer(context_id->cableVertices.getBuf());
  if (context_id->cableIndices)
    context_id->releaseBuffer(context_id->cableIndices.getBuf());
  context_id->cableVertices.close();
  context_id->cableIndices.close();
  context_id->cableBLASes.clear();
  if (metaAllocId > -1)
    context_id->freeMetaRegion(metaAllocId);
}

static bool create_blas(ContextId context_id, Cables *cables, int index)
{
  auto triangleCount = cables->getTranglesPerCable();

  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = context_id->cableVertices.getBuf();
  desc.data.triangles.vertexCount = triangleCount + 2;
  desc.data.triangles.vertexStride = vertex_stride;
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.indexBuffer = context_id->cableIndices.getBuf();
  desc.data.triangles.indexCount = triangleCount * 3;
  desc.data.triangles.indexOffset = triangleCount * 3 * index;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::IS_OPAQUE;

  auto blas = UniqueBLAS::create(&desc, 1, blas_flags);
  HANDLE_LOST_DEVICE_STATE(blas, false);

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.flags = blas_flags;
  buildInfo.geometryDesc = &desc;
  buildInfo.geometryDescCount = 1;
  buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = blas.getBuildScratchSize();
  HANDLE_LOST_DEVICE_STATE(buildInfo.scratchSpaceBuffer, false);

  d3d::build_bottom_acceleration_structure(blas.get(), buildInfo);

  context_id->cableBLASes.push_back(eastl::move(blas));

  return true;
}

void on_cables_changed(Cables *cables, ContextId context_id)
{
  if (!context_id->has(Features::Cable))
    return;

  context_id->cableBLASes.clear();

  auto maxCables = cables->getMaxCables();
  if (maxCables == 0)
    return;
  auto cablesData = cables->getCablesData();
  if (!cablesData || cablesData->empty())
    return;

  auto triPerCable = cables->getTranglesPerCable();
  auto triangleCount = triPerCable * maxCables;
  auto vertexCount = (triPerCable + 2) * maxCables;
  auto indexCount = triangleCount * 3;

  if (context_id->cableVertices && context_id->cableVertices->getNumElements() < vertexCount)
  {
    context_id->releaseBuffer(context_id->cableVertices.getBuf());
    context_id->cableVertices.close();
  }
  if (context_id->cableIndices && context_id->cableIndices->getNumElements() < indexCount)
  {
    context_id->releaseBuffer(context_id->cableIndices.getBuf());
    context_id->cableIndices.close();
  }

  if (!context_id->cableVertices)
  {
    context_id->cableVertices = dag::buffers::create_ua_sr_structured(vertex_stride, vertexCount, "bvh_cable_vertices");
    HANDLE_LOST_DEVICE_STATE(context_id->cableVertices, );

    uint32_t bindlessIndex;
    context_id->holdBuffer(context_id->cableVertices.getBuf(), bindlessIndex);

    MeshMeta &meta = context_id->meshMetaAllocator.get(metaAllocId);
    meta.indexAndVertexBufferIndex &= 0xFFFF0000U;
    meta.indexAndVertexBufferIndex |= bindlessIndex;
  }

  if (!context_id->cableIndices)
  {
    context_id->cableIndices =
      dag::create_sbuffer(index_stride, indexCount, SBCF_UA_SR_STRUCTURED | SBCF_INDEX32, 0, "bvh_cable_indices");
    HANDLE_LOST_DEVICE_STATE(context_id->cableIndices, );

    uint32_t bindlessIndex;
    context_id->holdBuffer(context_id->cableIndices.getBuf(), bindlessIndex);

    MeshMeta &meta = context_id->meshMetaAllocator.get(metaAllocId);
    meta.indexAndVertexBufferIndex &= 0xFFFFU;
    meta.indexAndVertexBufferIndex |= bindlessIndex << 16;
  }

  static int bvh_cables_vertices_buf_reg_no = ShaderGlobal::get_int(get_shader_variable_id("bvh_cables_vertices_buf_reg_no"));
  static int bvh_cables_indices_buf_reg_no = ShaderGlobal::get_int(get_shader_variable_id("bvh_cables_indices_buf_reg_no"));

  d3d::set_rwbuffer(STAGE_VS, bvh_cables_vertices_buf_reg_no, context_id->cableVertices.getBuf());
  d3d::set_rwbuffer(STAGE_VS, bvh_cables_indices_buf_reg_no, context_id->cableIndices.getBuf());

  cables->render(Cables::RENDER_PASS_BVH);

  for (auto cableIx : cables->getDirtyCables())
    if (!create_blas(context_id, cables, cableIx))
      return;

  cables->getDirtyCables().clear();

  // next usage is hopefully, read from, so barrier it up
  d3d::resource_barrier(ResourceBarrierDesc(context_id->cableVertices.getBuf(), bindlessSRVBarrier));
  d3d::resource_barrier(ResourceBarrierDesc(context_id->cableIndices.getBuf(), bindlessSRVBarrier));
}

const dag::Vector<UniqueBLAS> &get_blases(ContextId context_id, int &meta_alloc_id)
{
  meta_alloc_id = metaAllocId;
  return context_id->cableBLASes;
}

} // namespace bvh::cables
