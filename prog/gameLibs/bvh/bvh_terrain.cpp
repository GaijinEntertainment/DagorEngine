// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>

#include "bvh_context.h"

#if _TARGET_PC
int bvh_terrain_lod_count = 6;
#else
int bvh_terrain_lod_count = 5;
#endif

bool bvh_terrain_lock = false;

namespace bvh::terrain
{

static UniqueBuf indices;
static uint32_t indices_bindless_slot = -1;

static constexpr int terrain_cell_size = 100;
static constexpr int terrain_cell_vertex_count = (terrain_cell_size + 1) * (terrain_cell_size + 1);
static constexpr int terrain_cell_index_count = terrain_cell_size * terrain_cell_size * 6;

static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::ALLOW_UPDATE | RaytraceBuildFlags::LOW_MEMORY;

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
using TerrainVertex = eastl::conditional_t<embed_normals, TerrainVertexPositionNormal, TerrainVertexPosition>;

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

void init()
{
  indices = dag::buffers::create_persistent_sr_byte_address(divide_up(terrain_cell_index_count, 2), "bvh_terrain_indices",
    d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(indices, );

  auto upload = lock_sbuffer<uint16_t>(indices.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  HANDLE_LOST_DEVICE_STATE(upload, );

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
}

template <bool embed_normals>
bool generate_patch_buffer(UniqueBVHBuffer &vertices, void *scratch)
{
  using TerrainVertex = ::bvh::terrain::TerrainVertex<embed_normals>;
  if (!vertices)
  {
    static uint32_t vb_gen = 0;
    String vbName = String(64, "bvh_terrain_patch_vertices_%u", ++vb_gen);
    vertices.reset(d3d::buffers::create_persistent_sr_byte_address(divide_up(sizeof(TerrainVertex) * terrain_cell_vertex_count, 4),
      vbName, d3d::buffers::Init::No, RESTAG_BVH));
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
static void generate_patch_template(TerrainPatch &patch)
{
  dag::Vector<TerrainVertex<embed_normals>, framemem_allocator> scratch(terrain_cell_vertex_count);
  for (int z = 0; z <= terrain_cell_size; ++z)
    for (int x = 0; x <= terrain_cell_size; ++x)
    {
      auto &v = scratch[z * (terrain_cell_size + 1) + x];
      v.position = Point3(x * terrain_cell_size, 0, z * terrain_cell_size);
      if constexpr (embed_normals)
        v.normal = 0;
    }

  if (!generate_patch_buffer<embed_normals>(patch.vertices, scratch.data()))
    return;

  auto patch_desc = get_patch_blas_desc<embed_normals>(patch.vertices.get());
  patch.blas = build_blas(patch_desc, blas_flags);
  if (!patch.blas)
    return;

  // build_bottom_acceleration_structure is not flushing the BLAS, so we flush it here, before cloning it.
  d3d::resource_barrier(ResourceBarrierDesc(patch.blas.get()));
}

template <bool embed_normals>
static void instantiate_patch(ContextId context_id, TerrainPatch &patch, void *scratch, Point2 origin)
{
  CHECK_LOST_DEVICE_STATE();

  patch.position = origin;

  if (!generate_patch_buffer<embed_normals>(patch.vertices, scratch))
    return;

  auto patch_desc = get_patch_blas_desc<embed_normals>(patch.vertices.get());

  if (!patch.blas)
  {
    patch.blas = create_patch_blas<embed_normals>(patch.vertices.get());
    HANDLE_LOST_DEVICE_STATE(patch.blas, );
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

  if (patch.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
  {
    G_ASSERT(indices_bindless_slot != -1);

    TIME_PROFILE(meta_lock_terrain);
    patch.metaAllocId = context_id->allocateMetaRegion(1, "terrain");
    LockedMetaAccess lockedMeta(*context_id, patch.metaAllocId);
    auto &meta = lockedMeta[0];

    meta.markInitialized();

    uint32_t bindlessVerticesIndex = 0;
    if constexpr (embed_normals)
      context_id->holdBuffer(patch.vertices.get(), bindlessVerticesIndex);

    meta.materialType = MeshMeta::bvhMaterialTerrain;
    meta.setIndexBufferIndex(indices_bindless_slot);
    meta.setVertexBufferIndex(bindlessVerticesIndex);
  }
}

static constexpr Point2 patch_offsets[] = {{0, 0}, {1, 0}, {2, 0}, {0, 1}, {2, 1}, {0, 2}, {1, 2}, {2, 2}, {1, 1}};

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

  const char *getJobName(bool &) const override { return DAPROFILER_STRING("TerrainBVHJob"); }

  void doJob()
  {
    bool normals = contextId->heightProvider->embedNormals();

    if (scratches.empty())
    {
      // The middle one is all the lower lods, except for the lowest one
      scratches.resize(lodIx == 0 ? 9 : 8);
      for (auto &scratch : scratches)
        scratch.resize(terrain_cell_vertex_count * (normals ? sizeof(TerrainVertexPositionNormal) : sizeof(TerrainVertexPosition)));
    }

    if (state == Idle || state == Done)
      return;

    int lodCellSize = terrain_cell_size * lodGridSize;
    int firstPatch = 2 * (state - Start2);
    int patchEnd = state == Start8 ? scratches.size() : firstPatch + 2;
    for (int patchIx = firstPatch; patchIx < patchEnd; ++patchIx)
      contextId->heightProvider->getHeight(scratches[patchIx].data(), leftBottomOrigin + patch_offsets[patchIx] * lodCellSize,
        lodGridSize, terrain_cell_size);
    state = State(state + 1);
  }
} terrain_bvh_jobs[terrain_thread_count];

static void stop_jobs(ContextId context_id)
{
  for (int lodIx = 0; lodIx < context_id->terrainLods.size(); ++lodIx)
  {
    auto &job = terrain_bvh_jobs[lodIx];
    threadpool::wait(&job);
    job.state = TerrainBVHJob::Idle;
    job.scratches.clear();
  }
}

template <bool embed_normals>
static void update_terrain(ContextId context_id, const Point2 &location)
{
  // TODO: LOD borders vertices can have holes

  TIME_PROFILE(bvh::update_terrain);

  bool isWorking = false;

  int li = 0;
  for (auto &lod : context_id->terrainLods)
  {
    int lodIx = li++;

    if (!interlocked_acquire_load(terrain_bvh_jobs[lodIx].done))
    {
      isWorking = true;
      continue;
    }

    auto &job = terrain_bvh_jobs[lodIx];
    if (job.state == TerrainBVHJob::Done)
    {
      lod.patches.resize(job.scratches.size());
      int lodCellSize = terrain_cell_size * job.lodGridSize;
      for (int patchIx = 0; patchIx < lod.patches.size(); ++patchIx)
        instantiate_patch<embed_normals>(context_id, lod.patches[patchIx], job.scratches[patchIx].data(),
          job.leftBottomOrigin + patch_offsets[patchIx] * lodCellSize);

      CHECK_LOST_DEVICE_STATE();

      context_id->terrainDirty = true;

      job.state = TerrainBVHJob::Idle;
    }
    else if (job.state != TerrainBVHJob::Idle)
    {
      threadpool::add(&terrain_bvh_jobs[lodIx]);
      isWorking = true;
    }
  }

  if (isWorking)
    return;

  uint32_t heightVersion = context_id->heightProvider->getVersion();
  auto distance = location - context_id->terrainMiddlePoint;
  if (heightVersion == context_id->terrainHeightVersion && abs(distance.x) <= terrain_cell_size / 2 &&
      abs(distance.y) <= terrain_cell_size / 2)
    return;

  context_id->terrainHeightVersion = heightVersion;
  context_id->terrainMiddlePoint = location;
  context_id->terrainMiddlePoint.x = round(context_id->terrainMiddlePoint.x / terrain_cell_size) * terrain_cell_size;
  context_id->terrainMiddlePoint.y = round(context_id->terrainMiddlePoint.y / terrain_cell_size) * terrain_cell_size;

  int lodGridSize = 1;
  Point2 leftBottomOrigin = context_id->terrainMiddlePoint - Point2(3 * terrain_cell_size / 2, 3 * terrain_cell_size / 2);
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

void init(ContextId context_id)
{
  if (context_id->hasAny(Features::Terrain))
  {
    G_ASSERT(indices && indices_bindless_slot == -1);
    context_id->terrainLods.resize(bvh_terrain_lod_count);
    context_id->holdBuffer(indices.getBuf(), indices_bindless_slot);
  }
}

void teardown() { indices.close(); }

void teardown(ContextId context_id)
{
  if (context_id->hasAny(Features::Terrain))
  {
    remove_patches(context_id);
    // not an explicit destructor call: ~Context destroys the member again later
    context_id->terrainPatchTemplate = TerrainPatch();
    G_ASSERT(indices_bindless_slot != -1);
    G_VERIFY(context_id->releaseBuffer(indices.getBuf()));
    indices_bindless_slot = -1;
  }
}

bool is_dirty(ContextId context_id) { return context_id->terrainDirty; }

dag::Vector<eastl::tuple<uint64_t, MeshMetaAllocator::AllocId, Point2>, framemem_allocator> get_blases(ContextId context_id)
{
  TIME_PROFILE(terrain_get_blases);

  dag::Vector<eastl::tuple<uint64_t, MeshMetaAllocator::AllocId, Point2>, framemem_allocator> blases;
  for (auto &lod : context_id->terrainLods)
    for (auto &patch : lod.patches)
      blases.push_back({patch.blas.getGPUAddress(), patch.metaAllocId, patch.position});
  return blases;
}

} // namespace bvh::terrain

namespace bvh
{

void add_terrain(ContextId context_id, HeightProvider *height_provider)
{
  if (!context_id->hasAny(Features::Terrain))
    return;

  if (context_id->heightProvider != height_provider)
    terrain::remove_patches(context_id);
  context_id->heightProvider = height_provider;
}

void remove_terrain(ContextId context_id)
{
  terrain::stop_jobs(context_id);
  add_terrain(context_id, nullptr);
  context_id->terrainMiddlePoint = Context::noTerrainPoint;
}

void update_terrain(ContextId context_id, const Point2 &location)
{
  if (!context_id || !context_id->heightProvider)
    return;

  if (!context_id->hasAny(Features::Terrain))
    return;

  if (bvh_terrain_lod_count != context_id->terrainLods.size())
  {
    terrain::stop_jobs(context_id);
    terrain::remove_patches(context_id);
    context_id->terrainLods.clear();
    context_id->terrainLods.resize(bvh_terrain_lod_count);
    context_id->terrainMiddlePoint += Point2(terrain::terrain_cell_size, terrain::terrain_cell_size);
  }

  if (!context_id->terrainPatchTemplate.blas)
  {
    if (context_id->heightProvider->embedNormals())
      terrain::generate_patch_template<true>(context_id->terrainPatchTemplate);
    else
      terrain::generate_patch_template<false>(context_id->terrainPatchTemplate);
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
