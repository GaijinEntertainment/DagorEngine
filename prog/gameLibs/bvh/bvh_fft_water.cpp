// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include "bvh_context.h"
#include "bvh_tools.h"
#include <fftWater/fftWater.h>
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_lockSbuffer.h>

namespace bvh
{
Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);
}

namespace bvh::fftwater
{
static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;
static constexpr int water_cell_size = 32;
static constexpr int water_cell_vertex_count = (water_cell_size + 1) * (water_cell_size + 1);
static constexpr int water_cell_index_count = water_cell_size * water_cell_size * 6;
static constexpr int water_cell_triangle_count = water_cell_size * water_cell_size * 2;
static constexpr int WATER_AREA = 8192;

struct WaterVertexData
{
  Point3 postition;
  E3DCOLOR normal;
};

void init() {}

void teardown() {}

static bool create_blas_and_meta(ContextId context_id, Context::WaterPatches &water_patch, bool ignore_normals)
{
  RaytraceGeometryDescription desc;
  memset(&desc, 0, sizeof(desc));
  desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
  desc.data.triangles.vertexBuffer = water_patch.vertexBuffer.getBuf();
  desc.data.triangles.vertexCount = water_patch.vertexCount;
  desc.data.triangles.vertexStride = sizeof(WaterVertexData);
  desc.data.triangles.vertexFormat = VSDT_FLOAT3;
  desc.data.triangles.vertexOffset = 0;
  desc.data.triangles.indexBuffer = water_patch.indexBuffer.getBuf();
  desc.data.triangles.indexCount = water_patch.triangleCount * 3;
  desc.data.triangles.indexOffset = 0;
  desc.data.triangles.flags = RaytraceGeometryDescription::Flags::IS_OPAQUE;

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

  water_patch.blas = eastl::move(blas);

  G_ASSERT(water_patch.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID);
  water_patch.metaAllocId = context_id->allocateMetaRegion(1);

  MeshMeta &meta = context_id->meshMetaAllocator.get(water_patch.metaAllocId)[0];

  meta.markInitialized();
  meta.setIndexBit(2);
  meta.materialType |= MeshMeta::bvhMaterialWater;
  meta.texcoordOffset = 0xFF;
  // Ignore normals on flat water to avoid precision issues, the default normal in shader is float3(0,1,0)
  meta.normalOffset = ignore_normals ? 0xFF : sizeof(Point3);
  meta.colorOffset = 0xFF;
  meta.vertexStride = sizeof(WaterVertexData);
  context_id->holdBuffer(water_patch.indexBuffer.getBuf(), water_patch.indexBufferBindless);
  meta.setIndexBufferIndex(water_patch.indexBufferBindless);
  context_id->holdBuffer(water_patch.vertexBuffer.getBuf(), water_patch.vertexBufferBindless);
  meta.setVertexBufferIndex(water_patch.vertexBufferBindless);

  return true;
}

static E3DCOLOR pack_normal(const Point3 &normal) { return e3dcolor(Color3::xyz(normal * 0.5 + 0.5), 0); }

static bool generate_flat_indices(ContextId context_id)
{
  uint16_t ibData[] = {0, 1, 2, 0, 2, 3};
  constexpr int ibDwords = 3;
  static_assert(ibDwords * sizeof(uint32_t) == sizeof(ibData));
  context_id->waterFlatIb =
    dag::buffers::create_persistent_sr_byte_address(ibDwords, "bvh_flat_water_ib", d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(context_id->waterFlatIb, false);
  context_id->waterFlatIb->updateData(0, sizeof(ibData), ibData, VBLOCK_WRITEONLY);
  d3d::resource_barrier(ResourceBarrierDesc(context_id->waterFlatIb.getBuf(), bindlessSRVBarrier));

  return true;
}

static void make_flat_patches(ContextId context_id, const fft_water::WaterHeightmap *heightmap, float water_level, const IBBox2 &box,
  int tile_size, int grid_size)
{
  if (!generate_flat_indices(context_id))
    return;

  auto &flatPatches = context_id->water_patches.push_back();
  flatPatches.triangleCount = 2;
  flatPatches.indexBuffer = context_id->waterFlatIb;

  constexpr int VERTEX_COUNT = 4;
  E3DCOLOR up = pack_normal(Point3(0, 1, 0));
  WaterVertexData vbData[4];
  vbData[0] = {Point3(0, water_level, 0), up};
  vbData[1] = {Point3(0, water_level, tile_size), up};
  vbData[2] = {Point3(tile_size, water_level, tile_size), up};
  vbData[3] = {Point3(tile_size, water_level, 0), up};
  flatPatches.vertexCount = VERTEX_COUNT;
  flatPatches.vertexBuffer = dag::buffers::create_persistent_sr_byte_address(divide_up(sizeof(vbData), 4), "bvh_flat_water_vb",
    d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(flatPatches.vertexBuffer, );
  flatPatches.vertexBuffer->updateData(0, sizeof(vbData), vbData, VBLOCK_WRITEONLY);

  create_blas_and_meta(context_id, flatPatches, true);

  dag::Vector<bool> free;
  free.resize(grid_size * grid_size);
  for (int j = 0; j < grid_size; j++)
  {
    for (int i = 0; i < grid_size; i++)
    {
      free[j * grid_size + i] = (!heightmap || heightmap->isFlat(i, j));
    }
  }

  for (int j = 0; j < grid_size; j++)
  {
    for (int i = 0; i < grid_size; i++)
    {
      if (!free[j * grid_size + i])
        continue;

      IBBox2 extent; // Inclusive bbox!
      extent.add(i, j);
      free[j * grid_size + i] = false;

      bool canExpandRight = true;
      bool canExpandDown = true;

      while (canExpandRight || canExpandDown)
      {
        if (canExpandRight)
        {
          int x = extent.right() + 1;
          if (x >= grid_size)
            canExpandRight = false;
          else
          {
            for (int y = extent.top(); y <= extent.bottom(); y++)
            {
              if (!free[y * grid_size + x])
              {
                canExpandRight = false;
                break;
              }
            }
            if (canExpandRight)
            {
              for (int y = extent.top(); y <= extent.bottom(); y++)
                free[y * grid_size + x] = false;
              extent.lim[1].x++;
            }
          }
        }
        if (canExpandDown)
        {
          int y = extent.bottom() + 1;
          if (y >= grid_size)
            canExpandDown = false;
          else
          {
            for (int x = extent.left(); x <= extent.right(); x++)
            {
              if (!free[y * grid_size + x])
              {
                canExpandDown = false;
                break;
              }
            }
            if (canExpandDown)
            {
              for (int x = extent.left(); x <= extent.right(); x++)
                free[y * grid_size + x] = false;
              extent.lim[1].y++;
            }
          }
        }
      }
      Point2 scale = point2(extent.size()) + Point2(1, 1);
      flatPatches.instances.push_back({Point2(i * tile_size + box.left(), j * tile_size + box.top()), scale});
    }
  }

  auto makeExtenderInstance = [&](const Point2 &start, const Point2 &end) {
    flatPatches.instances.push_back({start, (end - start) / tile_size});
  };
  IBBox2 targetBox = IBBox2(IPoint2(-WATER_AREA, -WATER_AREA), IPoint2(WATER_AREA, WATER_AREA));
  if (targetBox.top() < box.top())
  {
    Point2 start = Point2(targetBox.left(), targetBox.top());
    Point2 end = Point2(box.right(), box.top());
    makeExtenderInstance(start, end);
  }
  if (targetBox.right() > box.right())
  {
    Point2 start = Point2(box.right(), targetBox.top());
    Point2 end = Point2(targetBox.right(), box.bottom());
    makeExtenderInstance(start, end);
  }
  if (targetBox.bottom() > box.bottom())
  {
    Point2 start = Point2(box.left(), box.bottom());
    Point2 end = Point2(targetBox.right(), targetBox.bottom());
    makeExtenderInstance(start, end);
  }
  if (targetBox.left() < box.left())
  {
    Point2 start = Point2(targetBox.left(), box.top());
    Point2 end = Point2(box.left(), targetBox.bottom());
    makeExtenderInstance(start, end);
  }
}

static bool generate_heightmap_indices(ContextId context_id)
{
  context_id->waterHeightIb = dag::buffers::create_persistent_sr_byte_address(divide_up(water_cell_index_count, 2),
    "bvh_water_heightmap_ib", d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(context_id->waterHeightIb, false);

  auto upload = lock_sbuffer<uint16_t>(context_id->waterHeightIb.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  HANDLE_LOST_DEVICE_STATE(upload, false);

  for (int z = 0; z < water_cell_size; ++z)
    for (int x = 0; x < water_cell_size; ++x)
    {
      int ix = (z * water_cell_size + x) * 6;
      int base = z * (water_cell_size + 1) + x;

      upload[ix++] = base;
      upload[ix++] = base + water_cell_size + 1;
      upload[ix++] = base + 1;

      upload[ix++] = base + 1;
      upload[ix++] = base + water_cell_size + 1;
      upload[ix++] = base + water_cell_size + 1 + 1;
    }
  upload.close();

  d3d::resource_barrier(ResourceBarrierDesc(context_id->waterHeightIb.getBuf(), bindlessSRVBarrier));

  return true;
}

static void make_heightmap_patches(ContextId context_id, const fft_water::WaterHeightmap *heightmap, float water_level,
  const IBBox2 &box, int tile_size, int grid_size)
{
  if (!generate_heightmap_indices(context_id))
    return;

  for (int j = 0; j < grid_size; j++)
  {
    for (int i = 0; i < grid_size; i++)
    {
      if (heightmap->isFlat(i, j))
        continue;

      auto &patch = context_id->water_patches.push_back();

      patch.triangleCount = water_cell_triangle_count;
      patch.indexBuffer = context_id->waterHeightIb;
      patch.vertexCount = water_cell_vertex_count;
      static int counter = 0;
      patch.vertexBuffer = dag::buffers::create_persistent_sr_byte_address(divide_up(sizeof(WaterVertexData) * patch.vertexCount, 4),
        String(32, "bvh_flat_water_vb_%d", counter++), d3d::buffers::Init::No, RESTAG_BVH);
      HANDLE_LOST_DEVICE_STATE(patch.vertexBuffer, );

      auto scratch = lock_sbuffer<WaterVertexData>(patch.vertexBuffer.getBuf(), 0, water_cell_vertex_count, VBLOCK_WRITEONLY);
      HANDLE_LOST_DEVICE_STATE(scratch, );
      IPoint2 tileStart = {box.left() + tile_size * i, box.top() + tile_size * j};
      G_ASSERT(tile_size % water_cell_size == 0);
      int step = tile_size / water_cell_size;
      for (int z = 0; z <= water_cell_size; ++z)
      {
        for (int x = 0; x <= water_cell_size; ++x)
        {
          Point3 location = Point3(tileStart.x + x * step, water_level, tileStart.y + z * step);
          heightmap->getHeightmapDataBilinear(location.x, location.z, location.y);

          Point3 locationRight = Point3::xVz(location, water_level) + Point3(0.5, 0, 0);
          heightmap->getHeightmapDataBilinear(locationRight.x, locationRight.z, locationRight.y);
          Point3 locationDown = Point3::xVz(location, water_level) + Point3(0, 0, 0.5);
          heightmap->getHeightmapDataBilinear(locationDown.x, locationDown.z, locationDown.y);
          Point3 right = normalize(locationRight - location);
          Point3 down = normalize(locationDown - location);
          Point3 normal = normalize(cross(down, right));

          WaterVertexData &v = scratch[z * (water_cell_size + 1) + x];
          v.postition = location;
          v.normal = pack_normal(normal);
        }
      }
      scratch.close();

      create_blas_and_meta(context_id, patch, false);
      patch.instances.push_back({Point2(0, 0), Point2(1, 1)});
    }
  }
}

static bool validate_water_area(ContextId context_id, int tile_size)
{
  int areaSum = 0;
  for (auto &patch : context_id->water_patches)
  {
    for (auto &instance : patch.instances)
    {
      int area = int(instance.scale.x) * int(instance.scale.y);
      areaSum += area;
    }
  }
  areaSum *= tile_size * tile_size;
  constexpr int targetArea = WATER_AREA * WATER_AREA * 4;
  return areaSum == targetArea;
}

void create_patches(ContextId context_id, FFTWater &fft_water)
{
  if (!context_id->has(Features::FftWater))
    return;

  float waterLevel = fft_water::get_level(&fft_water);

  if (auto heightmap = fft_water::get_heightmap(&fft_water))
  {
    Point2 size = round(Point2(1.0 / heightmap->tcOffsetScale.z, 1.0 / heightmap->tcOffsetScale.w));
    Point2 topleft = round(mul(Point2(-heightmap->tcOffsetScale.x, -heightmap->tcOffsetScale.y), size));
    Point2 bottomright = round(topleft + size);
    Point2 pixelSize = round(size / heightmap->gridSize);
    G_ASSERT(abs(pixelSize.x - pixelSize.y) < 0.01);
    auto box = IBBox2(ipoint2(topleft), ipoint2(bottomright));
    make_flat_patches(context_id, heightmap, waterLevel, box, pixelSize.x, heightmap->gridSize);
    make_heightmap_patches(context_id, heightmap, waterLevel, box, pixelSize.x, heightmap->gridSize);
    G_ASSERT(validate_water_area(context_id, pixelSize.x));
  }
  else
  {
    IBBox2 box = IBBox2({-WATER_AREA, -WATER_AREA}, {WATER_AREA, WATER_AREA});
    constexpr int tileSize = 2 * WATER_AREA;
    constexpr int gridSize = 1;
    make_flat_patches(context_id, nullptr, waterLevel, box, tileSize, gridSize);
    G_ASSERT(validate_water_area(context_id, tileSize));
  }
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::FftWater))
    return;

  for (auto &patch : context_id->water_patches)
  {
    context_id->releaseBuffer(patch.indexBuffer.getBuf());
    context_id->releaseBuffer(patch.vertexBuffer.getBuf());
    context_id->freeMetaRegion(patch.metaAllocId);
  }
  context_id->waterFlatIb.close();
  context_id->waterHeightIb.close();
  context_id->water_patches.clear();
  context_id->water_patches.shrink_to_fit();
}

} // namespace bvh::fftwater