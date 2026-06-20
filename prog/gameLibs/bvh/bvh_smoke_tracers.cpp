// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_eventQueryHolder.h>

#include <shaders/dag_computeShaders.h>
#include "bvh_context.h"
#include "bvh_generic_connection.h"
#include "bvh_particles.h"

#include <render/smokeTracers.hlsli>

namespace bvh
{

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

namespace smoke_tracers
{

struct BillboardVertex
{
  Point3 position;
  Point2 texcoord;
};

using BillboardIndex = uint16_t;

struct BillboardGeometry
{
  UniqueBLAS blas;
  UniqueBuf vb;
  UniqueBuf ib;
  uint32_t bindlessIndicesIndex = 0;
  uint32_t bindlessVerticesIndex = 0;
  Ptr<ComputeShaderElement> generateCs;
};

static BillboardGeometry billboards;

namespace var
{
static ShaderVariableInfo smoke_tracer_bvh_meta_id("smoke_tracer_bvh_meta_id", true);
static ShaderVariableInfo smoke_tracer_bvh_blas_address("smoke_tracer_bvh_blas_address", true);
static ShaderVariableInfo smoke_tracer_bvh_max_count("smoke_tracer_bvh_max_count", true);
static ShaderVariableInfo smoke_tracer_instance_offset("smoke_tracer_instance_offset", true);
static ShaderVariableInfo smoke_tracer_bvh_counter("smoke_tracer_bvh_counter", true);
static ShaderVariableInfo smoke_tracer_bvh_instances("smoke_tracer_bvh_instances", true);
static ShaderVariableInfo smoke_tracer_tracer_buf("smoke_tracer_tracer_buf", true);
static ShaderVariableInfo smoke_tracer_dynamic_buf("smoke_tracer_dynamic_buf", true);
static ShaderVariableInfo smoke_tracer_verts_buf("smoke_tracer_verts_buf", true);
} // namespace var

struct BVHConnection : public bvh::BVHConnection
{
  Sbuffer *instancesSnap = nullptr;
  Sbuffer *counterSnap = nullptr;

  int lastInstanceCount = 0;

  BVHConnection(const char *name) : bvh::BVHConnection(name) { instanceBufferAlign = particles::TRACER_CAPACITY_ALIGN; }

  void onInstanceCountReadback(int instance_count) override { lastInstanceCount = instance_count; }

  bool prepare() override
  {
    if (!bvh::BVHConnection::prepare())
      return false;

    return true;
  }

