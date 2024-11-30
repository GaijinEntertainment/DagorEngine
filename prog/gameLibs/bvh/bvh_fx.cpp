// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_bindless.h>
#include <render/randomGrass.h>
#include <render/randomGrassInstance.hlsli>
#include <shaders/dag_shaderMesh.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_eventQueryHolder.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_generic_connection.h"

#include "../commonFx/commonFxGame/modfx/modfx_bvh.hlsli"

namespace bvh
{

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

namespace fx
{

struct BillboardVertex
{
  Point3 position;
  Point2 texcoord;
};

using BillboardIndex = uint16_t;

static UniqueBLAS billboard_blas;
static UniqueBLAS billboard_shadow_blas;
static UniqueBuf billboard_vb;
static UniqueBuf billboard_ib;

static uint32_t bindless_indices_index;
static uint32_t bindless_vertices_index;

void create_billboard_blases()
{
  const BillboardVertex vertices[] = {
    {Point3(-1, -1, 0), Point2(0, 1)},
    {Point3(1, -1, 0), Point2(1, 1)},
    {Point3(1, 1, 0), Point2(1, 0)},
    {Point3(-1, 1, 0), Point2(0, 0)},

    {Point3(0, -1, -1), Point2(0, 1)},
    {Point3(0, 1, -1), Point2(1, 1)},
    {Point3(0, 1, 1), Point2(1, 0)},
    {Point3(0, -1, 1), Point2(0, 0)},
  };

  const BillboardIndex indices[] = {0, 1, 2, 0, 2, 3, /**/ 4, 5, 6, 5, 6, 7};

  billboard_ib =
    dag::buffers::create_persistent_sr_byte_address(divide_up(sizeof(BillboardIndex) * countof(indices), 4), "bvh_billboard_ib");
  HANDLE_LOST_DEVICE_STATE(billboard_ib, );
  billboard_vb =
    dag::buffers::create_persistent_sr_byte_address(divide_up(sizeof(BillboardVertex) * countof(vertices), 4), "bvh_billboard_vb");
  HANDLE_LOST_DEVICE_STATE(billboard_vb, );

  billboard_ib->updateData(0, sizeof(indices), indices, 0);
  billboard_vb->updateData(0, sizeof(vertices), vertices, 0);

  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = billboard_vb.getBuf();
  desc.data.triangles.indexBuffer = billboard_ib.getBuf();
  desc.data.triangles.vertexCount = countof(vertices);
  desc.data.triangles.vertexStride = sizeof(BillboardVertex);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.indexCount = countof(indices);
  desc.data.triangles.indexOffset = 0;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::NONE;

  RaytraceBuildFlags buildFlags = RaytraceBuildFlags::FAST_TRACE;

  billboard_blas = UniqueBLAS::create(&desc, 1, buildFlags);
  HANDLE_LOST_DEVICE_STATE(billboard_blas, );

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.geometryDesc = &desc;
  buildInfo.geometryDescCount = 1;
  buildInfo.flags = buildFlags;
  buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(billboard_blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = billboard_blas.getBuildScratchSize();
  HANDLE_LOST_DEVICE_STATE(buildInfo.scratchSpaceBuffer, );

  d3d::build_bottom_acceleration_structure(billboard_blas.get(), buildInfo);

  desc.data.triangles.vertexCount /= 2;
  desc.data.triangles.indexCount /= 2;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::NONE;
  billboard_shadow_blas = UniqueBLAS::create(&desc, 1, buildFlags);
  HANDLE_LOST_DEVICE_STATE(billboard_shadow_blas, );

  buildInfo.scratchSpaceBuffer =
    alloc_scratch_buffer(billboard_shadow_blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = billboard_shadow_blas.getBuildScratchSize();
  HANDLE_LOST_DEVICE_STATE(buildInfo.scratchSpaceBuffer, );

  d3d::build_bottom_acceleration_structure(billboard_shadow_blas.get(), buildInfo);
}

static int get_particle_meta(ContextId context_id, TEXTUREID texture_id)
{
  auto iter = context_id->particleMeta.find((uint32_t)texture_id);
  if (iter != context_id->particleMeta.end())
    return iter->second.metaAllocId;

  int metaAllocId = context_id->allocateMetaRegion();
  auto &meta = context_id->meshMetaAllocator.get(metaAllocId);

  meta.markInitialized();

  context_id->holdTexture(texture_id, meta.albedoTextureAndSamplerIndex);

  meta.materialType = MeshMeta::bvhMaterialParticle;
  meta.setIndexBitAndTexcoordFormat(2, VSDT_FLOAT2);
  meta.texcoordNormalColorOffsetAndVertexStride =
    (offsetof(BillboardVertex, texcoord) << 24) | 0xFF0000U | 0xFF00U | sizeof(BillboardVertex);
  meta.indexAndVertexBufferIndex = (bindless_indices_index << 16) | bindless_vertices_index;
  meta.startIndex = 0;
  meta.startVertex = 0;

  context_id->particleMeta.insert({uint32_t(texture_id), {texture_id, metaAllocId}});

  return metaAllocId;
}

struct BVHConnection : public bvh::BVHConnection
{
  BVHConnection(const char *name) : bvh::BVHConnection(name) {}

  UniqueBuf particleData;

  void textureUsed(TEXTUREID texture_id) override
  {
    static int reg_no = ShaderGlobal::get_slot_by_name("dafx_modfx_bvh_meta_id_regno");

    int metaAllocId = get_particle_meta(*contexts.begin(), texture_id);
    int reg[] = {metaAllocId, 0, 0, 0};
    d3d::set_const(STAGE_VS, reg_no, reg, 1);
  }

  bool prepare() override
  {
    if (!bvh::BVHConnection::prepare())
      return false;

    static int dafx_modfx_bvh_max_countVarId = get_shader_variable_id("dafx_modfx_bvh_max_count");
    static int dafx_modfx_bvh_blas_addressVarId = get_shader_variable_id("dafx_modfx_bvh_blas_address");
    static int bvh_particle_dataVarId = get_shader_variable_id("bvh_particle_data");

    static int dafx_modfx_bvh_instance_buffer_regno =
      ShaderGlobal::get_int(get_shader_variable_id("dafx_modfx_bvh_instance_buffer_regno"));
    static int dafx_modfx_bvh_instance_count_regno =
      ShaderGlobal::get_int(get_shader_variable_id("dafx_modfx_bvh_instance_count_regno"));
    static int dafx_modfx_bvh_instance_data_regno =
      ShaderGlobal::get_int(get_shader_variable_id("dafx_modfx_bvh_instance_data_regno"));

    if (particleData && particleData->getNumElements() < maxCount)
      particleData.close();

    if (!particleData && maxCount)
    {
      particleData = dag::buffers::create_ua_sr_structured(sizeof(ModfxBVHParticleData), maxCount, "bvh_fx_particle_data");
      HANDLE_LOST_DEVICE_STATE(particleData, false);
      ShaderGlobal::set_buffer(bvh_particle_dataVarId, particleData);
    }

    d3d::set_rwbuffer(STAGE_VS, dafx_modfx_bvh_instance_buffer_regno, getInstancesBuffer().getBuf());
    d3d::set_rwbuffer(STAGE_VS, dafx_modfx_bvh_instance_count_regno, getInstanceCounter().getBuf());
    d3d::set_rwbuffer(STAGE_VS, dafx_modfx_bvh_instance_data_regno, particleData.getBuf());

    auto blasAddress = billboard_blas.getGPUAddress();
    auto shadowBlasAddress = billboard_shadow_blas.getGPUAddress();

    ShaderGlobal::set_int(dafx_modfx_bvh_max_countVarId, getInstancesBuffer() ? getInstancesBuffer()->getNumElements() : 0);
    ShaderGlobal::set_int4(dafx_modfx_bvh_blas_addressVarId, blasAddress & 0xFFFFFFFFLLU, blasAddress >> 32,
      shadowBlasAddress & 0xFFFFFFFFLLU, shadowBlasAddress >> 32);

    return true;
  }

  void teardown() override
  {
    bvh::BVHConnection::teardown();
    particleData.close();
  }

} bvhConnection("fx");

void on_unload_scene(ContextId context_id)
{
  for (auto &meta : context_id->particleMeta)
  {
    context_id->releaseTexure(meta.second.textureId);
    context_id->freeMetaRegion(meta.second.metaAllocId);
  }
  context_id->particleMeta.clear();
}

void init()
{
  bvhConnection.init();
  create_billboard_blases();
}

void teardown()
{
  billboard_blas.reset();
  billboard_shadow_blas.reset();
  billboard_vb.close();
  billboard_ib.close();

  bvhConnection.teardown();
}

void init(ContextId context_id)
{
  if (!context_id->has(Features::Fx))
    return;

  G_ASSERT(bvhConnection.contexts.empty());

  bvhConnection.contexts.insert(context_id);

  context_id->holdBuffer(billboard_ib.getBuf(), bindless_indices_index);
  context_id->holdBuffer(billboard_vb.getBuf(), bindless_vertices_index);

  d3d::resource_barrier(ResourceBarrierDesc(billboard_ib.getBuf(), bindlessSRVBarrier));
  d3d::resource_barrier(ResourceBarrierDesc(billboard_vb.getBuf(), bindlessSRVBarrier));
}

void teardown(ContextId context_id)
{
  if (!context_id->has(Features::Fx))
    return;

  bvhConnection.contexts.erase(context_id);

  context_id->releaseBuffer(billboard_ib.getBuf());
  context_id->releaseBuffer(billboard_vb.getBuf());
  bvh::fx::on_unload_scene(context_id);

  bvhConnection.contexts.erase(context_id);
}

void connect(fx_connect_callback callback) { callback(&bvhConnection); }

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->has(Features::Fx))
  {
    instances = bvhConnection.instances.getBuf();
    instance_count = bvhConnection.counter.getBuf();
  }
  else
  {
    instances = nullptr;
    instance_count = nullptr;
  }
}

void get_memory_statistics(int &meta, int &queries)
{
  meta = queries = 0;
  if (bvhConnection.instances)
    queries = bvhConnection.instances->getElementSize() * bvhConnection.instances->getNumElements();
  if (bvhConnection.metainfoMappings)
    meta = bvhConnection.metainfoMappings->getElementSize() * bvhConnection.metainfoMappings->getNumElements();
}

} // namespace fx

} // namespace bvh