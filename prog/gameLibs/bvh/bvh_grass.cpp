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

// The write of the address is what puts a LOD into the BVH.
static void publish_grass_blas_address(RandomGrassBvhMapping &mapping, const UniqueBLAS &blas)
{
  const auto handle = d3d::get_raytrace_acceleration_structure_gpu_handle(blas.get()).handle;
  mapping.blas.x = handle & GPU_ADDRESS_LOW_MASK;
  mapping.blas.y = handle >> GPU_ADDRESS_HIGH_SHIFT;
}

struct LOD
{
  TEXTUREID diffuseTexId;
  TEXTUREID alphaTexId;
  BVHGeometryBufferWithOffset geometry;
  UniqueBVHBufferWithOffset ahsVertices;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;

  // Kept so process_omm() can build the geometry some frames after reload_grass().
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  int metaMappingIndex = -1;
  Mesh::OmmState ommState = Mesh::OmmState::None;
  uint32_t ommWaitAttempts = 0;
  render::omm::BakeHandle ommBakeHandle;
  render::omm::BakeResult ommBakeResult;
  render::omm::BakeStats ommBakeStats;
  render::omm::DebugBakeSource ommDebugBakeSource;
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
  String assetName;
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

void teardown(ContextId context_id)
{
  bvh::grass::on_unload_scene(context_id);
  bvhConnection.contexts.erase(context_id);
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
    bvhLayer.assetName = layer.info.resName;
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
      // Grass is alpha-tested, thus with OMM the BLAS must wait for process_omm(). Without OMM the
      // any-hit shader does the cutout, and the BLAS can be built now.
      bvhLod.ommState = context_id->ommEnabled ? Mesh::OmmState::None : Mesh::OmmState::Failed;
      if (!context_id->ommEnabled)
      {
        bvhLod.blas = create_grass_blas(context_id, bvhLod.geometry, elem.numv, elem.numf * 3);
        HANDLE_LOST_DEVICE_STATE(bvhLod.blas, );
      }

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

      meta.materialType |= MeshMeta::bvhMaterialAlphaInRed;

      auto metaIx = grass->getGrassLayerCount() * lodIx + layerIx;
      bvhLod.metaMappingIndex = metaIx;
      auto &mapping = bvhConnection.metainfoMappingsCpu[metaIx];
      mapping.metaIndex = MeshMetaAllocator::decode(bvhLod.metaAllocId);

      // Only the path without OMM has a BLAS at this time; otherwise process_omm() writes it later.
      if (bvhLod.blas)
        publish_grass_blas_address(mapping, bvhLod.blas);

      // Need to fit in 15 bits so there is enough space for the alpha value
      G_ASSERT(MeshMetaAllocator::decode(bvhLod.metaAllocId) < (1 << 15));

      uint32_t indexCount = elem.numf * 3;
      meta.indexCount = indexCount;
      if (indexCount > 0xFFFFU)
        logerr("BVH Grass vertex buffer index count out of range: %u", indexCount);

      if (!context_id->ommEnabled)
      {
        uint32_t bindlessIndex;
        ProcessorInstances::getAHSProcessor().process(context_id, bvhLod.geometry, bvhLod.ahsVertices, bindlessIndex, 2, elem.numf * 3,
          offsetof(BVHVertex, texcoord), VSDT_FLOAT2, sizeof(BVHVertex), -1);

        meta.setAhsVertexBufferIndex(bindlessIndex);
        if (bindlessIndex > BVH_BINDLESS_BUFFER_MAX)
          logerr("BVH Grass vertex buffer bindless index out of range: %u", bindlessIndex);
      }
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
    .texCoordFormat = render::omm::TexCoordFormat::Float2,
    .texCoordOffsetInBytes = static_cast<uint32_t>(lod.geometry.vbOffset + offsetof(BVHVertex, texcoord)),
    .texCoordStrideInBytes = sizeof(BVHVertex),
    .indexFormat = render::omm::IndexFormat::UINT16,
    .indexCount = lod.indexCount,
    .indexStrideInBytes = sizeof(uint16_t),
    .indexBufferOffsetInBytes = lod.geometry.ibOffset,
    .globalFormat = render::omm::Format::OC1_2_State,
  };
  // Opacity counts, for the same reason as the mesh path (see start_omm_bake).
  input.bakeFlags |= render::omm::ENABLE_POST_DISPATCH_INFO_STATS;

  lod.ommDebugBakeSource = render::omm::make_debug_bake_source(input, lod.alphaTexId);

  const bool dispatched = render::omm::begin_bake(context_id->ommContext, input, lod.ommBakeHandle);
  release_managed_tex(lod.alphaTexId);
  return dispatched;
}