  void teardown() override
  {
    bvh::BVHConnection::teardown();
    instancesSnap = nullptr;
    counterSnap = nullptr;
    lastInstanceCount = 0;
  }

  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;

} smokeTracersBvhConnection("smoke_tracers");

void init()
{
  smokeTracersBvhConnection.init();
  billboards.generateCs = new_compute_shader("bvh_smoke_tracers_cs");

  // Two crossed planes (XZ and YZ) along the trail Z direction.
  const BillboardVertex vertices[] = {
    {Point3(-1, 0, -1), Point2(0, 1)},
    {Point3(1, 0, -1), Point2(1, 1)},
    {Point3(1, 0, 1), Point2(1, 0)},
    {Point3(-1, 0, 1), Point2(0, 0)},

    {Point3(0, -1, -1), Point2(0, 1)},
    {Point3(0, 1, -1), Point2(1, 1)},
    {Point3(0, 1, 1), Point2(1, 0)},
    {Point3(0, -1, 1), Point2(0, 0)},
  };

  const BillboardIndex indices[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};

  billboards.ib = dag::buffers::create_persistent_sr_byte_address(divide_up(sizeof(BillboardIndex) * countof(indices), 4),
    "bvh_smoke_tracer_ib", d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(billboards.ib, );
  billboards.vb = dag::buffers::create_persistent_sr_byte_address(divide_up(sizeof(BillboardVertex) * countof(vertices), 4),
    "bvh_smoke_tracer_vb", d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(billboards.vb, );

  billboards.ib->updateData(0, sizeof(indices), indices, 0);
  billboards.vb->updateData(0, sizeof(vertices), vertices, 0);

  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = billboards.vb.getBuf();
  desc.data.triangles.indexBuffer = billboards.ib.getBuf();
  desc.data.triangles.vertexCount = countof(vertices);
  desc.data.triangles.vertexStride = sizeof(BillboardVertex);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.indexCount = countof(indices);
  desc.data.triangles.indexOffset = 0;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::NONE;

  const RaytraceBuildFlags buildFlags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

  billboards.blas = UniqueBLAS::create(&desc, 1, buildFlags);
  HANDLE_LOST_DEVICE_STATE(billboards.blas, );

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.geometryDesc = &desc;
  buildInfo.geometryDescCount = 1;
  buildInfo.flags = buildFlags;
  buildInfo.scratchSpaceBuffer =
    alloc_scratch_buffer(billboards.blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = billboards.blas.getBuildScratchSize();
  HANDLE_LOST_DEVICE_STATE(buildInfo.scratchSpaceBuffer, );

  d3d::build_bottom_acceleration_structure(billboards.blas.get(), buildInfo);
}

void teardown()
{
  billboards.blas.reset();
  billboards.vb.close();
  billboards.ib.close();
  billboards.generateCs = {};

  smokeTracersBvhConnection.teardown();

  ShaderGlobal::set_buffer(var::smoke_tracer_bvh_counter, BAD_D3DRESID);
  ShaderGlobal::set_buffer(var::smoke_tracer_bvh_instances, BAD_D3DRESID);
  ShaderGlobal::set_buffer(var::smoke_tracer_tracer_buf, BAD_D3DRESID);
  ShaderGlobal::set_buffer(var::smoke_tracer_dynamic_buf, BAD_D3DRESID);
  ShaderGlobal::set_buffer(var::smoke_tracer_verts_buf, BAD_D3DRESID);
}

void init(ContextId context_id)
{
  if (!context_id->has(Features::SmokeTracers))
    return;

  G_ASSERT(smokeTracersBvhConnection.contexts.empty());

  smokeTracersBvhConnection.contexts.insert(context_id);

  context_id->holdBuffer(billboards.ib.getBuf(), billboards.bindlessIndicesIndex);
  context_id->holdBuffer(billboards.vb.getBuf(), billboards.bindlessVerticesIndex);

  d3d::resource_barrier(ResourceBarrierDesc(billboards.ib.getBuf(), bindlessSRVBarrier));
  d3d::resource_barrier(ResourceBarrierDesc(billboards.vb.getBuf(), bindlessSRVBarrier));

  smokeTracersBvhConnection.metaAllocId = context_id->allocateMetaRegion(1, "smokeTracer");
  LockedMetaAccess lockedMeta(*context_id, smokeTracersBvhConnection.metaAllocId);
  auto &meta = lockedMeta[0];

  meta.markInitialized();
  meta.albedoTextureIndex = MeshMeta::INVALID_TEXTURE;
  meta.materialType = MeshMeta::bvhMaterialSmokeTracer;
  meta.setIndexBitAndTexcoordFormat(2, VSDT_FLOAT2);
  meta.texcoordOffset = offsetof(BillboardVertex, texcoord);
  meta.normalOffset = 0xFFU;
  meta.colorOffset = 0xFFU;
  meta.vertexStride = sizeof(BillboardVertex);
  meta.setIndexBufferIndex(billboards.bindlessIndicesIndex);
  meta.setVertexBufferIndex(billboards.bindlessVerticesIndex);
  meta.startIndex = 0;
  meta.startVertex = 0;
}

void teardown(ContextId context_id)
{
  if (!context_id->has(Features::SmokeTracers))
    return;

  smokeTracersBvhConnection.contexts.erase(context_id);

  context_id->releaseBuffer(billboards.ib.getBuf());
  context_id->releaseBuffer(billboards.vb.getBuf());

  if (smokeTracersBvhConnection.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
  {
    context_id->freeMetaRegion(smokeTracersBvhConnection.metaAllocId);
    smokeTracersBvhConnection.metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  }

  smokeTracersBvhConnection.instancesSnap = nullptr;
  smokeTracersBvhConnection.counterSnap = nullptr;
}

void connect(smoke_tracers_connect_callback callback) { callback(&smokeTracersBvhConnection); }

void set_source_buffers(Sbuffer *tracer_buf, Sbuffer *dynamic_buf, Sbuffer *verts_buf)
{
  ShaderGlobal::set_buffer(var::smoke_tracer_tracer_buf, tracer_buf);
  ShaderGlobal::set_buffer(var::smoke_tracer_dynamic_buf, dynamic_buf);
  ShaderGlobal::set_buffer(var::smoke_tracer_verts_buf, verts_buf);
}

void update_instances()
{
  if (!smokeTracersBvhConnection.isReady() || !billboards.generateCs)
    return;
  if (!smokeTracersBvhConnection.getInstancesBuffer())
    return;
  if (!particles::is_ready())
    return;

  TIME_D3D_PROFILE(bvh_smoke_tracer);

  // Source SRBs and dynamic-buf RO_SRV barrier come from SmokeTracerManager::beforeRender.
  auto blasAddress = billboards.blas.getGPUAddress();
  ShaderGlobal::set_int4(var::smoke_tracer_bvh_blas_address, blasAddress & GPU_ADDRESS_LOW_MASK, blasAddress >> GPU_ADDRESS_HIGH_SHIFT,
    0, 0);
  const int tracerInstancesCap = (int)smokeTracersBvhConnection.getInstancesBuffer()->getNumElements();
  ShaderGlobal::set_int(var::smoke_tracer_bvh_max_count, min(tracerInstancesCap, particles::get_smoke_tracer_capacity()));
  ShaderGlobal::set_int(var::smoke_tracer_bvh_meta_id, MeshMetaAllocator::decode(smokeTracersBvhConnection.metaAllocId));
  // Re-set in case bvh::particles::grow() did not run this frame.
  ShaderGlobal::set_int(var::smoke_tracer_instance_offset, particles::get_fx_capacity());

  ShaderGlobal::set_buffer(var::smoke_tracer_bvh_counter, smokeTracersBvhConnection.getInstanceCounter());
  ShaderGlobal::set_buffer(var::smoke_tracer_bvh_instances, smokeTracersBvhConnection.getInstancesBuffer());

  billboards.generateCs->dispatchThreads(MAX_TRACERS, 1, 1);

  // CS UAV -> closesthit SRV; mirrors the VS-side barrier in bvh::fx::BVHConnection::done.
  if (Sbuffer *particleData = particles::get_data_buf())
    d3d::resource_barrier({particleData, RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  smokeTracersBvhConnection.instancesSnap = smokeTracersBvhConnection.getInstancesBuffer().getBuf();
  smokeTracersBvhConnection.counterSnap = smokeTracersBvhConnection.getInstanceCounter().getBuf();
}

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->has(Features::SmokeTracers))
  {
    instances = smokeTracersBvhConnection.instancesSnap;
    instance_count = smokeTracersBvhConnection.counterSnap;
    smokeTracersBvhConnection.instancesSnap = nullptr;
    smokeTracersBvhConnection.counterSnap = nullptr;
  }
  else
  {
    instances = nullptr;
    instance_count = nullptr;
  }
}

void get_memory_statistics(int &count, int64_t &vb, int64_t &blas)
{
  count = smokeTracersBvhConnection.lastInstanceCount;
  // bvh_particle_data is shared with bvh::fx and tracked via bvh::particles::get_size_bytes().
  vb = (billboards.vb ? billboards.vb->getSize() : 0) + (billboards.ib ? billboards.ib->getSize() : 0) + particles::get_size_bytes() +
       (smokeTracersBvhConnection.instances ? smokeTracersBvhConnection.instances->getSize() : 0) +
       (smokeTracersBvhConnection.counter ? smokeTracersBvhConnection.counter->getSize() : 0) +
       (smokeTracersBvhConnection.counterReadback ? smokeTracersBvhConnection.counterReadback->getSize() : 0);
  blas = billboards.blas ? d3d::get_raytrace_acceleration_structure_size(billboards.blas.get()) : 0;
}

} // namespace smoke_tracers

} // namespace bvh
