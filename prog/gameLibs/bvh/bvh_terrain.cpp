// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>

#include "bvh_context.h"

int bvh_terrain_lod_count = 6;
bool bvh_terrain_lock = false;

namespace bvh
{
Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);
}

namespace bvh::terrain
{

static UniqueBuf indices;
static uint32_t indices_bindless_slot = -1;

static constexpr int terrain_cell_size = 100;
static constexpr int terrain_cell_vertex_count = (terrain_cell_size + 1) * (terrain_cell_size + 1);
static constexpr int terrain_cell_index_count = terrain_cell_size * terrain_cell_size * 6;

static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::ALLOW_UPDATE;

struct TerrainVertexPosition
{
  Point3 position;
};

struct TerrainVertexPositionNormal
{
  Point3 position;
  uint32_t normal;
};

template <bool embed_normals>
using TerrainVertex = eastl::type_select_t<embed_normals, TerrainVertexPositionNormal, TerrainVertexPosition>;

static constexpr int terrain_thread_count = 8;

void remove_patches(ContextId context_id)
{
  for (auto &lod : context_id->terrainLods)
  {
    for (auto &patch : lod.patches)
      patch.teardown(context_id);
    lod.patches.clear();
  }
}

static bool generate_indices()
{
  indices = dag::buffers::create_persistent_sr_byte_address(divide_up(terrain_cell_index_count, 2), "bvh_terrain_indices");
  HANDLE_LOST_DEVICE_STATE(indices, false);

  auto upload = lock_sbuffer<uint16_t>(indices.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  HANDLE_LOST_DEVICE_STATE(upload, false);

  for (int z = 0; z < terrain_cell_size; ++z)
    for (int x = 0; x < terrain_cell_size; ++x)
    {
      int ix = (z * terrain_cell_size + x) * 6;
      int base = z * (terrain_cell_size + 1) + x;

      upload[ix++] = base;
      upload[ix++] = base + terrain_cell_size + 1;
      upload[ix++] = base + 1;

      upload[ix++] = base + 1;
      upload[ix++] = base + terrain_cell_size + 1;
      upload[ix++] = base + terrain_cell_size + 1 + 1;
    }
  upload.close();

  d3d::resource_barrier(ResourceBarrierDesc(indices.getBuf(), bindlessSRVBarrier));

  return true;
}

template <bool has_context, bool embed_normals>
bool generate_patch_buffer(ContextId context_id, UniqueBVHBuffer &vertices, void *scratch, Point2 origin, int cell_size,
  bool scratch_ready)
{
  using TerrainVertex = ::bvh::terrain::TerrainVertex<embed_normals>;

  if (!scratch_ready)
  {
    if constexpr (has_context)
    {
      context_id->heightProvider->getHeight(scratch, origin, cell_size, terrain_cell_size);
    }
    else
    {
      TerrainVertex *scratchT = (TerrainVertex *)scratch;

      for (int z = 0; z <= terrain_cell_size; ++z)
        for (int x = 0; x <= terrain_cell_size; ++x)
        {
          Point2 loc(x * cell_size, z * cell_size);

          TerrainVertex &v = scratchT[z * (terrain_cell_size + 1) + x];
          v.position.x = loc.x;
          v.position.z = loc.y;

          if constexpr (embed_normals)
          {
            v.position.y = 0;
            v.normal = 0;
          }
          else
          {
            v.position.y = 0;
          }
        }
    }
  }

  static uint32_t vb_gen = 0;
  String vbName = String(64, "bvh_terrain_patch_vertices_%u", ++vb_gen);
  if (!vertices)
  {
    vertices.reset(
      d3d::buffers::create_persistent_sr_byte_address(divide_up(sizeof(TerrainVertex) * terrain_cell_vertex_count, 4), vbName));
    HANDLE_LOST_DEVICE_STATE(vertices, false);
  }

  if (auto upload = lock_sbuffer<TerrainVertex>(vertices.get(), 0, terrain_cell_vertex_count, VBLOCK_WRITEONLY))
    memcpy(upload.get(), scratch, sizeof(TerrainVertex) * terrain_cell_vertex_count);
  else
    G_ASSERT_RETURN(false, false);

  d3d::resource_barrier(ResourceBarrierDesc(vertices.get(), bindlessSRVBarrier));

  return true;
}

template <bool embed_normals>
RaytraceGeometryDescription get_patch_blas_desc(Sbuffer *vertices)
{
  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = vertices;
  desc.data.triangles.indexBuffer = indices.getBuf();
  desc.data.triangles.vertexCount = terrain_cell_vertex_count;
  desc.data.triangles.vertexStride = sizeof(TerrainVertex<embed_normals>);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.indexCount = terrain_cell_index_count;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::IS_OPAQUE;
  return desc;
}

template <bool embed_normals>
static UniqueBLAS create_patch_blas(Sbuffer *vertices)
{
  auto desc = get_patch_blas_desc<embed_normals>(vertices);
  return UniqueBLAS::create(&desc, 1, blas_flags);
}

template <bool embed_normals>
static bool generate_patch_template(int cell_size, TerrainPatch &patch, TerrainVertex<embed_normals> *scratch)
{
  patch.position = Point2::ZERO;

  if (!generate_patch_buffer<false, embed_normals>(nullptr, patch.vertices, scratch, Point2::ZERO, cell_size, false))
    return false;

  patch.blas = create_patch_blas<embed_normals>(patch.vertices.get());
  HANDLE_LOST_DEVICE_STATE(patch.blas, false);

  auto patch_desc = get_patch_blas_desc<embed_normals>(patch.vertices.get());

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.geometryDesc = &patch_desc;
  buildInfo.geometryDescCount = 1;
  buildInfo.flags = blas_flags;
  buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(patch.blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = patch.blas.getBuildScratchSize();

  d3d::build_bottom_acceleration_structure(patch.blas.get(), buildInfo);

  // build_bottom_acceleration_structure is not flushing the BLAS, so we flush it here, before cloning it.
  d3d::resource_barrier(ResourceBarrierDesc(patch.blas.get()));

  return true;
}

template <bool embed_normals>
static bool instantiate_patch(ContextId context_id, TerrainPatch &patch, void *scratch, Point2 origin, int cell_size)
{
  CHECK_LOST_DEVICE_STATE_RET(false);

  patch.position = origin;

  if (!generate_patch_buffer<true, embed_normals>(context_id, patch.vertices, scratch, origin, cell_size, true))
    return false;

  auto patch_desc = get_patch_blas_desc<embed_normals>(patch.vertices.get());

  if (!patch.blas)
  {
    patch.blas = create_patch_blas<embed_normals>(patch.vertices.get());
    HANDLE_LOST_DEVICE_STATE(patch.blas, false);
    d3d::copy_raytrace_acceleration_structure(patch.blas.get(), context_id->terrainPatchTemplate.blas.get());
  }

  raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
  buildInfo.geometryDesc = &patch_desc;
  buildInfo.geometryDescCount = 1;
  buildInfo.flags = blas_flags;
  buildInfo.doUpdate = true;
  buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(patch.blas.getUpdateScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.scratchSpaceBufferSizeInBytes = patch.blas.getUpdateScratchSize();

  d3d::build_bottom_acceleration_structure(patch.blas.get(), buildInfo);

  if (patch.metaAllocId < 0)
  {
    G_ASSERT(indices_bindless_slot != -1);

    patch.metaAllocId = context_id->allocateMetaRegion();
    auto &meta = context_id->meshMetaAllocator.get(patch.metaAllocId);

    meta.markInitialized();

    uint32_t bindlessVerticesIndex;
    context_id->holdBuffer(patch.vertices.get(), bindlessVerticesIndex);

    meta.materialType = MeshMeta::bvhMaterialTerrain;
    meta.indexAndVertexBufferIndex = (indices_bindless_slot << 16) | bindlessVerticesIndex;
  }

  return true;
}

struct TerrainBVHJob : public cpujobs::IJob
{
  enum State
  {
    Idle,
    Start2,
    Start4,
    Start6,
    Start8,
    Done
  } state = Idle;

  ContextId contextId;
  int lodIx;
  Point2 leftBottomOrigin;
  int lodGridSize;

  dag::Vector<dag::Vector<uint8_t>> scratches;

  void doJob()
  {
    TIME_PROFILE(TerrainBVHJob);

    bool normals = contextId->heightProvider->embedNormals();

    if (scratches.empty())
    {
      scratches.resize(lodIx == 0 ? 9 : 8);
      for (auto &scratch : scratches)
        scratch.resize(terrain_cell_vertex_count * (normals ? sizeof(TerrainVertexPositionNormal) : sizeof(TerrainVertexPosition)));
    }

    int lodCellSize = terrain_cell_size * lodGridSize;

    switch (state)
    {
      case Idle:
      case Done: break;
      case Start2:
        contextId->heightProvider->getHeight(scratches[0].data(), leftBottomOrigin + Point2(0, 0) * lodCellSize, lodGridSize,
          terrain_cell_size);
        contextId->heightProvider->getHeight(scratches[1].data(), leftBottomOrigin + Point2(1, 0) * lodCellSize, lodGridSize,
          terrain_cell_size);
        state = State(state + 1);
        break;
      case Start4:
        contextId->heightProvider->getHeight(scratches[2].data(), leftBottomOrigin + Point2(2, 0) * lodCellSize, lodGridSize,
          terrain_cell_size);
        contextId->heightProvider->getHeight(scratches[3].data(), leftBottomOrigin + Point2(0, 1) * lodCellSize, lodGridSize,
          terrain_cell_size);
        state = State(state + 1);
        break;
      case Start6:
        contextId->heightProvider->getHeight(scratches[4].data(), leftBottomOrigin + Point2(2, 1) * lodCellSize, lodGridSize,
          terrain_cell_size);
        contextId->heightProvider->getHeight(scratches[5].data(), leftBottomOrigin + Point2(0, 2) * lodCellSize, lodGridSize,
          terrain_cell_size);
        state = State(state + 1);
        break;
      case Start8:
        contextId->heightProvider->getHeight(scratches[6].data(), leftBottomOrigin + Point2(1, 2) * lodCellSize, lodGridSize,
          terrain_cell_size);
        contextId->heightProvider->getHeight(scratches[7].data(), leftBottomOrigin + Point2(2, 2) * lodCellSize, lodGridSize,
          terrain_cell_size);

        // The middle one is all the lower lods, except for the lowest one
        if (lodIx == 0)
          contextId->heightProvider->getHeight(scratches[8].data(), leftBottomOrigin + Point2(1, 1) * lodCellSize, lodGridSize,
            terrain_cell_size);

        state = State(state + 1);
        break;
    }
  }
} terrain_bvh_jobs[terrain_thread_count];

template <bool embed_normals>
static void update_terrain(ContextId context_id, const Point2 &location)
{
  // TODO: LOD borders vertices can have holes

  bool isWorking = false;

  Point2 leftBottomOrigin = context_id->terrainMiddlePoint - Point2(3 * terrain_cell_size / 2, 3 * terrain_cell_size / 2);
  int lodGridSize = 1;

  int li = 0;
  for (auto &lod : context_id->terrainLods)
  {
    int lodIx = li++;
    threadpool::wait(&terrain_bvh_jobs[lodIx]);

    auto &job = terrain_bvh_jobs[lodIx];
    if (job.state == TerrainBVHJob::Done)
    {
      lod.patches.resize(lodIx == 0 ? 9 : 8);

      bool normals = context_id->heightProvider->embedNormals();

      int lodCellSize = terrain_cell_size * lodGridSize;

      if (!normals)
      {
        instantiate_patch<false>(context_id, lod.patches[0], job.scratches[0].data(), leftBottomOrigin + Point2(0, 0) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[1], job.scratches[1].data(), leftBottomOrigin + Point2(1, 0) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[2], job.scratches[2].data(), leftBottomOrigin + Point2(2, 0) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[3], job.scratches[3].data(), leftBottomOrigin + Point2(0, 1) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[4], job.scratches[4].data(), leftBottomOrigin + Point2(2, 1) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[5], job.scratches[5].data(), leftBottomOrigin + Point2(0, 2) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[6], job.scratches[6].data(), leftBottomOrigin + Point2(1, 2) * lodCellSize,
          lodGridSize);
        instantiate_patch<false>(context_id, lod.patches[7], job.scratches[7].data(), leftBottomOrigin + Point2(2, 2) * lodCellSize,
          lodGridSize);
      }
      else
      {
        instantiate_patch<true>(context_id, lod.patches[0], job.scratches[0].data(), leftBottomOrigin + Point2(0, 0) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[1], job.scratches[1].data(), leftBottomOrigin + Point2(1, 0) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[2], job.scratches[2].data(), leftBottomOrigin + Point2(2, 0) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[3], job.scratches[3].data(), leftBottomOrigin + Point2(0, 1) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[4], job.scratches[4].data(), leftBottomOrigin + Point2(2, 1) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[5], job.scratches[5].data(), leftBottomOrigin + Point2(0, 2) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[6], job.scratches[6].data(), leftBottomOrigin + Point2(1, 2) * lodCellSize,
          lodGridSize);
        instantiate_patch<true>(context_id, lod.patches[7], job.scratches[7].data(), leftBottomOrigin + Point2(2, 2) * lodCellSize,
          lodGridSize);
      }

      CHECK_LOST_DEVICE_STATE();

      if (lodIx == 0)
      {
        if (!normals)
          instantiate_patch<false>(context_id, lod.patches[8], job.scratches[8].data(), leftBottomOrigin + Point2(1, 1) * lodCellSize,
            lodGridSize);
        else
          instantiate_patch<true>(context_id, lod.patches[8], job.scratches[8].data(), leftBottomOrigin + Point2(1, 1) * lodCellSize,
            lodGridSize);
      }

      job.state = TerrainBVHJob::Idle;
    }
    else if (job.state != TerrainBVHJob::Idle)
    {
      threadpool::add(&terrain_bvh_jobs[lodIx]);
      isWorking = true;
    }

    lodGridSize *= 3;
    leftBottomOrigin.y -= lodGridSize * terrain_cell_size;
    leftBottomOrigin.x -= lodGridSize * terrain_cell_size;
  }

  if (isWorking)
    return;

  auto distance = location - context_id->terrainMiddlePoint;
  if (abs(distance.x) <= terrain_cell_size / 2 && abs(distance.y) <= terrain_cell_size / 2)
    return;

  context_id->terrainMiddlePoint = location;
  context_id->terrainMiddlePoint.x = round(context_id->terrainMiddlePoint.x / terrain_cell_size) * terrain_cell_size;
  context_id->terrainMiddlePoint.y = round(context_id->terrainMiddlePoint.y / terrain_cell_size) * terrain_cell_size;

  lodGridSize = 1;
  leftBottomOrigin = context_id->terrainMiddlePoint - Point2(3 * terrain_cell_size / 2, 3 * terrain_cell_size / 2);
  for (int lodIx = 0; lodIx < context_id->terrainLods.size(); ++lodIx)
  {
    auto &job = terrain_bvh_jobs[lodIx];
    job.contextId = context_id;
    job.lodIx = lodIx;
    job.leftBottomOrigin = leftBottomOrigin;
    job.lodGridSize = lodGridSize;
    job.state = TerrainBVHJob::Start2;
    threadpool::add(&job);

    lodGridSize *= 3;
    leftBottomOrigin.y -= lodGridSize * terrain_cell_size;
    leftBottomOrigin.x -= lodGridSize * terrain_cell_size;
  }
}

void init() { generate_indices(); }

void init(ContextId context_id)
{
  if (context_id->has(Features::Terrain))
  {
    G_ASSERT(indices && indices_bindless_slot == -1);
    context_id->terrainLods.resize(bvh_terrain_lod_count);
    context_id->holdBuffer(indices.getBuf(), indices_bindless_slot);
  }
}

void teardown() { indices.close(); }

void teardown(ContextId context_id)
{
  remove_patches(context_id);
  context_id->terrainPatchTemplate.~TerrainPatch();

  G_ASSERT(indices_bindless_slot != -1);
  G_VERIFY(context_id->releaseBuffer(indices.getBuf()));
  indices_bindless_slot = -1;
}

dag::Vector<eastl::tuple<uint64_t, uint32_t, Point2>> get_blases(ContextId context_id)
{
  TIME_PROFILE(terrain_get_blases);

  Point2 leftBottomOrigin = context_id->terrainMiddlePoint - Point2(3 * terrain_cell_size / 2, 3 * terrain_cell_size / 2);
  int lodGridSize = 1;

  dag::Vector<eastl::tuple<uint64_t, uint32_t, Point2>> blases;
  for (int lodIx = 0; lodIx < context_id->terrainLods.size(); ++lodIx)
  {
    for (auto &patch : context_id->terrainLods[lodIx].patches)
      blases.push_back({patch.blas.getGPUAddress(), patch.metaAllocId, patch.position});

    lodGridSize *= 3;
    leftBottomOrigin.y -= lodGridSize * terrain_cell_size;
    leftBottomOrigin.x -= lodGridSize * terrain_cell_size;
  }
  return blases;
}

} // namespace bvh::terrain

namespace bvh
{

void add_terrain(ContextId context_id, HeightProvider *height_provider)
{
  if (!context_id->has(Features::Terrain))
    return;

  if (context_id->heightProvider != height_provider)
    terrain::remove_patches(context_id);
  context_id->heightProvider = height_provider;
}

void remove_terrain(ContextId context_id)
{
  for (int lodIx = 0; lodIx < context_id->terrainLods.size(); ++lodIx)
  {
    auto &job = terrain::terrain_bvh_jobs[lodIx];
    threadpool::wait(&job);
    job.state = terrain::TerrainBVHJob::Idle;
    job.scratches.clear();
  }

  add_terrain(context_id, nullptr);
}

void update_terrain(ContextId context_id, const Point2 &location)
{
  if (!context_id || !context_id->heightProvider)
    return;

  if (!context_id->has(Features::Terrain))
    return;

  if (bvh_terrain_lod_count != context_id->terrainLods.size())
  {
    terrain::remove_patches(context_id);
    context_id->terrainLods.clear();
    context_id->terrainLods.resize(bvh_terrain_lod_count);
    context_id->terrainMiddlePoint += Point2(terrain::terrain_cell_size, terrain::terrain_cell_size);
  }

  if (!context_id->terrainPatchTemplate.blas)
  {
    if (context_id->heightProvider->embedNormals())
    {
      dag::Vector<terrain::TerrainVertex<true>, framemem_allocator> scratch(terrain::terrain_cell_vertex_count);
      terrain::generate_patch_template<true>(terrain::terrain_cell_size, context_id->terrainPatchTemplate, scratch.data());
    }
    else
    {
      dag::Vector<terrain::TerrainVertex<false>, framemem_allocator> scratch(terrain::terrain_cell_vertex_count);
      terrain::generate_patch_template<false>(terrain::terrain_cell_size, context_id->terrainPatchTemplate, scratch.data());
    }
  }

  CHECK_LOST_DEVICE_STATE();

  if (bvh_terrain_lock)
    return;

  if (context_id->heightProvider->embedNormals())
    terrain::update_terrain<true>(context_id, location);
  else
    terrain::update_terrain<false>(context_id, location);
}

} // namespace bvh
