// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include "bvh_context.h"
#include "bvh_omm.h"
#include "bvh_tools.h"
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_enumerate.h>
#include "bvh_generic_connection.h"
#include <render/gpuGrass.h>
#include <render/grassInstance.hlsli>
#include <render/omm.h>

namespace bvh::gpugrass
{
static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

struct GPUGrassVertexData
{
  Point3 postition;
  E3DCOLOR normal;
  Point2 texcoord;
};

struct BVHConnection : public bvh::BVHConnection
{
  // Store a view to ensure only the current frame's can be used
  ManagedBufView instancesView, instanceCountView;

  dag::Vector<GpuGrassBvhMapping> metainfoMappingsCpu;
  // With no address the instancer can make no instance, thus skip the dispatch while the first bakes run.
  bool anyBlasPublished = false;

  BVHConnection(const char *name) : bvh::BVHConnection(name) {}

  float getMaxRange() const override { return contexts.empty() ? 0 : (*contexts.begin())->grassRange; }
  float getGrassFraction() const { return contexts.empty() ? 0 : (*contexts.begin())->grassFraction; }

  void teardown() override
  {
    bvh::BVHConnection::teardown();
    metainfoMappingsCpu.clear();
    anyBlasPublished = false;
  }
} bvhConnection("gpu_grass");

static Ptr<ComputeShaderElement> makeRTHWInstances;
static Ptr<ComputeShaderElement> makeIndirectArgs;
static UniqueBufWithShaderVar indirectArgs;

static ShaderVariableInfo gpu_grass_bvh_max_instancesVarId = ShaderVariableInfo("gpu_grass_bvh_max_instances", true);
static ShaderVariableInfo gpu_grass_bvh_counterVarId = ShaderVariableInfo("gpu_grass_bvh_counter", true);
static ShaderVariableInfo gpu_grass_bvh_instancesVarId = ShaderVariableInfo("gpu_grass_bvh_instances", true);
static ShaderVariableInfo gpu_grass_bvh_mappingVarId = ShaderVariableInfo("gpu_grass_bvh_mapping", true);
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
  static_assert(countof(quad_indices) == Context::GPUGrassBillboard::INDEX_COUNT);
  if (!create_quad_ib(grass.indexBuffer, is_horizontal ? "gpu_grass_horizontal_ib" : "gpu_grass_ib"))
    return false;
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

  if (!context_id->ommEnabled)
  {
    uint32_t ahsData[Context::GPUGrassBillboard::INDEX_COUNT];
    for (int i = 0; i < Context::GPUGrassBillboard::INDEX_COUNT; i++)
    {
      auto x = float_to_half(vbData[quad_indices[i]].texcoord.x);
      auto y = float_to_half(vbData[quad_indices[i]].texcoord.y);
      ahsData[i] = x | y << 16;
    }

    const char *ahs_name = is_horizontal ? "gpu_grass_horizontal_ahs" : "gpu_grass_ahs";
    grass.ahsBuffer = dag::buffers::create_persistent_sr_byte_address(Context::GPUGrassBillboard::INDEX_COUNT, ahs_name,
      d3d::buffers::Init::No, RESTAG_BVH);
    HANDLE_LOST_DEVICE_STATE(grass.ahsBuffer, false);
    grass.ahsBuffer->updateData(0, sizeof(ahsData), ahsData, VBLOCK_WRITEONLY);
    context_id->holdBuffer(grass.ahsBuffer.getBuf(), grass.ahsBufferBindless);
  }

  return true;
}

static bool create_blas(const Context::GPUGrassBillboard &grass, UniqueBLAS &out_blas,
  const RaytraceGeometryDescription::OpacityMicroMapLinkage *omm_linkage = nullptr)
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
  if (omm_linkage)
  {
    desc.ommLinkage = *omm_linkage;
    desc.extraDataAvailableMask.hasOpacityMicroMapLinkage = true;
  }

  out_blas = build_blas(desc, blas_flags);
  return bool(out_blas);
}

