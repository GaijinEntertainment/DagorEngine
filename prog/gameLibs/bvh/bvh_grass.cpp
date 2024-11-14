// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <render/randomGrass.h>
#include <render/randomGrassInstance.hlsli>
#include <shaders/dag_shaderMesh.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_eventQueryHolder.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>
#include <math/dag_half.h>

#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_generic_connection.h"
#include <drv/3d/dag_bindless.h>

namespace bvh
{

void add_mesh(ContextId context_id, uint64_t mesh_id, const MeshInfo &info);
Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

namespace grass
{

struct BVHVertex
{
  Point3 position;
  uint32_t normal;
  Point2 texcoord;
};

static UniqueBLAS create_grass_blas(Sbuffer *vb, Sbuffer *ib, int vcount, int icount)
{
  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = vb;
  desc.data.triangles.indexBuffer = ib;
  desc.data.triangles.vertexCount = vcount;
  desc.data.triangles.vertexStride = sizeof(BVHVertex);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.indexCount = icount;
  desc.data.triangles.indexOffset = 0;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::NONE;

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.flags = RaytraceBuildFlags::FAST_TRACE;

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
  UniqueBuf vertices;
  UniqueBuf indices;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = -1;

  void teardown(ContextId context_id)
  {
    context_id->releaseTexure(diffuseTexId);
    context_id->releaseTexure(alphaTexId);
    context_id->releaseBuffer(vertices.getBuf());
    context_id->releaseBuffer(indices.getBuf());
    context_id->freeMetaRegion(metaAllocId);
    diffuseTexId = BAD_TEXTUREID;
    alphaTexId = BAD_TEXTUREID;
    vertices.close();
    indices.close();
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
  if (context_id->has(Features::Grass))
    bvhConnection.contexts.insert(context_id);
}

void teardown(ContextId context_id)
{
  if (context_id->has(Features::Grass))
    bvhConnection.contexts.insert(context_id);

  bvhConnection.contexts.erase(context_id);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::Grass))
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
  if (!grass || !context_id->has(Features::Grass))
    return;

  bvh::grass::on_unload_scene(context_id);

  auto maxLodCount = 8;
  auto metaCount = grass->getGrassLayerCount() * maxLodCount;

  if (metaCount == 0)
    return;

  bvhConnection.metainfoMappings =
    dag::buffers::create_persistent_sr_structured(sizeof(RandomGrassBvhMapping), metaCount, "bvh_grass_mapping");

  HANDLE_LOST_DEVICE_STATE(bvhConnection.metainfoMappings, );

  bvhConnection.metainfoMappingsCpu.resize(metaCount);
  memset(bvhConnection.metainfoMappingsCpu.data(), 0, bvhConnection.metainfoMappingsCpu.size() * sizeof(RandomGrassBvhMapping));

