// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include "bvh_context.h"
#include "bvh_tools.h"
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_lockSbuffer.h>
#include "bvh_generic_connection.h"
#include <3d/dag_lockSbuffer.h>
#include <render/gpuGrass.h>

namespace bvh
{
Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);
}

namespace bvh::gpugrass
{
static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

struct GPUGrassVertexData
{
  Point3 postition;
  E3DCOLOR normal;
  Point2 texcoord;
};

static E3DCOLOR pack_normal(const Point3 &normal) { return e3dcolor(Color3::xyz(normal * 0.5 + 0.5), 0); }

struct BVHConnection : public bvh::BVHConnection
{
  // Store a view to ensure only the current frame's can be used
  ManagedBufView instancesView, instanceCountView;

  BVHConnection(const char *name) : bvh::BVHConnection(name) {}

  float getMaxRange() const override { return contexts.empty() ? 0 : (*contexts.begin())->grassRange; }
  float getGrassFraction() const { return contexts.empty() ? 0 : (*contexts.begin())->grassFraction; }
} bvhConnection("gpu_grass");

static Ptr<ComputeShaderElement> makeRTHWInstances;
static Ptr<ComputeShaderElement> makeIndirectArgs;
static UniqueBufHolder indirectArgs;

static ShaderVariableInfo gpu_grass_bvh_max_instancesVarId = ShaderVariableInfo("gpu_grass_bvh_max_instances", true);
static ShaderVariableInfo gpu_grass_bvh_counterVarId = ShaderVariableInfo("gpu_grass_bvh_counter", true);
static ShaderVariableInfo gpu_grass_bvh_instancesVarId = ShaderVariableInfo("gpu_grass_bvh_instances", true);
static ShaderVariableInfo gpu_grass_bvh_blasVarId = ShaderVariableInfo("gpu_grass_bvh_blas", true);
static ShaderVariableInfo gpu_grass_bvh_blas_horizontalVarId = ShaderVariableInfo("gpu_grass_bvh_blas_horizontal", true);
static ShaderVariableInfo gpu_grass_bvh_meta_index_horizontalVarId = ShaderVariableInfo("gpu_grass_bvh_meta_index_horizontal", true);
static ShaderVariableInfo gpu_grass_bvh_meta_indexVarId = ShaderVariableInfo("gpu_grass_bvh_meta_index", true);
static ShaderVariableInfo gpu_grass_bvh_meta_countVarId = ShaderVariableInfo("gpu_grass_bvh_meta_count", true);
static ShaderVariableInfo gpu_grass_bvh_rangeVarId = ShaderVariableInfo("gpu_grass_bvh_range", true);
static ShaderVariableInfo gpu_grass_bvh_fraction_of_instances_to_keepVarId =
  ShaderVariableInfo("gpu_grass_bvh_fraction_of_instances_to_keep", true);

void init()
{
  bvhConnection.init();
  makeIndirectArgs = new_compute_shader("gpu_grass_bvh_make_indirect_args");
  makeRTHWInstances = new_compute_shader("gpu_grass_bvh_instantiate");
  indirectArgs = dag::buffers::create_ua_indirect(d3d::buffers::Indirect::Dispatch, 1, "gpu_grass_bvh_indirect", RESTAG_BVH);
}

void teardown()
{
  bvhConnection.teardown();
  makeIndirectArgs = {};
  makeRTHWInstances = {};
  indirectArgs.close();
}

static bool make_grass_buffer(ContextId context_id, Context::GPUGrassBillboard &grass, bool is_horizontal)
{
  uint16_t ibData[] = {0, 1, 2, 0, 2, 3};
  constexpr int ibDwords = 3;
  static_assert(ibDwords * sizeof(uint32_t) == sizeof(ibData));
  static_assert(countof(ibData) == Context::GPUGrassBillboard::INDEX_COUNT);
  const char *ib_name = is_horizontal ? "gpu_grass_horizontal_ib" : "gpu_grass_ib";
  grass.indexBuffer = dag::buffers::create_persistent_sr_byte_address(ibDwords, ib_name, d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(grass.indexBuffer, false);
  grass.indexBuffer->updateData(0, sizeof(ibData), ibData, VBLOCK_WRITEONLY);
  context_id->holdBuffer(grass.indexBuffer.getBuf(), grass.indexBufferBindless);

  GPUGrassVertexData vbData[Context::GPUGrassBillboard::VERTEX_COUNT];
  if (is_horizontal)
  {
    E3DCOLOR packedNormal = pack_normal(Point3(0, -1, 0));
    vbData[0] = {Point3(-1, 0, -1), packedNormal, Point2(0, 1)};
    vbData[1] = {Point3(1, 0, -1), packedNormal, Point2(1, 1)};
    vbData[2] = {Point3(1, 0, 1), packedNormal, Point2(1, 0)};
    vbData[3] = {Point3(-1, 0, 1), packedNormal, Point2(0, 0)};
  }
  else
  {
    E3DCOLOR packedNormal = pack_normal(Point3(0, 0, -1));
    vbData[0] = {Point3(-1, 1, 0), packedNormal, Point2(0, 0)};
    vbData[1] = {Point3(1, 1, 0), packedNormal, Point2(1, 0)};
    vbData[2] = {Point3(1, 0, 0), packedNormal, Point2(1, 1)};
    vbData[3] = {Point3(-1, 0, 0), packedNormal, Point2(0, 1)};
  }
  constexpr int vbDrowrds = 24;
  static_assert(vbDrowrds * sizeof(uint32_t) == sizeof(vbData));
  const char *vb_name = is_horizontal ? "gpu_grass_horizontal_vb" : "gpu_grass_vb";
  grass.vertexBuffer = dag::buffers::create_persistent_sr_byte_address(vbDrowrds, vb_name, d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(grass.vertexBuffer, false);
  grass.vertexBuffer->updateData(0, sizeof(vbData), vbData, VBLOCK_WRITEONLY);
  context_id->holdBuffer(grass.vertexBuffer.getBuf(), grass.vertexBufferBindless);

  uint32_t ahsData[Context::GPUGrassBillboard::INDEX_COUNT];
  for (int i = 0; i < Context::GPUGrassBillboard::INDEX_COUNT; i++)
  {
    auto x = float_to_half(vbData[ibData[i]].texcoord.x);
    auto y = float_to_half(vbData[ibData[i]].texcoord.y);
    ahsData[i] = x | y << 16;
  }
  const char *ahs_name = is_horizontal ? "gpu_grass_horizontal_ahs" : "gpu_grass_ahs";
  grass.ahsBuffer = dag::buffers::create_persistent_sr_byte_address(Context::GPUGrassBillboard::INDEX_COUNT, ahs_name,
    d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(grass.ahsBuffer, false);
  grass.ahsBuffer->updateData(0, sizeof(ahsData), ahsData, VBLOCK_WRITEONLY);
  context_id->holdBuffer(grass.ahsBuffer.getBuf(), grass.ahsBufferBindless);

  return true;
}

static bool create_blas(Context::GPUGrassBillboard &grass)
{
  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = grass.vertexBuffer.getBuf();
  desc.data.triangles.vertexCount = grass.VERTEX_COUNT;
  desc.data.triangles.vertexStride = sizeof(GPUGrassVertexData);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.vertexOffset = 0;
  desc.data.triangles.indexBuffer = grass.indexBuffer.getBuf();
  desc.data.triangles.indexCount = grass.INDEX_COUNT;
  desc.data.triangles.indexOffset = 0;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::NONE;

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

  grass.blas = eastl::move(blas);

  return true;
}

void make_meta(ContextId context_id, const GPUGrassBase &grass_base)
{
  auto &names = grass_base.getTextureNames();
  int metaSize = names.diffuse.size();
  G_ASSERT(metaSize == names.alpha.size() && metaSize == names.normal.size());

  auto makeMetaForOne = [&](Context::GPUGrassBillboard &grass) {
    G_ASSERT(grass.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID);
    grass.metaAllocId = context_id->allocateMetaRegion(metaSize);
    grass.metaSize = metaSize;
    return context_id->meshMetaAllocator.get(grass.metaAllocId);
  };
  auto metas = makeMetaForOne(context_id->gpuGrassBillboard);
  auto metasHorizontal = makeMetaForOne(context_id->gpuGrassHorizontal);

  int counter = 0;
  for (auto [diffuse, normal, alpha, meta, metaHorizontal] : zip(names.diffuse, names.normal, names.alpha, metas, metasHorizontal))
  {
    auto loadTexture = [&](const eastl::string &name) {
      auto &tex = context_id->gpuGrassTextures.push_back(dag::get_tex_gameres(name.c_str()));
      return tex.getTexId();
    };
    meta.markInitialized();
    meta.setIndexBit(2);
    meta.materialType |= MeshMeta::bvhMaterialRendinst | MeshMeta::bvhMaterialGrass | MeshMeta::bvhMaterialAlphaInRed;
    meta.texcoordOffset = offsetof(GPUGrassVertexData, texcoord);
    meta.texcoordFormat = VSDT_FLOAT2;
    meta.normalOffset = offsetof(GPUGrassVertexData, normal);
    meta.colorOffset = 0xFF;
    meta.vertexOffset = 0;
    meta.vertexStride = sizeof(GPUGrassVertexData);
    meta.indexCount = Context::GPUGrassBillboard::INDEX_COUNT;
    meta.setIndexBufferIndex(context_id->gpuGrassBillboard.indexBufferBindless);
    meta.setVertexBufferIndex(context_id->gpuGrassBillboard.vertexBufferBindless);
    meta.setAhsVertexBufferIndex(context_id->gpuGrassBillboard.ahsBufferBindless);
    meta.holdAlbedoTex(context_id, loadTexture(diffuse));
    meta.holdNormalTex(context_id, loadTexture(normal));
    meta.holdAlphaTex(context_id, loadTexture(alpha));
    meta.forceNonMetal = true;
    meta.materialData1.x = grass_base.mapTextureIndexToType(counter++);

    metaHorizontal = meta;
    metaHorizontal.setIndexBufferIndex(context_id->gpuGrassHorizontal.indexBufferBindless);
    metaHorizontal.setVertexBufferIndex(context_id->gpuGrassHorizontal.vertexBufferBindless);
    metaHorizontal.setAhsVertexBufferIndex(context_id->gpuGrassHorizontal.ahsBufferBindless);
  }
}

static void init_grass_billboards(ContextId context_id)
{
  if (make_grass_buffer(context_id, context_id->gpuGrassBillboard, false))
  {
    create_blas(context_id->gpuGrassBillboard);
  }
  if (make_grass_buffer(context_id, context_id->gpuGrassHorizontal, true))
  {
    create_blas(context_id->gpuGrassHorizontal);
  }
}

static void close_grass_billboards(ContextId context_id)
{
  auto closeGrass = [context_id](Context::GPUGrassBillboard &grass) {
    context_id->releaseBuffer(grass.indexBuffer.getBuf());
    context_id->releaseBuffer(grass.vertexBuffer.getBuf());
    context_id->releaseBuffer(grass.ahsBuffer.getBuf());
    grass = {};
  };
  closeGrass(context_id->gpuGrassBillboard);
  closeGrass(context_id->gpuGrassHorizontal);
}

void init(ContextId context_id)
{
  if (!context_id->has(Features::GPUGrass))
    return;

  bvhConnection.contexts.insert(context_id);
  init_grass_billboards(context_id);
}

void teardown(ContextId context_id)
{
  if (!context_id->has(Features::GPUGrass))
    return;

  bvhConnection.contexts.erase(context_id);
  close_grass_billboards(context_id);
  ShaderGlobal::set_float(gpu_grass_bvh_rangeVarId, 0);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::GPUGrass))
    return;

  bvhConnection.teardown();
  bvhConnection.instancesView = {};
  bvhConnection.instanceCountView = {};

  if (context_id->gpuGrassBillboard.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
    context_id->freeMetaRegion(context_id->gpuGrassBillboard.metaAllocId);
  if (context_id->gpuGrassHorizontal.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
    context_id->freeMetaRegion(context_id->gpuGrassHorizontal.metaAllocId);

  for (auto &tex : context_id->gpuGrassTextures)
    context_id->releaseTexture(tex.getTexId());
  context_id->gpuGrassTextures.clear();
}

void generate_instances(ContextId context_id, bool has_grass)
{
  if (!context_id->has(Features::GPUGrass))
    return;

  bvhConnection.instancesView = {};
  bvhConnection.instanceCountView = {};
  ShaderGlobal::set_float(gpu_grass_bvh_rangeVarId, 0); // Reset to 0 just in case something below returns

  if (!has_grass)
    return;

  if (bvhConnection.getMaxRange() < 0.01)
    return;

  if (!context_id->gpuGrassBillboard.isValid() || !context_id->gpuGrassHorizontal.isValid() || context_id->gpuGrassTextures.empty())
    return;

  for (auto &tex : context_id->gpuGrassTextures)
    prefetch_managed_texture(tex.getTexId());

  TIME_D3D_PROFILE(GPUGrass_generate_instances)

  G_ASSERT(bvhConnection.isReady());
  bvhConnection.prepare();

  makeIndirectArgs->dispatchThreads(1, 1, 1);

  IPoint4 blasAddress;
  blasAddress.x = context_id->gpuGrassBillboard.blas.getGPUAddress() & 0xFFFFFFFFLLU;
  blasAddress.y = context_id->gpuGrassBillboard.blas.getGPUAddress() >> 32;
  ShaderGlobal::set_int4(gpu_grass_bvh_blasVarId, blasAddress);
  IPoint4 blasAddressHorizontal;
  blasAddressHorizontal.x = context_id->gpuGrassHorizontal.blas.getGPUAddress() & 0xFFFFFFFFLLU;
  blasAddressHorizontal.y = context_id->gpuGrassHorizontal.blas.getGPUAddress() >> 32;
  ShaderGlobal::set_int4(gpu_grass_bvh_blas_horizontalVarId, blasAddressHorizontal);
  int metaIndex = MeshMetaAllocator::decode(context_id->gpuGrassBillboard.metaAllocId);
  ShaderGlobal::set_int(gpu_grass_bvh_meta_indexVarId, metaIndex);
  int metaIndexHorizontal = MeshMetaAllocator::decode(context_id->gpuGrassHorizontal.metaAllocId);
  ShaderGlobal::set_int(gpu_grass_bvh_meta_index_horizontalVarId, metaIndexHorizontal);
  ShaderGlobal::set_int(gpu_grass_bvh_meta_countVarId, context_id->gpuGrassBillboard.metaSize);
  G_ASSERT(context_id->gpuGrassBillboard.metaSize == context_id->gpuGrassHorizontal.metaSize);
  ShaderGlobal::set_int(gpu_grass_bvh_max_instancesVarId, bvhConnection.getInstancesBuffer()->getNumElements());
  ShaderGlobal::set_buffer(gpu_grass_bvh_counterVarId, bvhConnection.getInstanceCounter());
  ShaderGlobal::set_buffer(gpu_grass_bvh_instancesVarId, bvhConnection.getInstancesBuffer());
  ShaderGlobal::set_float(gpu_grass_bvh_rangeVarId, bvhConnection.getMaxRange());
  ShaderGlobal::set_float(gpu_grass_bvh_fraction_of_instances_to_keepVarId, bvhConnection.getGrassFraction());
  makeRTHWInstances->dispatch_indirect(indirectArgs.getBuf(), 0);

  bvhConnection.done();
  bvhConnection.instancesView = bvhConnection.getInstancesBuffer();
  bvhConnection.instanceCountView = bvhConnection.getInstanceCounter();
}

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->has(Features::GPUGrass))
  {
    instances = bvhConnection.instancesView.getBuf();
    instance_count = bvhConnection.instanceCountView.getBuf();
    bvhConnection.instancesView = {};
    bvhConnection.instanceCountView = {};
  }
  else
  {
    instances = nullptr;
    instance_count = nullptr;
  }
}

void get_memory_statistics(ContextId context_id, int &gpuGrassCount, int64_t &gpuGrassMemory, int64_t &gpuGrassTexturesMemory)
{
  gpuGrassCount = 0;
  gpuGrassMemory = 0;
  if (bvhConnection.getInstancesBuffer())
  {
    gpuGrassCount = bvhConnection.getInstancesBuffer()->getNumElements();
    gpuGrassMemory += bvhConnection.getInstancesBuffer()->getSize();
  }
  if (bvhConnection.getInstanceCounter())
    gpuGrassMemory += bvhConnection.getInstanceCounter()->getSize();
  if (context_id->gpuGrassBillboard.isValid())
    gpuGrassMemory += context_id->gpuGrassBillboard.vertexBuffer->getSize() + context_id->gpuGrassBillboard.indexBuffer->getSize() +
                      context_id->gpuGrassBillboard.ahsBuffer->getSize() + context_id->gpuGrassBillboard.blas.getASSize();
  if (context_id->gpuGrassHorizontal.isValid())
    gpuGrassMemory += context_id->gpuGrassHorizontal.vertexBuffer->getSize() + context_id->gpuGrassHorizontal.indexBuffer->getSize() +
                      context_id->gpuGrassHorizontal.ahsBuffer->getSize() + context_id->gpuGrassHorizontal.blas.getASSize();

  gpuGrassTexturesMemory = 0;
  for (auto &tex : context_id->gpuGrassTextures)
    gpuGrassTexturesMemory += tex->getSize();
}

} // namespace bvh::gpugrass