static void upload_mappings()
{
  if (bvhConnection.metainfoMappings && !bvhConnection.metainfoMappingsCpu.empty())
    bvhConnection.metainfoMappings->updateData(0, data_size(bvhConnection.metainfoMappingsCpu),
      bvhConnection.metainfoMappingsCpu.data(), 0);
}

// All meta indices are known here; with OMM the BLAS addresses arrive later, from process_omm().
static void make_mappings(ContextId context_id, int texture_count)
{
  if (texture_count == 0)
    return;

  bvhConnection.metainfoMappings = dag::buffers::create_persistent_sr_structured(sizeof(GpuGrassBvhMapping), texture_count * 2,
    "bvh_gpu_grass_mapping", d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(bvhConnection.metainfoMappings, );

  bvhConnection.metainfoMappingsCpu.resize(texture_count * 2);
  memset(bvhConnection.metainfoMappingsCpu.data(), 0, data_size(bvhConnection.metainfoMappingsCpu));
  bvhConnection.anyBlasPublished = context_id->gpuGrassBillboard.blas || context_id->gpuGrassHorizontal.blas;

  auto fillForOne = [texture_count](const Context::GPUGrassBillboard &grass) {
    const int metaIndex = MeshMetaAllocator::decode(grass.metaAllocId);
    for (int textureIndex = 0; textureIndex < texture_count; ++textureIndex)
    {
      auto &mapping = bvhConnection.metainfoMappingsCpu[grass.mappingBase + textureIndex];
      mapping.metaIndex = metaIndex + textureIndex;
      if (grass.blas)
        split_gpu_address(mapping.blas, grass.blas.getGPUAddress());
    }
  };
  fillForOne(context_id->gpuGrassBillboard);
  fillForOne(context_id->gpuGrassHorizontal);

  upload_mappings();
}

void make_meta(ContextId context_id, const GPUGrassBase &grass_base)
{
  auto &names = grass_base.getTextureNames();
  int metaSize = names.diffuse.size();
  G_ASSERT(metaSize == names.alpha.size() && metaSize == names.normal.size());

  auto allocateForOne = [&](Context::GPUGrassBillboard &grass, int mapping_base) {
    G_ASSERT(grass.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID);
    grass.metaAllocId = context_id->allocateMetaRegion(metaSize, "gpuGrass");
    grass.metaSize = metaSize;
    grass.mappingBase = mapping_base;
    grass.textureSlots.resize(metaSize);
  };
  allocateForOne(context_id->gpuGrassBillboard, 0);
  allocateForOne(context_id->gpuGrassHorizontal, metaSize);

  {
    TIME_PROFILE(meta_lock_gpu_grass);
    OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock);
    auto metas = context_id->meshMetaAllocator.get(context_id->gpuGrassBillboard.metaAllocId);
    auto metasHorizontal = context_id->meshMetaAllocator.get(context_id->gpuGrassHorizontal.metaAllocId);

    int counter = 0;
    for (auto [diffuse, normal, alpha, meta, metaHorizontal] : zip(names.diffuse, names.normal, names.alpha, metas, metasHorizontal))
    {
      auto loadTexture = [&](const eastl::string &name) {
        auto &tex = context_id->gpuGrassTextures.push_back(dag::get_tex_gameres(name.c_str()));
        return tex.getTexId();
      };
      const int textureIndex = counter++;
      const TEXTUREID diffuseTexId = loadTexture(diffuse);
      const TEXTUREID normalTexId = loadTexture(normal);
      const TEXTUREID alphaTexId = loadTexture(alpha);

      meta.markInitialized();
      meta.setIndexBit(2);
      meta.materialType |= MeshMeta::bvhMaterialRendinst | MeshMeta::bvhMaterialGrass | MeshMeta::bvhMaterialAlphaInRed;
      meta.texcoordOffset = offsetof(GPUGrassVertexData, texcoord);
      meta.setTexcoordFormat(VSDT_FLOAT2);
      meta.normalOffset = offsetof(GPUGrassVertexData, normal);
      meta.colorOffset = 0xFF;
      meta.vertexOffset = 0;
      meta.vertexStride = sizeof(GPUGrassVertexData);
      meta.indexCount = Context::GPUGrassBillboard::INDEX_COUNT;
      meta.setIndexBufferIndex(context_id->gpuGrassBillboard.indexBufferBindless);
      meta.setVertexBufferIndex(context_id->gpuGrassBillboard.vertexBufferBindless);
      meta.setAhsVertexBufferIndex(context_id->gpuGrassBillboard.ahsBufferBindless);
      meta.holdAlbedoTex(context_id, diffuseTexId);
      meta.holdNormalTex(context_id, normalTexId);
      meta.holdAlphaTex(context_id, alphaTexId);
      meta.forceNonMetal = true;
      meta.materialData1.x = grass_base.mapTextureIndexToType(textureIndex);

      metaHorizontal = meta;
      metaHorizontal.setIndexBufferIndex(context_id->gpuGrassHorizontal.indexBufferBindless);
      metaHorizontal.setVertexBufferIndex(context_id->gpuGrassHorizontal.vertexBufferBindless);
      metaHorizontal.setAhsVertexBufferIndex(context_id->gpuGrassHorizontal.ahsBufferBindless);

      for (auto *grass : {&context_id->gpuGrassBillboard, &context_id->gpuGrassHorizontal})
      {
        grass->textureSlots[textureIndex].alphaTexId = alphaTexId;
        grass->textureSlots[textureIndex].diffuseTexId = diffuseTexId;
        grass->textureSlots[textureIndex].ommState = OmmState::None;
      }
    }
  }

  make_mappings(context_id, metaSize);
}