// The mapping of the LOD keeps its initial zero BLAS address, thus the LOD stays out of the BVH.
static void fail_grass_lod(const Layer &layer, size_t lod_ix, LOD &lod, const char *reason)
{
  logerr("BVH grass: dropping '%s' lod %u (diffuse '%s') from the BVH -- %s", layer.assetName.c_str(), unsigned(lod_ix),
    get_managed_texture_name(lod.diffuseTexId), reason);
  lod.ommState = Mesh::OmmState::Failed;

  const String label(0, "%s grass '%s' lod %u", get_managed_texture_name(lod.diffuseTexId), layer.assetName.c_str(), unsigned(lod_ix));
  publish_failed_grass_omm_debug_result(lod.ommBakeResult, lod.ommDebugBakeSource, label.c_str(), reason);
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
    for (auto [lodIx, lod] : enumerate(layer.lods))
    {
      if (lod.ommState == Mesh::OmmState::Built || lod.ommState == Mesh::OmmState::Failed)
        continue;
      if (lod.alphaTexId == BAD_TEXTUREID)
      {
        fail_grass_lod(layer, lodIx, lod, grass_omm_failure_text(Mesh::OmmFailure::NoAlphaSource));
        continue;
      }

      String waitReason;
      if (const OmmTextureWait wait = wait_for_grass_omm_texture(lod.alphaTexId, lod.ommWaitAttempts, waitReason);
          wait != OmmTextureWait::Ready)
      {
        if (wait == OmmTextureWait::GaveUp)
          fail_grass_lod(layer, lodIx, lod, waitReason.c_str());
        continue;
      }

      if (lod.ommState == Mesh::OmmState::None)
      {
        if (!render::omm::has_free_bake_slot(context_id->ommContext))
          continue;

        if (!begin_lod_omm_bake(context_id, lod))
        {
          fail_grass_lod(layer, lodIx, lod, grass_omm_failure_text(Mesh::OmmFailure::BakeStartFailed));
          continue;
        }
        lod.ommState = Mesh::OmmState::Baking;
        continue;
      }

      if (lod.ommState == Mesh::OmmState::Baking)
      {
        const render::omm::ConsumeBakeResult r =
          render::omm::consume_bake(context_id->ommContext, lod.ommBakeHandle, lod.ommBakeResult, &lod.ommBakeStats);
        if (r == render::omm::ConsumeBakeResult::NotReady)
          continue;
        if (r == render::omm::ConsumeBakeResult::Failed)
        {
          lod.ommBakeHandle = {};
          fail_grass_lod(layer, lodIx, lod, grass_omm_failure_text(Mesh::OmmFailure::ReadbackInvalid));
          continue;
        }
        lod.ommState = Mesh::OmmState::Ready;
      }

      if (lod.ommState == Mesh::OmmState::Ready)
      {
        OmmBuildInfos ommBuilds;
        OmmBuildResults ommResults;
        const Mesh::OmmFailure failure = build_grass_omm_array(lod.ommBakeResult, lod.ommBakeStats, lod.omm, ommBuilds, ommResults);
        if (is_in_lost_device_state)
          return;
        if (failure != Mesh::OmmFailure::None)
        {
          // fail_grass_lod hands the buffers to the viewer, thus it must come before clear_result.
          fail_grass_lod(layer, lodIx, lod, grass_omm_failure_text(failure));
          render::omm::clear_result(lod.ommBakeResult);
          continue;
        }
        // Build the OMM array now (its post-build flush orders it before the BLAS), then rebuild this
        // LOD's BLAS with the OMM linked.
        build_pending_omm_arrays(ommBuilds, ommResults);
        const auto linkage = render::omm::make_geometry_linkage(lod.ommBakeResult, lod.omm.get());
        lod.blas = create_grass_blas(context_id, lod.geometry, lod.vertexCount, lod.indexCount, &linkage);
        lod.ommState = Mesh::OmmState::Built;

        if (lod.metaMappingIndex >= 0 && lod.metaMappingIndex < int(bvhConnection.metainfoMappingsCpu.size()) && lod.blas)
        {
          publish_grass_blas_address(bvhConnection.metainfoMappingsCpu[lod.metaMappingIndex], lod.blas);
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

// A LOD that has no BLAS yet is not in the BVH, thus it has no address to compare with.
void collect_blas_addresses(dag::Vector<uint64_t> &addresses)
{
  for (auto &layer : layers)
    for (auto &lod : layer.lods)
      if (lod.blas)
        addresses.push_back(lod.blas.getGPUAddress());
}

void get_memory_statistics(ContextId context_id, int64_t &vb, int64_t &ib, int64_t &blas, int64_t &meta, int64_t &queries)
{
  vb = ib = blas = meta = queries = 0;
  for (auto &layer : layers)
    for (auto &lod : layer.lods)
    {
      vb += context_id->getSourceBufferSize(lod.geometry.heapIndex, lod.geometry.bufferRegion);
      if (lod.blas) // no BLAS until process_omm() links the OMM
        blas += d3d::get_raytrace_acceleration_structure_size(lod.blas.get());
    }
  if (bvhConnection.instances)
    queries = bvhConnection.instances->getElementSize() * bvhConnection.instances->getNumElements();
  if (bvhConnection.metainfoMappings)
    meta = bvhConnection.metainfoMappings->getElementSize() * bvhConnection.metainfoMappings->getNumElements();
}

} // namespace grass

} // namespace bvh