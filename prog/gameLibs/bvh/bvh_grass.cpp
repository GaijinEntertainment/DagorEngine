// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <render/randomGrass.h>
#include <render/randomGrassInstance.hlsli>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_eventQueryHolder.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>
#include <math/dag_half.h>

#include "bvh_context.h"
#include "bvh_omm.h"
#include "bvh_tools.h"
#include "bvh_generic_connection.h"
#include <render/omm.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_lock.h>

namespace bvh
{

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

namespace grass
{

struct BVHVertex
{
  Point3 position;
  uint32_t normal;
  Point2 texcoord;
};

static UniqueBLAS create_grass_blas(ContextId context_id, const BVHGeometryBufferWithOffset &geometry, int vcount, int icount,
  const RaytraceGeometryDescription::OpacityMicroMapLinkage *omm_linkage = nullptr)
{
  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = geometry.getVertexBuffer(context_id);
  desc.data.triangles.indexBuffer = geometry.getIndexBuffer(context_id);
  desc.data.triangles.vertexCount = vcount;
  desc.data.triangles.vertexStride = sizeof(BVHVertex);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.vertexOffsetExtraBytes = geometry.vbOffset;
  desc.data.triangles.indexCount = icount;
  desc.data.triangles.indexOffset = geometry.ibOffset / 2;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::NONE;
  if (omm_linkage)
  {
    desc.ommLinkage = *omm_linkage;
    desc.extraDataAvailableMask.hasOpacityMicroMapLinkage = true;
  }

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

  UniqueBLAS blas = UniqueBLAS::create(&desc, 1, buildInfo.flags);
  HANDLE_LOST_DEVICE_STATE(blas, blas);

  buildInfo.geometryDesc = &desc;
  buildInfo.geometryDescCount = 1;
  buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = blas.getBuildScratchSize();

  HANDLE_LOST_DEVICE_STATE(buildInfo.scratchSpaceBuffer, UniqueBLAS());

  d3d::build_bottom_acceleration_structure(blas.get(), buildInfo);

  return blas;
}

struct LOD
{
  TEXTUREID diffuseTexId;
  TEXTUREID alphaTexId;
  BVHGeometryBufferWithOffset geometry;
  UniqueBVHBufferWithOffset ahsVertices;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;

  // OMM is baked asynchronously after the BLAS already exists, then the
  // BLAS is rebuilt with the OMM linked. The vertex/index counts and
  // mapping slot are kept so the rebuild can reproduce the geometry and
  // refresh the BLAS address the instancer reads.
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  int metaMappingIndex = -1;
  Mesh::OmmState ommState = Mesh::OmmState::None;
  render::omm::BakeHandle ommBakeHandle;
  render::omm::BakeResult ommBakeResult;
  UniqueOMM omm;

  void teardown(ContextId context_id)
  {
    if (context_id->ommEnabled)
    {
      if (ommState == Mesh::OmmState::Baking)
        render::omm::discard_bake(context_id->ommContext, ommBakeHandle);
      render::omm::clear_result(ommBakeResult);
    }
    omm.reset();
    ommBakeHandle = {};
    ommState = Mesh::OmmState::None;

    context_id->releaseTexture(diffuseTexId);
    context_id->releaseTexture(alphaTexId);
    context_id->releaseBuffer(geometry.processedVertexBuffer.get());
    context_id->releaseBuffer(ahsVertices.get());
    context_id->freeMetaRegion(metaAllocId);
    diffuseTexId = BAD_TEXTUREID;
    alphaTexId = BAD_TEXTUREID;
    geometry.close(context_id);
    blas.reset();
  }
};

struct Layer
{
  eastl::vector<LOD> lods;
};

static eastl::vector<Layer> layers;

static int layer_id_map[128] = {};

struct BVHConnection : public bvh::BVHConnection
{
  dag::Vector<RandomGrassBvhMapping> metainfoMappingsCpu;

  BVHConnection(const char *name) : bvh::BVHConnection(name) {}

  float getMaxRange() const override { return contexts.empty() ? 0 : (*contexts.begin())->grassRange; }
} bvhConnection("grass");

void init()
{
  bvhConnection.init();
  RandomGrass::setBVHConnection(&bvhConnection);
}
void teardown()
{
  bvhConnection.teardown();
  bvhConnection.metainfoMappings.close();
  bvhConnection.metainfoMappingsCpu.clear();
  RandomGrass::setBVHConnection(nullptr);
}

void init(ContextId context_id)
{
  if (context_id->hasAny(Features::Grass))
    bvhConnection.contexts.insert(context_id);
}

void teardown(ContextId context_id)
{
  if (context_id->hasAny(Features::Grass))
    bvhConnection.contexts.insert(context_id);

  bvhConnection.contexts.erase(context_id);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->hasAny(Features::Grass))
    return;