static void init_grass_billboards(ContextId context_id)
{
  // With OMM there is one cutout for each texture, thus process_omm() replaces this shared BLAS.
  const bool needSharedBlas = !context_id->ommEnabled;
  if (make_grass_buffer(context_id, context_id->gpuGrassBillboard, false) && needSharedBlas)
    create_blas(context_id->gpuGrassBillboard, context_id->gpuGrassBillboard.blas);
  if (make_grass_buffer(context_id, context_id->gpuGrassHorizontal, true) && needSharedBlas)
    create_blas(context_id->gpuGrassHorizontal, context_id->gpuGrassHorizontal.blas);
}

static void close_grass_billboards(ContextId context_id)
{
  auto closeGrass = [context_id](Context::GPUGrassBillboard &grass) {
    for (auto &slot : grass.textureSlots)
      discard_grass_omm_bake(context_id, slot);
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
  if (!context_id->hasAny(Features::GPUGrass))
    return;

  bvhConnection.contexts.insert(context_id);
  init_grass_billboards(context_id);
}

void on_unload_scene(ContextId context_id)
{
  bvhConnection.teardown();
  bvhConnection.instancesView = {};
  bvhConnection.instanceCountView = {};

  auto unloadOne = [context_id](Context::GPUGrassBillboard &grass) {
    for (auto &slot : grass.textureSlots)
      discard_grass_omm_bake(context_id, slot);
    grass.textureSlots.clear();
    if (grass.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
      context_id->freeMetaRegion(grass.metaAllocId);
    grass.metaSize = 0;
    grass.mappingBase = 0;
  };
  unloadOne(context_id->gpuGrassBillboard);
  unloadOne(context_id->gpuGrassHorizontal);

  for (auto &tex : context_id->gpuGrassTextures)
    context_id->releaseTexture(tex.getTexId());
  context_id->gpuGrassTextures.clear();
}

void teardown(ContextId context_id)
{
  if (!context_id->hasAny(Features::GPUGrass))
    return;

  bvh::gpugrass::on_unload_scene(context_id);
  bvhConnection.contexts.erase(context_id);
  close_grass_billboards(context_id);
  ShaderGlobal::set_float(gpu_grass_bvh_rangeVarId, 0);
}

static void fail_slot(GrassOmmBake &slot, const char *orientation, const char *reason)
{
  const String label(0, "%s GPU grass %s billboard", get_managed_texture_name(slot.diffuseTexId), orientation);
  fail_grass_omm_bake(slot, label.c_str(), reason);
}

// The billboards are permanent, thus this needs no discarded-bake bookkeeping as the mesh pipeline does.
// The caller must give it a valid GPU-dispatch context.
void process_omm(ContextId context_id)
{
  if (!context_id->ommEnabled || !context_id->hasAny(Features::GPUGrass))
    return;

  TIME_PROFILE(gpu_grass__process_omm);

  FRAMEMEM_REGION;
  bool mappingsDirty = false;

  auto processOne = [&](Context::GPUGrassBillboard &grass, const char *orientation) {
    CHECK_LOST_DEVICE_STATE();
    for (auto [textureIndex, slot] : enumerate(grass.textureSlots))
    {
      if (slot.ommState == OmmState::Built || slot.ommState == OmmState::Failed)
        continue;
      if (slot.alphaTexId == BAD_TEXTUREID)
      {
        fail_slot(slot, orientation, grass_omm_failure_text(OmmFailure::NoAlphaSource));
        continue;
      }

      String waitReason;
      if (const OmmTextureWait wait = wait_for_grass_omm_texture(slot.alphaTexId, slot.ommWaitAttempts, waitReason);
          wait != OmmTextureWait::Ready)
      {
        if (wait == OmmTextureWait::GaveUp)
          fail_slot(slot, orientation, waitReason.c_str());
        continue;
      }

      if (slot.ommState == OmmState::None)
      {
        if (!render::omm::has_free_bake_slot(context_id->ommContext))
          continue;

        const render::omm::BakeInput input{
          .texCoordBuffer = grass.vertexBuffer.getBuf(),
          .indexBuffer = grass.indexBuffer.getBuf(),
          .texCoordOffsetInBytes = static_cast<uint32_t>(offsetof(GPUGrassVertexData, texcoord)),
          .texCoordStrideInBytes = sizeof(GPUGrassVertexData),
          .indexCount = Context::GPUGrassBillboard::INDEX_COUNT,
        };
        if (!start_grass_omm_bake(context_id, slot, input))
          fail_slot(slot, orientation, grass_omm_failure_text(OmmFailure::BakeStartFailed));
        continue;
      }

      if (slot.ommState == OmmState::Baking)
      {
        const render::omm::ConsumeBakeResult r =
          render::omm::consume_bake(context_id->ommContext, slot.ommBakeHandle, slot.ommBakeResult, &slot.ommBakeStats);
        if (r == render::omm::ConsumeBakeResult::NotReady)
          continue;
        if (r == render::omm::ConsumeBakeResult::Failed)
        {
          slot.ommBakeHandle = {};
          fail_slot(slot, orientation, grass_omm_failure_text(OmmFailure::ReadbackInvalid));
          continue;
        }
        slot.ommState = OmmState::Ready;
      }

      if (slot.ommState == OmmState::Ready)
      {
        OmmBuildInfos ommBuilds;
        OmmBuildResults ommResults;
        const OmmFailure failure = build_grass_omm_array(slot.ommBakeResult, slot.ommBakeStats, slot.omm, ommBuilds, ommResults);
        if (is_in_lost_device_state)
          return;
        if (failure != OmmFailure::None)
        {
          // fail_slot hands the buffers to the viewer, thus it must come before the release.
          fail_slot(slot, orientation, grass_omm_failure_text(failure));
          release_omm_result(context_id->ommContext, slot.ommBakeResult);
          continue;
        }
        // The post-build flush of the OMM array orders it before the BLAS build below.
        build_pending_omm_arrays(context_id->ommContext, ommBuilds, ommResults);
        const auto linkage = render::omm::make_geometry_linkage(slot.ommBakeResult, slot.omm.get());
        if (!create_blas(grass, slot.blas, &linkage))
        {
          fail_slot(slot, orientation, "its BLAS could not be built");
          continue;
        }
        slot.ommState = OmmState::Built;

        split_gpu_address(bvhConnection.metainfoMappingsCpu[grass.mappingBase + textureIndex].blas, slot.blas.getGPUAddress());
        bvhConnection.anyBlasPublished = true;
        mappingsDirty = true;
      }
    }
  };

  processOne(context_id->gpuGrassBillboard, "vertical");
  processOne(context_id->gpuGrassHorizontal, "horizontal");

  if (mappingsDirty)
    upload_mappings();
}

void generate_instances(ContextId context_id, bool has_grass)
{
  if (!context_id->hasAny(Features::GPUGrass))
    return;

  bvhConnection.instancesView = {};
  bvhConnection.instanceCountView = {};
  ShaderGlobal::set_float(gpu_grass_bvh_rangeVarId, 0); // Reset to 0 just in case something below returns

  if (!has_grass)
    return;

  if (bvhConnection.getMaxRange() < 0.01)
    return;

  if (!context_id->gpuGrassBillboard.hasGeometry() || !context_id->gpuGrassHorizontal.hasGeometry() ||
      context_id->gpuGrassTextures.empty())
    return;

  if (!bvhConnection.metainfoMappings || !bvhConnection.anyBlasPublished)
    return;

  for (auto &tex : context_id->gpuGrassTextures)
    prefetch_managed_texture(tex.getTexId());

  TIME_D3D_PROFILE(GPUGrass_generate_instances)

  G_ASSERT(bvhConnection.isReady());
  bvhConnection.prepare();

  makeIndirectArgs->dispatchThreads(1, 1, 1);

  G_ASSERT(context_id->gpuGrassBillboard.metaSize == context_id->gpuGrassHorizontal.metaSize);
  ShaderGlobal::set_buffer(gpu_grass_bvh_mappingVarId, bvhConnection.metainfoMappings);
  ShaderGlobal::set_int(gpu_grass_bvh_meta_countVarId, context_id->gpuGrassBillboard.metaSize);
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
  if (context_id->hasAny(Features::GPUGrass))
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

// A grass texture that has no BLAS yet is not in the BVH, thus it has no address to compare with.
void collect_blas_addresses(ContextId context_id, dag::Vector<uint64_t> &addresses)
{
  for (auto *grass : {&context_id->gpuGrassBillboard, &context_id->gpuGrassHorizontal})
  {
    if (grass->blas)
      addresses.push_back(grass->blas.getGPUAddress());
    for (auto &slot : grass->textureSlots)
      if (slot.blas)
        addresses.push_back(slot.blas.getGPUAddress());
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
  if (bvhConnection.metainfoMappings)
    gpuGrassMemory += bvhConnection.metainfoMappings->getSize();
  for (auto *grass : {&context_id->gpuGrassBillboard, &context_id->gpuGrassHorizontal})
  {
    if (!grass->hasGeometry())
      continue;
    gpuGrassMemory += grass->vertexBuffer->getSize() + grass->indexBuffer->getSize();
    if (grass->ahsBuffer)
      gpuGrassMemory += grass->ahsBuffer->getSize();
    if (grass->blas)
      gpuGrassMemory += grass->blas.getASSize();
    for (auto &slot : grass->textureSlots)
    {
      if (slot.blas)
        gpuGrassMemory += slot.blas.getASSize();
      if (slot.omm)
        gpuGrassMemory += slot.omm.getASSize();
      if (slot.ommBakeResult.indexBuffer)
        gpuGrassMemory += slot.ommBakeResult.indexBuffer->getSize();
    }
  }

  gpuGrassTexturesMemory = 0;
  for (auto &tex : context_id->gpuGrassTextures)
    gpuGrassTexturesMemory += tex->getSize();
}

} // namespace bvh::gpugrass