  for (int layerIx = 0; layerIx < grass->getGrassLayerCount(); ++layerIx)
  {
    auto &layer = *grass->getGrassLayerAt(layerIx);

    G_ASSERT(layerIx < countof(layer_id_map));
    layer_id_map[layerIx] = ++bvh_id_gen;

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

      static int counter = 0;

      struct MeshVertex
      {
        Point3 position;
        Point3 normal;
        Point2 texcoord;
      };
      using Index = uint16_t;

      auto indexDwords = divide_up(sizeof(Index) * elem.numf * 3, 4);
      auto vertexDwords = divide_up(sizeof(BVHVertex) * elem.numv, 4);

      bvhLod.indices = dag::buffers::create_persistent_sr_byte_address(indexDwords, String(64, "bvh_grass_indices_%d", counter++));
      bvhLod.vertices = dag::buffers::create_persistent_sr_byte_address(vertexDwords, String(64, "bvh_grass_vertices_%d", counter++));

      HANDLE_LOST_DEVICE_STATE(bvhLod.indices, );
      HANDLE_LOST_DEVICE_STATE(bvhLod.vertices, );

      {
        auto ibData = lock_sbuffer<Index>(bvhLod.indices.getBuf(), 0, 0, VBLOCK_WRITEONLY);
        auto vbData = lock_sbuffer<BVHVertex>(bvhLod.vertices.getBuf(), 0, 0, VBLOCK_WRITEONLY);

        if (ibData && vbData)
        {
          auto packNormal = [](const Point3 &n) {
            return (uint32_t(n.x * 127.0f + 128.0f) << 16) | (uint32_t(n.y * 127.0f + 128.0f) << 8) |
                   (uint32_t(n.z * 127.0f + 128.0f));
          };

          if (auto *elemIb = elem.vertexData->getIBMem<Index>(elem.si, elem.numf))
            for (unsigned int indexNo = 0; indexNo < elem.numf * 3; indexNo++)
              ibData[indexNo] = elemIb[indexNo] - elem.sv;

          if (auto *elemVb = elem.vertexData->getVBMem<MeshVertex>(elem.baseVertex, elem.sv, elem.numv))
            for (unsigned int vertexNo = 0; vertexNo < elem.numv; vertexNo++)
            {
              vbData[vertexNo].position = elemVb[vertexNo].position;
              vbData[vertexNo].texcoord = elemVb[vertexNo].texcoord;
              vbData[vertexNo].normal = packNormal(elemVb[vertexNo].normal);
            }
        }
      }

      bvhLod.diffuseTexId = lod.diffuseTexId;
      bvhLod.alphaTexId = lod.alphaTexId;
      bvhLod.blas = create_grass_blas(bvhLod.vertices.getBuf(), bvhLod.indices.getBuf(), elem.numv, elem.numf * 3);
      HANDLE_LOST_DEVICE_STATE(bvhLod.blas, );

      bvhLod.metaAllocId = context_id->allocateMetaRegion();
      auto &meta = context_id->meshMetaAllocator.get(bvhLod.metaAllocId);

      meta.markInitialized();

      uint32_t bindlessIndicesIndex, bindlessVerticesIndex;

      context_id->holdTexture(lod.alphaTexId, meta.alphaTextureAndSamplerIndex);
      context_id->holdTexture(lod.diffuseTexId, meta.albedoTextureAndSamplerIndex);
      context_id->holdBuffer(bvhLod.indices.getBuf(), bindlessIndicesIndex);
      context_id->holdBuffer(bvhLod.vertices.getBuf(), bindlessVerticesIndex);

      d3d::resource_barrier(ResourceBarrierDesc(bvhLod.indices.getBuf(), bindlessSRVBarrier));
      d3d::resource_barrier(ResourceBarrierDesc(bvhLod.vertices.getBuf(), bindlessSRVBarrier));

      meta.materialType = MeshMeta::bvhMaterialRendinst | MeshMeta::bvhMaterialGrass;
      meta.setIndexBitAndTexcoordFormat(2, VSDT_FLOAT2);
      meta.texcoordNormalColorOffsetAndVertexStride =
        (offsetof(BVHVertex, texcoord) << 24) | (offsetof(BVHVertex, normal) << 16) | 0xFF00U | sizeof(BVHVertex);
      meta.indexAndVertexBufferIndex = (bindlessIndicesIndex << 16) | bindlessVerticesIndex;
      meta.startIndex = 0;
      meta.startVertex = 0;

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
      pack(meta.layerData, layer.info.colors[CHANNEL_BLUE]);

      auto metaIx = grass->getGrassLayerCount() * lodIx + layerIx;
      auto blasHandle = d3d::get_raytrace_acceleration_structure_gpu_handle(bvhLod.blas.get()).handle;
      auto &mapping = bvhConnection.metainfoMappingsCpu[metaIx];

      mapping.blas.x = blasHandle & 0xFFFFFFFFLLU;
      mapping.blas.y = blasHandle >> 32;
      mapping.metaIndex = bvhLod.metaAllocId;

      // Need to fit in 15 bits so there is enough space for the alpha value
      G_ASSERT(bvhLod.metaAllocId < (1 << 15));
    }
  }

  bvhConnection.metainfoMappings->updateData(0, bvhConnection.metainfoMappingsCpu.size() * sizeof(RandomGrassBvhMapping),
    bvhConnection.metainfoMappingsCpu.data(), 0);
}

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->has(Features::Grass))
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

void blas_compacted(int layer_ix, int lod_ix, uint64_t new_gpu_address)
{
  auto &mapping = bvhConnection.metainfoMappingsCpu[layers.size() * lod_ix + layer_ix];
  mapping.blas.x = new_gpu_address & 0xFFFFFFFFLLU;
  mapping.blas.y = new_gpu_address >> 32;

  bvhConnection.metainfoMappings->updateData(0, bvhConnection.metainfoMappingsCpu.size() * sizeof(RandomGrassBvhMapping),
    bvhConnection.metainfoMappingsCpu.data(), 0);
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

void get_memory_statistics(int &vb, int &ib, int &blas, int &meta, int &queries)
{
  vb = ib = blas = meta = queries = 0;
  for (auto &layer : layers)
    for (auto &lod : layer.lods)
    {
      vb += lod.vertices->getElementSize() * lod.vertices->getNumElements();
      ib += lod.indices->getElementSize() * lod.indices->getNumElements();
      blas += d3d::get_raytrace_acceleration_structure_size(lod.blas.get());
    }
  if (bvhConnection.instances)
    queries = bvhConnection.instances->getElementSize() * bvhConnection.instances->getNumElements();
  if (bvhConnection.metainfoMappings)
    meta = bvhConnection.metainfoMappings->getElementSize() * bvhConnection.metainfoMappings->getNumElements();
}

} // namespace grass

} // namespace bvh