  for (auto &layer : layers)
    for (auto &lod : layer.lods)
      lod.teardown(context_id);
  layers.clear();
  bvhConnection.teardown();
  bvhConnection.metainfoMappings.close();
  bvhConnection.metainfoMappingsCpu.clear();
}

void reload_grass(ContextId context_id, RandomGrass *grass)
{
  if (!grass || !context_id->hasAny(Features::Grass))
    return;

  bvh::grass::on_unload_scene(context_id);

  auto maxLodCount = 8;
  auto metaCount = grass->getGrassLayerCount() * maxLodCount;

  if (metaCount == 0)
    return;

  d3d::GpuAutoLock gpuLock;

  bvhConnection.metainfoMappings = dag::buffers::create_persistent_sr_structured(sizeof(RandomGrassBvhMapping), metaCount,
    "bvh_grass_mapping", d3d::buffers::Init::No, RESTAG_BVH);

  HANDLE_LOST_DEVICE_STATE(bvhConnection.metainfoMappings, );

  bvhConnection.metainfoMappingsCpu.resize(metaCount);
  memset(bvhConnection.metainfoMappingsCpu.data(), 0, bvhConnection.metainfoMappingsCpu.size() * sizeof(RandomGrassBvhMapping));

  for (int layerIx = 0; layerIx < grass->getGrassLayerCount(); ++layerIx)
  {
    auto &layer = *grass->getGrassLayerAt(layerIx);

    G_ASSERT(layerIx < countof(layer_id_map));
    layer_id_map[layerIx] = bvh_id_gen.fetch_add(1);

    auto &bvhLayer = layers.emplace_back();
    for (auto [lodIx, lod] : enumerate(layer.lods))
    {
      auto &bvhLod = bvhLayer.lods.emplace_back();

      auto &elem = lod.mesh->getAllElems().front();

      ChannelParser parser;
      if (!elem.mat->enum_channels(parser, parser.flags))
        return;

      G_ASSERT(parser.positionFormat == VSDT_FLOAT3);
      G_ASSERT(parser.normalFormat == VSDT_FLOAT3);
      G_ASSERT(parser.texcoordFormat == VSDT_FLOAT2);

      struct MeshVertex
      {
        Point3 position;
        Point3 normal;
        Point2 texcoord;
      };
      using Index = uint16_t;

      auto indexDwords = divide_up(sizeof(Index) * elem.numf * 3, 4);
      auto vertexDwords = divide_up(sizeof(BVHVertex) * elem.numv, 4);

      auto alloc = context_id->allocateSourceGeometry(indexDwords + vertexDwords);

      bvhLod.geometry.heapIndex = alloc.heapIx;
      bvhLod.geometry.bindlessIndex = alloc.bindlessId;
      bvhLod.geometry.bufferRegion = alloc.region;
      bvhLod.geometry.ibOffset = context_id->getSourceBufferOffset(alloc.heapIx, alloc.region);
      bvhLod.geometry.vbOffset = bvhLod.geometry.ibOffset + indexDwords * 4;

      HANDLE_LOST_DEVICE_STATE(bvhLod.geometry, );

      {
        if (auto gData = lock_sbuffer<uint8_t>(bvhLod.geometry.getIndexBuffer(context_id), bvhLod.geometry.ibOffset,
              (indexDwords + vertexDwords) * 4, VBLOCK_WRITEONLY))
        {
          auto ii = [&](int i) -> Index & { return *(Index *)&gData[sizeof(Index) * i]; };
          auto vi = [&](int i) -> BVHVertex & { return *(BVHVertex *)&gData[indexDwords * 4 + sizeof(BVHVertex) * i]; };
          auto packNormal = [](const Point3 &n) {
            return (uint32_t(n.x * 127.0f + 128.0f) << 16) | (uint32_t(n.y * 127.0f + 128.0f) << 8) |
                   (uint32_t(n.z * 127.0f + 128.0f));
          };

          if (auto *elemIb = elem.vertexData->getIBMem<Index>(elem.si, elem.numf))
            for (unsigned int indexNo = 0; indexNo < elem.numf * 3; indexNo++)
              ii(indexNo) = elemIb[indexNo] - elem.sv;

          if (auto *elemVb = elem.vertexData->getVBMem<MeshVertex>(elem.baseVertex, elem.sv, elem.numv))
            for (unsigned int vertexNo = 0; vertexNo < elem.numv; vertexNo++)
            {
              vi(vertexNo).position = elemVb[vertexNo].position;
              vi(vertexNo).texcoord = elemVb[vertexNo].texcoord;
              vi(vertexNo).normal = packNormal(elemVb[vertexNo].normal);
            }
        }
      }

      bvhLod.diffuseTexId = lod.diffuseTexId;
      bvhLod.alphaTexId = lod.alphaTexId;
      bvhLod.vertexCount = elem.numv;
      bvhLod.indexCount = elem.numf * 3;
      // Build without OMM first so grass is immediately present in the BVH; process_omm() bakes the OMM
      // over the next frames and rebuilds this BLAS with it once ready. Skip the bake entirely when the
      // context has no OMM support.
      bvhLod.ommState = context_id->ommEnabled ? Mesh::OmmState::None : Mesh::OmmState::Failed;
      bvhLod.blas = create_grass_blas(context_id, bvhLod.geometry, elem.numv, elem.numf * 3);
      HANDLE_LOST_DEVICE_STATE(bvhLod.blas, );

      TIME_PROFILE(meta_lock_grass);
      bvhLod.metaAllocId = context_id->allocateMetaRegion(1, "grass");
      LockedMetaAccess lockedMeta(*context_id, bvhLod.metaAllocId);
      auto &meta = lockedMeta[0];

      meta.markInitialized();

      meta.holdAlphaTex(context_id, lod.alphaTexId);
      meta.holdAlbedoTex(context_id, lod.diffuseTexId);

      d3d::resource_barrier(ResourceBarrierDesc(bvhLod.geometry.getIndexBuffer(context_id), bindlessSRVBarrier));

      meta.materialType = MeshMeta::bvhMaterialRendinst | MeshMeta::bvhMaterialGrass;
      meta.setIndexBitAndTexcoordFormat(2, VSDT_FLOAT2);
      meta.texcoordOffset = offsetof(BVHVertex, texcoord);
      meta.normalOffset = offsetof(BVHVertex, normal);
      meta.colorOffset = 0xFFU;
      meta.vertexStride = sizeof(BVHVertex);
      meta.setIndexBufferIndex(bvhLod.geometry.bindlessIndex);
      meta.setVertexBufferIndex(bvhLod.geometry.bindlessIndex);
      meta.startIndex = 0;
      meta.startVertex = 0;
      meta.vertexOffset = bvhLod.geometry.vbOffset;

      auto pack = [](void *d, const ColorRange &r) {
        uint32_t p[4];

        uint32_t sr = float_to_half(r.start.r);
        uint32_t sg = float_to_half(r.start.g);
        uint32_t sb = float_to_half(r.start.b);
        uint32_t er = float_to_half(r.end.r);
        uint32_t eg = float_to_half(r.end.g);
        uint32_t eb = float_to_half(r.end.b);

        p[0] = sr | (sg << 16);
        p[1] = sb;
        p[2] = er | (eg << 16);
        p[3] = eb;

        memcpy(d, p, sizeof(p));
      };

      pack(&meta.materialData1, layer.info.colors[CHANNEL_RED]);
      pack(&meta.materialData2, layer.info.colors[CHANNEL_GREEN]);
      pack(&meta.layerData, layer.info.colors[CHANNEL_BLUE]);

      auto metaIx = grass->getGrassLayerCount() * lodIx + layerIx;
      bvhLod.metaMappingIndex = metaIx;
      auto blasHandle = d3d::get_raytrace_acceleration_structure_gpu_handle(bvhLod.blas.get()).handle;
      auto &mapping = bvhConnection.metainfoMappingsCpu[metaIx];

      mapping.blas.x = blasHandle & GPU_ADDRESS_LOW_MASK;
      mapping.blas.y = blasHandle >> GPU_ADDRESS_HIGH_SHIFT;
      mapping.metaIndex = MeshMetaAllocator::decode(bvhLod.metaAllocId);

      // Need to fit in 15 bits so there is enough space for the alpha value
      G_ASSERT(MeshMetaAllocator::decode(bvhLod.metaAllocId) < (1 << 15));

      uint32_t bindlessIndex;
      ProcessorInstances::getAHSProcessor().process(context_id, bvhLod.geometry, bvhLod.ahsVertices, bindlessIndex, 2, elem.numf * 3,
        offsetof(BVHVertex, texcoord), VSDT_FLOAT2, sizeof(BVHVertex), -1);

      uint32_t indexCount = elem.numf * 3;

      meta.indexCount = indexCount;
      meta.setAhsVertexBufferIndex(bindlessIndex);
      if (bindlessIndex > BVH_BINDLESS_BUFFER_MAX)
        logerr("BVH Grass vertex buffer bindless index out of range: %u", bindlessIndex);
      if (indexCount > 0xFFFFU)
        logerr("BVH Grass vertex buffer index count out of range: %u", indexCount);

      meta.materialType |= MeshMeta::bvhMaterialAlphaInRed;
    }
  }

  bvhConnection.metainfoMappings->updateData(0, bvhConnection.metainfoMappingsCpu.size() * sizeof(RandomGrassBvhMapping),
    bvhConnection.metainfoMappingsCpu.data(), 0);
}

// The mesh pipeline's bake helpers in bvh_omm carry mesh-only concerns (texcoord-format lookup,
// per-object subdivision budgeting). Grass geometry is always float2 UVs with alpha in the red channel,
// so its bake is a fixed, trivial variant kept local here rather than generalizing the mesh helpers.
static bool begin_lod_omm_bake(ContextId context_id, LOD &lod)
{
  BaseTexture *texture = acquire_managed_tex(lod.alphaTexId);
  if (!texture)
    return false;

  render::omm::BakeInput input{
    .alphaTexture = texture,
    .texCoordBuffer = lod.geometry.getVertexBuffer(context_id),
    .indexBuffer = lod.geometry.getIndexBuffer(context_id),
    .alphaTextureChannel = 0, // grass stores alpha in red (bvhMaterialAlphaInRed)
    .texCoordFormat = render::omm::TexCoordFormat::UV32_FLOAT,
    .texCoordOffsetInBytes = static_cast<uint32_t>(lod.geometry.vbOffset + offsetof(BVHVertex, texcoord)),
    .texCoordStrideInBytes = sizeof(BVHVertex),
    .indexFormat = render::omm::IndexFormat::UINT16,
    .indexCount = lod.indexCount,
    .indexStrideInBytes = sizeof(uint16_t),
    .indexBufferOffsetInBytes = lod.geometry.ibOffset,
    .globalFormat = render::omm::Format::OC1_2_State,
  };

  const bool dispatched = render::omm::begin_bake(context_id->ommContext, input, lod.ommBakeHandle);
  release_managed_tex(lod.alphaTexId);
  return dispatched;
}

static bool build_lod_omm_array(LOD &lod, OmmBuildInfos &builds, OmmBuildResults &results)
{
  render::omm::BakeResult &result = lod.ommBakeResult;
  if (
    !result.arrayData || !result.descArray || !result.indexBuffer || result.arrayBuildDescs.empty() || result.blasLinkageDescs.empty())
    return false;

  auto sizeInfo = render::omm::make_array_build_info(result, nullptr, 0, 0, RaytraceBuildFlags::FAST_TRACE);
  const raytrace::AccelerationStructureSizes sizes = d3d::raytrace::calculate_acceleration_structure_sizes(sizeInfo);
  if (!sizes.structureSizeInBytes)
    return false;

  lod.omm = UniqueOMM::create_omm(sizes.structureSizeInBytes);
  HANDLE_LOST_DEVICE_STATE(lod.omm, false);
  if (!lod.omm)
    return false;

  uint32_t scratchOffset = 0;
  Sbuffer *scratchBuffer = alloc_scratch_buffer(sizes.buildScratchBufferSizeInBytes, scratchOffset);
  if (sizes.buildScratchBufferSizeInBytes)
    HANDLE_LOST_DEVICE_STATE(scratchBuffer, false);

  raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo build;
  build.omm = lod.omm.get();
  build.ommtabi = render::omm::make_array_build_info(result, scratchBuffer, scratchOffset, sizes.buildScratchBufferSizeInBytes,
    RaytraceBuildFlags::FAST_TRACE);
  builds.push_back(build);
  results.push_back(&result);
  return true;
}

// Advances each LOD's async OMM bake and, once ready, rebuilds its BLAS with the OMM linked. Called per
// frame from process_meshes (a valid GPU-dispatch context). Grass LODs are static and persistent, so
// there is no discard/leak bookkeeping like the mesh pipeline needs -- a LOD only leaves the state
// machine by reaching Built or Failed, and teardown discards any bake still in flight.
void process_omm(ContextId context_id)
{
  if (!context_id->ommEnabled || !context_id->hasAny(Features::Grass) || layers.empty())
    return;

  FRAMEMEM_REGION;
  bool mappingsDirty = false;

  for (auto &layer : layers)
    for (auto &lod : layer.lods)
    {
      if (lod.ommState == Mesh::OmmState::Built || lod.ommState == Mesh::OmmState::Failed)
        continue;
      if (lod.alphaTexId == BAD_TEXTUREID)
      {
        lod.ommState = Mesh::OmmState::Failed;
        continue;
      }

      // Bake only against the full-resolution alpha: an OMM baked from a half-streamed mip can mark
      // micro-triangles opaque where the full-res texture is cut out, dropping detail. This is a
      // background upgrade, so just poll until the texture is fully resident.
      if (get_managed_res_cur_tql(lod.alphaTexId) != get_managed_res_max_tql(lod.alphaTexId))
      {
        prefetch_and_check_managed_texture_loaded(lod.alphaTexId, true);
        mark_managed_tex_lfu(lod.alphaTexId);
        continue;
      }

      if (lod.ommState == Mesh::OmmState::None)
      {
        if (!render::omm::has_free_bake_slot(context_id->ommContext))
          continue;

        if (!begin_lod_omm_bake(context_id, lod))
        {
          lod.ommState = Mesh::OmmState::Failed;
          continue;
        }
        lod.ommState = Mesh::OmmState::Baking;
        continue;
      }

      if (lod.ommState == Mesh::OmmState::Baking)
      {
        const render::omm::ConsumeBakeResult r =
          render::omm::consume_bake(context_id->ommContext, lod.ommBakeHandle, lod.ommBakeResult);
        if (r == render::omm::ConsumeBakeResult::NotReady)
          continue;
        if (r == render::omm::ConsumeBakeResult::Failed)
        {
          lod.ommBakeHandle = {};
          lod.ommState = Mesh::OmmState::Failed;
          continue;
        }
        lod.ommState = Mesh::OmmState::Ready;
      }

      if (lod.ommState == Mesh::OmmState::Ready)
      {
        OmmBuildInfos ommBuilds;
        OmmBuildResults ommResults;
        if (!build_lod_omm_array(lod, ommBuilds, ommResults))
        {
          render::omm::clear_result(lod.ommBakeResult);
          lod.ommState = Mesh::OmmState::Failed;
          continue;
        }
        // Build the OMM array now (its post-build flush orders it before the BLAS), then rebuild this
        // LOD's BLAS with the OMM linked.
        build_pending_omm_arrays(ommBuilds, ommResults);
        const auto linkage = render::omm::make_geometry_linkage(lod.ommBakeResult, lod.omm.get());
        lod.blas = create_grass_blas(context_id, lod.geometry, lod.vertexCount, lod.indexCount, &linkage);
        lod.ommState = Mesh::OmmState::Built;

        // The rebuilt BLAS has a new GPU address; refresh the mapping the instancer reads.
        if (lod.metaMappingIndex >= 0 && lod.metaMappingIndex < int(bvhConnection.metainfoMappingsCpu.size()) && lod.blas)
        {
          const auto blasHandle = d3d::get_raytrace_acceleration_structure_gpu_handle(lod.blas.get()).handle;
          auto &mapping = bvhConnection.metainfoMappingsCpu[lod.metaMappingIndex];
          mapping.blas.x = blasHandle & GPU_ADDRESS_LOW_MASK;
          mapping.blas.y = blasHandle >> GPU_ADDRESS_HIGH_SHIFT;
          mappingsDirty = true;
        }
      }
    }

  if (mappingsDirty && bvhConnection.metainfoMappings)
    bvhConnection.metainfoMappings->updateData(0, bvhConnection.metainfoMappingsCpu.size() * sizeof(RandomGrassBvhMapping),
      bvhConnection.metainfoMappingsCpu.data(), 0);
}

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->hasAny(Features::Grass))
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

UniqueBLAS *get_blas(int layer_ix, int lod_ix)
{
  if (layer_ix < 0 || layer_ix >= layers.size())
    return nullptr;

  auto &layer = layers[layer_ix];
  if (lod_ix < 0 || lod_ix >= layer.lods.size())
    return nullptr;

  return &layer.lods[lod_ix].blas;
}

void get_memory_statistics(ContextId context_id, int64_t &vb, int64_t &ib, int64_t &blas, int64_t &meta, int64_t &queries)
{
  vb = ib = blas = meta = queries = 0;
  for (auto &layer : layers)
    for (auto &lod : layer.lods)
    {
      vb += context_id->getSourceBufferSize(lod.geometry.heapIndex, lod.geometry.bufferRegion);
      blas += d3d::get_raytrace_acceleration_structure_size(lod.blas.get());
    }
  if (bvhConnection.instances)
    queries = bvhConnection.instances->getElementSize() * bvhConnection.instances->getNumElements();
  if (bvhConnection.metainfoMappings)
    meta = bvhConnection.metainfoMappings->getElementSize() * bvhConnection.metainfoMappings->getNumElements();
}

} // namespace grass

} // namespace bvh