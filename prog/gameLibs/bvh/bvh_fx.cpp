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
#include "bvh_particles.h"

namespace bvh
{

namespace fx
{

static UniqueBLAS billboard_blas;
static UniqueBLAS billboard_shadow_blas;
static UniqueBuf billboard_vb;
static UniqueBuf billboard_ib;

static uint32_t bindless_indices_index;
static uint32_t bindless_vertices_index;

static float min_screen_height = 0;

void create_billboard_blases()
{
  const particles::BillboardVertex vertices[] = {
    {Point3(-1, -1, 0), Point2(0, 1)},
    {Point3(1, -1, 0), Point2(1, 1)},
    {Point3(1, 1, 0), Point2(1, 0)},
    {Point3(-1, 1, 0), Point2(0, 0)},

    {Point3(0, -1, -1), Point2(0, 1)},
    {Point3(0, 1, -1), Point2(1, 1)},
    {Point3(0, 1, 1), Point2(1, 0)},
    {Point3(0, -1, 1), Point2(0, 0)},
  };

  billboard_blas = particles::create_billboard_blas(vertices, billboard_vb, billboard_ib, "bvh_billboard_vb", "bvh_billboard_ib");
  CHECK_LOST_DEVICE_STATE();
  billboard_shadow_blas = particles::build_billboard_blas(billboard_vb.getBuf(), billboard_ib.getBuf(), 1);
}

static MeshMetaAllocator::AllocId get_particle_meta(ContextId context_id, TEXTUREID texture_id)
{
  auto iter = context_id->particleMeta.find((uint32_t)texture_id);
  if (iter != context_id->particleMeta.end())
    return iter->second.metaAllocId;

  TIME_PROFILE(meta_lock_particle);
  auto metaAllocId = context_id->allocateMetaRegion(1, "particle");
  LockedMetaAccess lockedMeta(*context_id, metaAllocId);
  auto &meta = lockedMeta[0];
  particles::fill_billboard_meta(meta, MeshMeta::bvhMaterialParticle, bindless_indices_index, bindless_vertices_index);
  meta.holdAlbedoTex(context_id, texture_id);

  context_id->particleMeta.insert({uint32_t(texture_id), {texture_id, metaAllocId}});

  return metaAllocId;
}

struct BVHConnection : public bvh::BVHConnection
{
  BVHConnection(const char *name) : bvh::BVHConnection(name) { instanceBufferAlign = particles::FX_CAPACITY_ALIGN; }

  void textureUsed(TEXTUREID texture_id) override
  {
    static int reg_no = ShaderGlobal::get_slot_by_name("dafx_modfx_bvh_meta_id_regno");

    auto metaAllocId = get_particle_meta(*contexts.begin(), texture_id);
    int reg[] = {MeshMetaAllocator::decode(metaAllocId), 0, 0, 0};
    d3d::set_const(STAGE_VS, reg_no, reg, 1);
  }

  bool prepare() override
  {
    if (!bvh::BVHConnection::prepare())
      return false;
    if (!particles::is_ready())
      return false;

    static int dafx_modfx_bvh_max_countVarId = get_shader_variable_id("dafx_modfx_bvh_max_count");
    static int dafx_modfx_bvh_blas_addressVarId = get_shader_variable_id("dafx_modfx_bvh_blas_address");
    static int dafx_modfx_bvh_min_screen_heightVarId = get_shader_variable_id("dafx_modfx_bvh_min_screen_height");

    static int dafx_modfx_bvh_instance_buffer_regno =
      ShaderGlobal::get_int(get_shader_variable_id("dafx_modfx_bvh_instance_buffer_regno"));
    static int dafx_modfx_bvh_instance_count_regno =
      ShaderGlobal::get_int(get_shader_variable_id("dafx_modfx_bvh_instance_count_regno"));
    static int dafx_modfx_bvh_instance_data_regno =
      ShaderGlobal::get_int(get_shader_variable_id("dafx_modfx_bvh_instance_data_regno"));

    d3d::set_rwbuffer(STAGE_VS, dafx_modfx_bvh_instance_buffer_regno, getInstancesBuffer().getBuf());
    d3d::set_rwbuffer(STAGE_VS, dafx_modfx_bvh_instance_count_regno, getInstanceCounter().getBuf());
    d3d::set_rwbuffer(STAGE_VS, dafx_modfx_bvh_instance_data_regno, particles::get_data_buf());

    auto blasAddress = billboard_blas.getGPUAddress();
    auto shadowBlasAddress = billboard_shadow_blas.getGPUAddress();

    const int fxInstancesCap = getInstancesBuffer() ? (int)getInstancesBuffer()->getNumElements() : 0;
    ShaderGlobal::set_int(dafx_modfx_bvh_max_countVarId, min(fxInstancesCap, particles::get_fx_capacity()));
    ShaderGlobal::set_int4(dafx_modfx_bvh_blas_addressVarId, blasAddress & GPU_ADDRESS_LOW_MASK, blasAddress >> GPU_ADDRESS_HIGH_SHIFT,
      shadowBlasAddress & GPU_ADDRESS_LOW_MASK, shadowBlasAddress >> GPU_ADDRESS_HIGH_SHIFT);

    ShaderGlobal::set_float(dafx_modfx_bvh_min_screen_heightVarId, min_screen_height);

    return true;
  }

  void done() override
  {
    bvh::BVHConnection::done();

    if (Sbuffer *particleData = particles::get_data_buf())
      d3d::resource_barrier({particleData, RB_STAGE_COMPUTE | RB_SOURCE_STAGE_VERTEX});
  }

  void teardown() override { bvh::BVHConnection::teardown(); }

} bvhConnection("fx");

void on_unload_scene(ContextId context_id)
{
  for (auto &meta : context_id->particleMeta)
  {
    context_id->releaseTexture(meta.second.textureId);
    context_id->freeMetaRegion(meta.second.metaAllocId);
  }
  context_id->particleMeta.clear();
}

void init(const AdditionalSettings &settings)
{
  min_screen_height = settings.fxMinScreenHeight;
  bvhConnection.init();
  create_billboard_blases();
}

void collect_blas_addresses(dag::Vector<uint64_t> &addresses)
{
  if (billboard_blas)
    addresses.push_back(billboard_blas.getGPUAddress());
  if (billboard_shadow_blas)
    addresses.push_back(billboard_shadow_blas.getGPUAddress());
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
  if (!context_id->hasAny(Features::Fx))
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
  if (!context_id->hasAny(Features::Fx))
    return;

  bvhConnection.contexts.erase(context_id);

  context_id->releaseBuffer(billboard_ib.getBuf());
  context_id->releaseBuffer(billboard_vb.getBuf());
  bvh::fx::on_unload_scene(context_id);
}

void connect(fx_connect_callback callback) { callback(&bvhConnection); }

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->hasAny(Features::Fx))
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