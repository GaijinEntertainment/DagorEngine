// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_lockSbuffer.h>
#include <shaders/dag_shaders.h>
#include <rendInst/riShaderConstBuffers.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_lockSbuffer.h>
#include <ecs/rendInst/riExtra.h>
#include <ecs/core/entityManager.h>

#include <gpuObjects/volumePlacer.h>
#include <math/dag_hlsl_floatx.h>
#include <gpuObjects/gpu_objects_const.hlsli>

#include <util/dag_console.h>

using namespace ecs;

namespace rendinst::gpuobjects
{
void init_r(); // Defined in rendInst/render/gpuObjects.cpp
}

ECS_DECLARE_RELOCATABLE_TYPE(gpu_objects::riex_handles);
ECS_REGISTER_RELOCATABLE_TYPE(gpu_objects::riex_handles, nullptr);
ECS_AUTO_REGISTER_COMPONENT(gpu_objects::riex_handles, "gpu_object_placer__surface_riex_handles", nullptr, 0);

namespace gpu_objects
{

#define GLOBAL_VARS_LIST                                \
  VAR(gpu_objects_buffer_offset)                        \
  VAR(gpu_objects_distance_emitter_buffer_offset)       \
  VAR(gpu_objects_distance_emitter_decal_buffer_offset) \
  VAR(gpu_objects_matrices_count)                       \
  VAR(gpu_objects_on_rendinst_geometry_matrices_count)  \
  VAR(gpu_objects_on_terrain_geometry_matrices_count)   \
  VAR(gpu_objects_max_on_terrain_instance_count)        \
  VAR(gpu_objects_bbox_y_min_max)                       \
  VAR(gpu_objects_ri_pool_id_offset)                    \
  VAR(gpu_objects_placer_tm)                            \
  VAR(gpu_objects_max_triangles)                        \
  VAR(gpu_objects_mesh_offset)                          \
  VAR(gpu_objects_debug_color)                          \
  VAR(gpu_objects_scale_rotate)                         \
  VAR(gpu_objects_up_vector)                            \
  VAR(gpu_objects_scale_radius)                         \
  VAR(gpu_objects_min_gathered_triangle_size)           \
  VAR(gpu_objects_sum_area_step)                        \
  VAR(gpu_objects_distance_affect_decal)                \
  VAR(gpu_objects_distance_to_scale_from)               \
  VAR(gpu_objects_distance_to_scale_to)                 \
  VAR(gpu_objects_distance_to_min_scale)                \
  VAR(gpu_objects_distance_to_max_scale)                \
  VAR(gpu_objects_distance_to_rotation_from)            \
  VAR(gpu_objects_distance_to_rotation_to)              \
  VAR(gpu_objects_distance_to_min_rotation)             \
  VAR(gpu_objects_distance_to_max_rotation)             \
  VAR(gpu_objects_distance_to_scale_pow)                \
  VAR(gpu_objects_distance_to_rotation_pow)             \
  VAR(gpu_objects_distance_out_of_range)                \
  VAR(gpu_objects_distance_emitter__range)              \
  VAR(gpu_objects_distance_emitter__position)           \
  VAR(gpu_objects_remove_box_min)                       \
  VAR(gpu_objects_remove_box_max)                       \
  VAR(gpu_objects_cutoff_ratio)


#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

struct DistanceEmitterParameters
{
  Point3 position = Point3::ZERO;
  float range = 0.0f;
  float scale_factor = 1.0f;
  float rotation_factor = 1.0f;
};

static eastl::unique_ptr<VolumePlacer> volume_placer_mgr;
VolumePlacer *get_volume_placer_mgr() { return volume_placer_mgr.get(); }
void init_volume_placer_mgr() { volume_placer_mgr.reset(new VolumePlacer()); }
void close_volume_placer_mgr() { volume_placer_mgr.reset(); }


constexpr int MATRIX_SIZE = sizeof(Point4) * ROWS_IN_MATRIX;
constexpr int DISPATCH_INDIRECT_ARGS = 3;
constexpr int DISPATCH_INDIRECT_ARGS_NUM = 2;
constexpr int LAYER_BITS_SHIFT = 16;
constexpr uint32_t RI_IDX_MASK = (1 << LAYER_BITS_SHIFT) - 1;
constexpr int ON_TERRAIN_OBJECT_COUNT_CLAMP = 800;

VolumePlacer::VolumePlacer()
{
  boxPlacer.reset(new_compute_shader("gpu_objects_box_placer_cs", true));
  clearTerrainObjects.reset(new_compute_shader("gpu_objects_clear_terrain_objects_cs", true));
  gatherTriangles.reset(new_compute_shader("gpu_objects_box_placer_gather_triangles_cs", true));
  computeTerrainObjectsCount.reset(new_compute_shader("gpu_objects_compute_terrain_objects_count_cs", true));
  onTerrainGeometryPlacer.reset(new_compute_shader("gpu_objects_on_terrain_geometry_placer_cs", true));
  onRendinstGeometryPlacer.reset(new_compute_shader("gpu_objects_on_rendinst_geometry_placer_cs", true));
  counterBuffer =
    dag::create_sbuffer(sizeof(int), COUNTERS_NUM, SBCF_UA_BYTE_ADDRESS_READBACK | SBCF_BIND_SHADER_RES, 0, "gpu_objects_counter");
  updateMatrices.reset(new_compute_shader("gpu_objects_update_matrices_cs", true));
  counterReadbackFence.reset(d3d::create_event_query());
  objectRemover.reset(new_compute_shader("gpu_objects_remove_objects_cs", true));
  readbackPending = false;
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
}

VolumePlacer::PrefixSumShaders::PrefixSumShaders()
{
  sumSteps.reset(new_compute_shader("gpu_objects_sum_area_cs", true));
  getGroupCount.reset(new_compute_shader("gpu_objects_get_group_count_cs", true));
  groupCountIndirect =
    dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, DISPATCH_INDIRECT_ARGS_NUM, "gpu_objects_group_count");
}

void VolumePlacer::PrefixSumShaders::calculate(Sbuffer *gathered_triangles)
{
  TIME_D3D_PROFILE(VolumePlacer_prefixSum)
  {
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), groupCountIndirect.getBuf());
    getGroupCount->dispatch(1, 1, 1);
  }
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), gathered_triangles);
  ShaderGlobal::set_int(gpu_objects_sum_area_stepVarId, 0);
  sumSteps->dispatch_indirect(groupCountIndirect.getBuf(), 0);
  ShaderGlobal::set_int(gpu_objects_sum_area_stepVarId, 1);
  sumSteps->dispatch(1, 1, 1);
  ShaderGlobal::set_int(gpu_objects_sum_area_stepVarId, 2);
  sumSteps->dispatch_indirect(groupCountIndirect.getBuf(), DISPATCH_INDIRECT_ARGS * sizeof(uint32_t));
}

template <class T>
inline void gpu_object_placer_fill_ecs_query(T b);
template <class T>
inline void gpu_object_placer_copy_on_expand_ecs_query(T b);
template <class T>
inline void gpu_object_placer_visibility_ecs_query(T b);
template <class T>
inline void gpu_object_placer_invalidate_ecs_query(T b);
template <class T>
inline void gpu_object_placer_distance_emitter_update_after_placing_ecs_query(ecs::EntityId id, T b);
template <class T>
inline void gpu_object_placer_distance_emitter_update_volume_placers_ecs_query(T b);
template <class T>
inline void gpu_object_placer_get_current_distance_emitter_position_ecs_query(ecs::EntityId id, T b);
template <class T>
inline void gpu_object_placer_select_closest_distance_emitter_ecs_query(T b);
template <class T>
inline void gpu_object_placer_remove_ecs_query(T b);


static void adjust_transform_to_border_offset(const Point2 *offset, int axis_index, TMatrix &transform)
{
  if (offset == nullptr)
    return;

  const Point2 &offsetV = *offset;
  Point3 axis = transform.getcol(axis_index);
  float axisLength = axis.length();
  float adjustedLength = axisLength - offsetV.x - offsetV.y;
  if (adjustedLength <= 0.0f)
  {
    logerr("GPUObjectsPlacer: box borders are too big for the object. Axis=%@, length=%@, offset=%@, pos=%@", axis_index, axisLength,
      offsetV, transform.getcol(3));
    return;
  }
  transform.setcol(axis_index, axis * (adjustedLength / axisLength));
  float axisTranslation = offsetV.y - offsetV.x;
  transform.setcol(3, transform.getcol(3) + axis * (axisTranslation / axisLength * 0.5f));
}

void VolumePlacer::performPlacing(const Point3 &camera_pos)
{
  TIME_D3D_PROFILE(VolumePlacer_performPlacing)

  if (matricesBuffer.needExpand)
    matricesBuffer.expandBuffer();

  gpu_object_placer_fill_ecs_query(
    [&](ecs::EntityId eid, int gpu_object_placer__ri_asset_idx, float gpu_object_placer__visible_distance_squared,
      bool gpu_object_placer__place_on_geometry, float gpu_object_placer__min_gathered_triangle_size,
      float gpu_object_placer__triangle_edge_length_ratio_cutoff, float &gpu_object_placer__current_distance_squared,
      bool &gpu_object_placer__filled, int &gpu_object_placer__buffer_offset,
      int &gpu_object_placer__distance_emitter_decal_buffer_size, int &gpu_object_placer__distance_emitter_buffer_size,
      int &gpu_object_placer__buffer_size, int &gpu_object_placer__on_rendinst_geometry_count,
      int &gpu_object_placer__on_terrain_geometry_count, float gpu_object_placer__object_density,
      int gpu_object_placer__object_max_count, const Point2 &gpu_object_placer__object_scale_range,
      const Point2 &gpu_object_placer__distance_based_scale, float gpu_object_placer__min_scale_radius,
      const Point4 &gpu_object_placer__object_up_vector_threshold, bool &gpu_object_placer__distance_emitter_is_dirty,
      const ecs::EntityId gpu_object_placer__distance_emitter_eid, const Point3 &gpu_object_placer__distance_to_scale_from,
      const Point3 &gpu_object_placer__distance_to_scale_to, float gpu_object_placer__distance_to_rotation_from,
      float gpu_object_placer__distance_to_rotation_to, const Point3 &gpu_object_placer__distance_to_scale_pow,
      float gpu_object_placer__distance_to_rotation_pow, bool gpu_object_placer__use_distance_emitter,
      bool gpu_object_placer__distance_affect_decals, bool gpu_object_placer__distance_out_of_range,
      gpu_objects::riex_handles &gpu_object_placer__surface_riex_handles, TMatrix transform,
      const Point2 *gpu_object_placer__boxBorderX, const Point2 *gpu_object_placer__boxBorderY,
      const Point2 *gpu_object_placer__boxBorderZ ECS_REQUIRE(ecs::Tag box_zone)) {
      G_ASSERT(!(gpu_object_placer__filled && gpu_object_placer__buffer_offset == -1));
      adjust_transform_to_border_offset(gpu_object_placer__boxBorderX, 0, transform);
      adjust_transform_to_border_offset(gpu_object_placer__boxBorderY, 1, transform);
      adjust_transform_to_border_offset(gpu_object_placer__boxBorderZ, 2, transform);

      float currentDistanceSq = lengthSq(transform.getcol(3) - camera_pos);
      gpu_object_placer__current_distance_squared = currentDistanceSq;

      if (currentDistanceSq >= gpu_object_placer__visible_distance_squared)
      {
        if (gpu_object_placer__buffer_offset != -1)
        {
          matricesBuffer.releaseBufferOffset(gpu_object_placer__buffer_size,
            gpu_object_placer__distance_emitter_buffer_size + gpu_object_placer__distance_emitter_decal_buffer_size,
            gpu_object_placer__buffer_offset);
        }
        resetReadbackEntity(eid);
        gpu_object_placer__filled = false;

        return;
      }

      if (gpu_object_placer__filled || gpu_object_placer__buffer_size == 0)
      {
        if (gpu_object_placer__distance_emitter_is_dirty && gpu_object_placer__buffer_size > 0)
          updateDistanceEmitterMatrix(gpu_object_placer__buffer_offset, gpu_object_placer__distance_emitter_buffer_size,
            gpu_object_placer__buffer_size, gpu_object_placer__distance_emitter_eid, gpu_object_placer__distance_to_scale_from,
            gpu_object_placer__distance_to_scale_to, gpu_object_placer__distance_to_rotation_from,
            gpu_object_placer__distance_to_rotation_to, gpu_object_placer__distance_to_scale_pow,
            gpu_object_placer__distance_to_rotation_pow, gpu_object_placer__use_distance_emitter,
            gpu_object_placer__distance_affect_decals, gpu_object_placer__distance_out_of_range);

        gpu_object_placer__distance_emitter_is_dirty = false;
        return;
      }

      gpu_object_placer__distance_emitter_is_dirty = false;
      bool needGeometryGather = gpu_object_placer__place_on_geometry;

      if (gpu_object_placer__buffer_size == -1)
      {
        int objCount = calculateObjectCount(eid, gpu_object_placer__object_density, transform, gpu_object_placer__place_on_geometry,
          gpu_object_placer__min_gathered_triangle_size, gpu_object_placer__triangle_edge_length_ratio_cutoff,
          gpu_object_placer__object_up_vector_threshold, gpu_object_placer__object_max_count,
          gpu_object_placer__on_rendinst_geometry_count, gpu_object_placer__on_terrain_geometry_count,
          gpu_object_placer__surface_riex_handles);

        if (objCount <= 0)
        {
          gpu_object_placer__on_rendinst_geometry_count = 0;
          gpu_object_placer__on_terrain_geometry_count = 0;
        }
        else
        {
          G_ASSERT(gpu_object_placer__on_rendinst_geometry_count >= 0);
          G_ASSERT(gpu_object_placer__on_terrain_geometry_count >= 0);
          G_ASSERT(objCount <= gpu_object_placer__object_max_count);
          G_ASSERT(!gpu_object_placer__place_on_geometry ||
                   objCount == (gpu_object_placer__on_rendinst_geometry_count + gpu_object_placer__on_terrain_geometry_count));
        }

        gpu_object_placer__buffer_size = objCount;
        if (gpu_object_placer__buffer_size <= 0)
          return;
        needGeometryGather = false; // already gathered in calculateObjectCount
      }

      gpu_object_placer__distance_emitter_buffer_size =
        gpu_object_placer__use_distance_emitter && gpu_object_placer__distance_emitter_eid != INVALID_ENTITY_ID
          ? gpu_object_placer__buffer_size
          : 0;
      gpu_object_placer__distance_emitter_decal_buffer_size = gpu_object_placer__use_distance_emitter &&
                                                                  gpu_object_placer__distance_emitter_eid != INVALID_ENTITY_ID &&
                                                                  gpu_object_placer__distance_affect_decals
                                                                ? gpu_object_placer__buffer_size
                                                                : 0;

      if (!matricesBuffer.getBufferOffset(gpu_object_placer__buffer_size + gpu_object_placer__distance_emitter_buffer_size +
                                            gpu_object_placer__distance_emitter_decal_buffer_size,
            gpu_object_placer__buffer_offset))
        return;

      gpu_object_placer__filled =
        placeInBox(gpu_object_placer__buffer_size, gpu_object_placer__ri_asset_idx, gpu_object_placer__buffer_offset, transform,
          gpu_object_placer__object_scale_range, gpu_object_placer__object_up_vector_threshold,
          gpu_object_placer__distance_based_scale, gpu_object_placer__min_scale_radius, gpu_object_placer__place_on_geometry,
          gpu_object_placer__min_gathered_triangle_size, gpu_object_placer__triangle_edge_length_ratio_cutoff,
          gpu_object_placer__on_rendinst_geometry_count, gpu_object_placer__on_terrain_geometry_count,
          gpu_object_placer__object_density, gpu_object_placer__surface_riex_handles, needGeometryGather);
      updateDistanceEmitterMatrix(gpu_object_placer__buffer_offset, gpu_object_placer__distance_emitter_buffer_size,
        gpu_object_placer__buffer_size, gpu_object_placer__distance_emitter_eid, gpu_object_placer__distance_to_scale_from,
        gpu_object_placer__distance_to_scale_to, gpu_object_placer__distance_to_rotation_from,
        gpu_object_placer__distance_to_rotation_to, gpu_object_placer__distance_to_scale_pow,
        gpu_object_placer__distance_to_rotation_pow, gpu_object_placer__use_distance_emitter,
        gpu_object_placer__distance_affect_decals, gpu_object_placer__distance_out_of_range);
    });

  if (!removeBoxList.empty())
  {
    gpu_object_placer_remove_ecs_query(
      [&](int &gpu_object_placer__buffer_offset, int &gpu_object_placer__buffer_size,
        int &gpu_object_placer__on_rendinst_geometry_count, int &gpu_object_placer__on_terrain_geometry_count,
        const TMatrix &transform ECS_REQUIRE(ecs::Tag box_zone)) {
        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), matricesBuffer.getBuf());
        ShaderGlobal::set_int(gpu_objects_buffer_offsetVarId, gpu_object_placer__buffer_offset);
        ShaderGlobal::set_int(gpu_objects_matrices_countVarId, gpu_object_placer__buffer_size);
        ShaderGlobal::set_int(gpu_objects_on_rendinst_geometry_matrices_countVarId, gpu_object_placer__on_rendinst_geometry_count);
        ShaderGlobal::set_int(gpu_objects_on_terrain_geometry_matrices_countVarId, gpu_object_placer__on_terrain_geometry_count);

        // We expect very few erase boxes here.
        for (auto &box : removeBoxList)
        {
          d3d::resource_barrier({matricesBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
          ShaderGlobal::set_color4(gpu_objects_remove_box_minVarId, box.lim[0]);
          ShaderGlobal::set_color4(gpu_objects_remove_box_maxVarId, box.lim[1]);
          objectRemover->dispatchThreads(
            (gpu_object_placer__on_rendinst_geometry_count + gpu_object_placer__on_terrain_geometry_count), 1, 1);
        }

        G_UNREFERENCED(transform);
      });

    removeBoxList.clear();
  }
}

void VolumePlacer::addEraseBox(const Point3 &box_min, const Point3 &box_max) { removeBoxList.emplace_back(box_min, box_max); }

bool VolumePlacer::RingBuffer::getBufferOffset(int count, int &buf_offset)
{
  if (buf_offset != -1)
    return true;
  if (dataEnd == dataStart && dataLastUsed)
  {
    // entire buffer is used and there is no empty space
    sizeForExpand += count;
    needExpand = true;
    return false;
  }
  if (dataEnd >= dataStart && dataEnd + count > size)
  {
    if (dataStart >= count)
    {
      // make cycle
      dataLastUsed = dataEnd;
      dataEnd = 0;
    }
    else
    {
      sizeForExpand += count;
      needExpand = true;
      return false;
    }
  }
  else if (dataEnd < dataStart && dataEnd + count > dataStart)
  {
    sizeForExpand += count;
    needExpand = true;
    return false;
  }

  buf_offset = dataEnd;
  dataEnd += count;
  return true;
}

void VolumePlacer::RingBuffer::releaseBufferOffset(int count, int additional_count, int &buf_offset)
{
  G_ASSERT(buf_offset != -1);
  G_ASSERT(count != -1);
  count += additional_count;
  int bufOffset = buf_offset;
  buf_offset = -1;
  if (dataStart != bufOffset)
  {
    releasedBufferRecords.insert(BufferRec(bufOffset, count));
    return;
  }
  dataStart += count;
  auto itFirst = releasedBufferRecords.begin();
  // needed bufferOffset can lay not at the begin in case when buffer make cycle
  if (itFirst != releasedBufferRecords.end() && itFirst->offset < dataStart)
    itFirst = releasedBufferRecords.find(BufferRec(dataStart, 0));
  auto itLast = itFirst;
  while (itLast != releasedBufferRecords.end() && itLast->offset == dataStart)
  {
    dataStart += itLast->size;
    ++itLast;
  }
  releasedBufferRecords.erase(itFirst, itLast);
  if (dataStart == dataLastUsed)
  {
    dataStart = 0;
    dataLastUsed = 0;
    itLast = releasedBufferRecords.begin();
    while (itLast != releasedBufferRecords.end() && itLast->offset == dataStart)
    {
      dataStart += itLast->size;
      ++itLast;
    }
    releasedBufferRecords.erase(releasedBufferRecords.begin(), itLast);
  }
}

void VolumePlacer::RingBuffer::expandBuffer()
{
  constexpr int EXPAND_STEP = 1024;
  constexpr int MAX_MATRICES_COUNT = 16 << 10; // 64b per matrix, 1 Mb at all
  if (size == MAX_MATRICES_COUNT)
  {
    // check if defragmentation can help
    int releasedSize = 0;
    for (const BufferRec &rec : releasedBufferRecords)
      releasedSize += rec.size;
    if (dataStart <= dataEnd && !dataLastUsed) // empty buffer from 0 to dataStart and from dataEnd to size
      releasedSize += size - dataEnd + dataStart;
    else // empty buffer from dataEnd to dataStart and from dataLastUsed to size
      releasedSize += dataStart - dataEnd + size - dataLastUsed;
    if (releasedSize < sizeForExpand)
      return;
  }
  size += max(EXPAND_STEP, sizeForExpand);
  size = min(size, MAX_MATRICES_COUNT);
  static int bufferIndex = 0;
  auto name = "gpu_objects_matrices_" + eastl::to_string(bufferIndex++);
  BufPtr newBuf = dag::buffers::create_ua_structured(sizeof(Point4), size * ROWS_IN_MATRIX, name.c_str());
  int offset = 0;
  if (dataStart != dataEnd || dataLastUsed)
  {
    gpu_object_placer_copy_on_expand_ecs_query(
      [&](int &gpu_object_placer__buffer_offset, int gpu_object_placer__buffer_size,
        int gpu_object_placer__distance_emitter_buffer_size, int gpu_object_placer__distance_emitter_decal_buffer_size) {
        if (gpu_object_placer__buffer_offset == -1)
          return;
        G_ASSERT(gpu_object_placer__buffer_size != -1);
        int bufferSize = gpu_object_placer__buffer_size + gpu_object_placer__distance_emitter_buffer_size +
                         gpu_object_placer__distance_emitter_decal_buffer_size;
        getBuf()->copyTo(newBuf.get(), offset * MATRIX_SIZE, gpu_object_placer__buffer_offset * MATRIX_SIZE, bufferSize * MATRIX_SIZE);
        gpu_object_placer__buffer_offset = offset;
        offset += bufferSize;
      });
    releasedBufferRecords.clear();
    dataStart = 0;
    dataEnd = offset;
    dataLastUsed = 0;
  }
  else
  {
    dataStart = 0;
    dataEnd = 0;
  }
  buf = eastl::move(newBuf);
  needExpand = false;
  sizeForExpand = 0;
}

static void set_placer_transform(const TMatrix4 &transform)
{
  TMatrix4 tm = transform.transpose();
  ShaderGlobal::set_float4x4(gpu_objects_placer_tmVarId, tm);
}

bool VolumePlacer::placeInBox(int count, int ri_idx, int buf_offset, const TMatrix &transform, const Point2 &scale_range,
  const Point4 &up_vector_threshold, const Point2 &distance_based_scale, float min_scale_radius, bool on_geometry,
  float min_triangle_size, float triangle_edge_length_ratio_cutoff, int on_rendinst_geometry_count, int on_terrain_geometry_count,
  float density, riex_handles &surface_riex_handles, bool need_geometry_gather)
{
  if (on_geometry && need_geometry_gather)
  {
    if (!gatherGeometryInBox(transform, min_triangle_size, triangle_edge_length_ratio_cutoff, up_vector_threshold, density,
          surface_riex_handles))
      return false;
  }
  if (on_geometry)
  {
    prefixSumShaders.calculate(gatheredTrianglesBuffer.getBuf());
  }
  ShaderGlobal::set_int(gpu_objects_buffer_offsetVarId, buf_offset);
  ShaderGlobal::set_int(gpu_objects_matrices_countVarId, count);
  ShaderGlobal::set_int(gpu_objects_on_rendinst_geometry_matrices_countVarId, on_rendinst_geometry_count);
  ShaderGlobal::set_int(gpu_objects_on_terrain_geometry_matrices_countVarId, on_terrain_geometry_count);
  ShaderGlobal::set_int(gpu_objects_ri_pool_id_offsetVarId, ri_idx * rendinst::render::PER_DRAW_VECS_COUNT + 1);
  int maxTerrainObjectsCount = getMaxTerrainObjectsCount(transform, density);
  ShaderGlobal::set_int(gpu_objects_max_on_terrain_instance_countVarId, maxTerrainObjectsCount);
  ShaderGlobal::set_color4(gpu_objects_scale_rotateVarId, scale_range.x, scale_range.y, 0, 0);
  ShaderGlobal::set_color4(gpu_objects_up_vectorVarId, Color4::xyzw(up_vector_threshold));
  ShaderGlobal::set_color4(gpu_objects_scale_radiusVarId, distance_based_scale.x, distance_based_scale.y, min_scale_radius, 0);

  set_placer_transform(TMatrix4(transform));

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), matricesBuffer.getBuf());

  if (on_geometry)
  {
    if (on_terrain_geometry_count > 0)
    {
      TIME_D3D_PROFILE(VolumePlacer_clearTerrainObjects)
      clearTerrainObjects->dispatchThreads(on_terrain_geometry_count, 1, 1);
    }

    if (on_rendinst_geometry_count > 0)
    {
      TIME_D3D_PROFILE(VolumePlacer_onRendinstGeometryPlacer)
      if (on_terrain_geometry_count > 0)
        d3d::resource_barrier({matricesBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      onRendinstGeometryPlacer->dispatchThreads(on_rendinst_geometry_count, 1, 1);
    }

    if (on_terrain_geometry_count > 0)
    {
      TIME_D3D_PROFILE(VolumePlacer_onTerrainGeometryPlacer)
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), counterBuffer.getBuf());
      d3d::resource_barrier({counterBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      d3d::resource_barrier({matricesBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      Point2 bboxYMinMax = getBboxYMinMax(transform);
      ShaderGlobal::set_color4(gpu_objects_bbox_y_min_maxVarId, bboxYMinMax);
      onTerrainGeometryPlacer->dispatchThreads(maxTerrainObjectsCount, 1, 1);
    }
  }
  else
  {
    TIME_D3D_PROFILE(VolumePlacer_box_placer)
    boxPlacer->dispatchThreads(count, 1, 1);
  }

  return true;
}

int VolumePlacer::getMaxTerrainObjectsCount(const TMatrix &transform, float density) const
{
  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, transform.array);
  Point4 extent;
  v_stu(&extent.x, v_add(v_abs(tm.col0), v_add(v_abs(tm.col1), v_abs(tm.col2))));
  int result = static_cast<int>(extent.x * extent.z * density);
  return clamp(result, 0, ON_TERRAIN_OBJECT_COUNT_CLAMP);
}

bool VolumePlacer::gatherGeometryInBox(const TMatrix &transform, float min_triangle_size, float triangle_edge_length_ratio_cutoff,
  const Point4 &up_vector_threshold, float density, riex_handles &surface_riex_handles)
{
  if (waitReadbackEntity)
    return false;
  bbox3f bbox;
  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, transform.array);
  v_bbox3_init(bbox, tm, {v_neg(V_C_HALF), V_C_HALF});
  const float minGeometrySize = 1.0;
  surface_riex_handles.clear();
  rendinst::riex_collidable_t out_handles;
  rendinst::gatherRIGenExtraCollidableMin(out_handles, bbox, minGeometrySize);

  struct MeshInfo
  {
    Vbuffer *vb;
    Ibuffer *ib;
    int startIndex;
    int numFaces;
    int baseVertex;
    int stride;
    int materialId;
    BBox3 bbox; // only for extreme parameter check
  };
  dag::Vector<MeshInfo, framemem_allocator> geometryMeshes;
  if (!out_handles.empty())
  {
    int numFaces = 0;
    for (int i = 0, e = out_handles.size(); i < e; ++i)
    {
      rendinst::riex_handle_t handle = out_handles[i];
      int riResIdx = rendinst::handle_to_ri_type(handle);
      if (rendinst::isRIGenExtraDynamic(riResIdx))
        continue;
      surface_riex_handles.push_back(handle);
      RenderableInstanceLodsResource *riLodsRes = rendinst::getRIGenExtraRes(riResIdx);
      RenderableInstanceResource *riRes = riLodsRes->lods[riLodsRes->getQlBestLod()].scene;
      dag::ConstSpan<ShaderMesh::RElem> elems = riRes->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);
      for (const ShaderMesh::RElem &elem : elems)
      {
        MeshInfo &mesh = *(MeshInfo *)geometryMeshes.push_back_uninitialized();
        mesh.vb = elem.vertexData->getVB();
        mesh.ib = elem.vertexData->getIB();
        mesh.startIndex = elem.si;
        mesh.numFaces = elem.numf;
        mesh.baseVertex = elem.baseVertex;
        mesh.stride = (elem.vertexData->getStride() & 0xffff) | (i << 16);
        numFaces += mesh.numFaces;
      }
    }
    eastl::sort(geometryMeshes.begin(), geometryMeshes.end(), [](const MeshInfo &left, const MeshInfo &right) -> bool {
      if (left.ib != right.ib)
        return left.ib < right.ib;
      return left.vb < right.vb;
    });

    numFaces = min(numFaces, MAX_GATHERED_TRIANGLES);

    if (gatheredTrianglesBufferSize < numFaces)
    {
      gatheredTrianglesBuffer.close();
      gatheredTrianglesBuffer =
        dag::buffers::create_ua_sr_structured(sizeof(GeometryTriangle), numFaces, "gpu_objects_gathered_triangles");
      gatheredTrianglesBufferSize = numFaces;
    }
    if (geometryMeshesBufferSize < geometryMeshes.size() || !geometryMeshesBuffer)
    {
      geometryMeshesBuffer.close();
      geometryMeshesBuffer = dag::create_sbuffer(sizeof(GeometryMesh), max<int>(geometryMeshes.size(), 64),
        SBCF_BIND_SHADER_RES | SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED, 0, "gpu_objects_geometry_meshes");
      G_ASSERT(geometryMeshesBuffer.getBuf());
      geometryMeshesBufferSize = geometryMeshes.size();
    }
    mat44f invPlacerTm;
    v_mat44_inverse43(invPlacerTm, tm);

    G_ASSERT(geometryMeshesBuffer.getBuf());
    if (LockedBuffer<GeometryMesh> buf =
          lock_sbuffer<GeometryMesh>(geometryMeshesBuffer.getBuf(), 0, geometryMeshes.size(), VBLOCK_DISCARD | VBLOCK_WRITEONLY))
    {
      for (size_t index = 0; index < geometryMeshes.size(); index++)
      {
        const MeshInfo &mesh = geometryMeshes[index];
        buf[index].startIndex = mesh.startIndex;
        buf[index].numFaces = mesh.numFaces;
        buf[index].baseVertex = mesh.baseVertex;
        buf[index].stride = mesh.stride & 0xffff;
        int ri_handle_idx = mesh.stride >> 16;
        mat44f meshTm;
        v_mat44_mul43(meshTm, invPlacerTm, rendinst::getRIGenExtra43(out_handles[ri_handle_idx]));
        mat43f meshTm43;
        v_mat44_transpose_to_mat43(meshTm43, meshTm);
        v_stu(&buf[index].tmRow0.x, meshTm43.row0);
        v_stu(&buf[index].tmRow1.x, meshTm43.row1);
        v_stu(&buf[index].tmRow2.x, meshTm43.row2);
      }
    }
    else
      return false;
  }

  unsigned int zeros[4] = {0};
  d3d::clear_rwbufi(counterBuffer.getBuf(), zeros);
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), counterBuffer.getBuf());

  ShaderGlobal::set_real(gpu_objects_min_gathered_triangle_sizeVarId, min_triangle_size);
  ShaderGlobal::set_real(gpu_objects_cutoff_ratioVarId, triangle_edge_length_ratio_cutoff);
  ShaderGlobal::set_color4(gpu_objects_up_vectorVarId, Color4::xyzw(up_vector_threshold));
  set_placer_transform(TMatrix4(transform));

  if (!geometryMeshes.empty())
  {
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), gatheredTrianglesBuffer.getBuf());
    ShaderGlobal::set_int(gpu_objects_max_trianglesVarId, gatheredTrianglesBufferSize);

    TIME_D3D_PROFILE(VolumePlacer_gatherTriangles)
    for (auto it = geometryMeshes.begin(), end = geometryMeshes.end(); it < end;)
    {
      Vbuffer *vb = it->vb;
      Ibuffer *ib = it->ib;
      uint32_t startNo = it - geometryMeshes.begin();
      for (; it < end && it->vb == vb && it->ib == ib; ++it) {}
      ShaderGlobal::set_int(gpu_objects_mesh_offsetVarId, startNo);
      STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, 14, VALUE), ib);
      STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, 15, VALUE), vb);
      gatherTriangles->dispatch(it - geometryMeshes.begin() - startNo, 1, 1);
    }
    d3d::resource_barrier({gatheredTrianglesBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  }

  d3d::resource_barrier({counterBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  {
    TIME_D3D_PROFILE(VolumePlacer_terrainCount)
    int maxTerrainObjectsCount = getMaxTerrainObjectsCount(transform, density);
    ShaderGlobal::set_int(gpu_objects_max_on_terrain_instance_countVarId, maxTerrainObjectsCount);
    Point2 bboxYMinMax = getBboxYMinMax(transform);
    ShaderGlobal::set_color4(gpu_objects_bbox_y_min_maxVarId, bboxYMinMax);
    computeTerrainObjectsCount->dispatchThreads(maxTerrainObjectsCount, 1, 1);
  }
  return true;
}

Point2 VolumePlacer::getBboxYMinMax(const TMatrix &transform) const
{
  float center = abs(transform.getcol(3).y);
  float offset = abs(transform.getcol(0).y) + abs(transform.getcol(1).y) + abs(transform.getcol(2).y);
  return Point2(center - 0.5f * offset, center + 0.5f * offset);
}

int VolumePlacer::calculateObjectCount(ecs::EntityId eid, float density, const TMatrix &transform, bool on_geometry,
  float min_triangle_size, float triangle_edge_length_ratio_cutoff, const Point4 &up_vector_threshold, int object_max_count,
  int &on_rendinst_geometry_count, int &on_terrain_geometry_count, riex_handles &surface_riex_handles)
{
  if (!on_geometry)
  {
    Point3 sides = transform % Point3(2, 2, 2);
    float volume = sides.x * sides.y * sides.z;
    return density * volume;
  }
  if (eid == waitReadbackEntity && d3d::get_event_query_status(counterReadbackFence.get(), false))
  {
    waitReadbackEntity.reset();
    readbackPending = false;
    if (auto locked = lock_sbuffer<const uint>(counterBuffer.getBuf(), 0, COUNTERS_NUM, 0))
    {
      on_rendinst_geometry_count = density * (locked[TRIANGLES_AREA_SUM] / (TRIANGLE_AREA_MULTIPLIER * 2.0));
      on_terrain_geometry_count = clamp(static_cast<int>(locked[TERRAIN_OBJECTS_COUNT]), 0, ON_TERRAIN_OBJECT_COUNT_CLAMP);
      int count = on_rendinst_geometry_count + on_terrain_geometry_count;
      if (count > object_max_count)
      {
        on_rendinst_geometry_count = on_rendinst_geometry_count * (static_cast<float>(object_max_count) / count);
        on_terrain_geometry_count = object_max_count - on_rendinst_geometry_count;
      }
      return on_rendinst_geometry_count + on_terrain_geometry_count;
    }
    else
      return -1;
  }
  if (!waitReadbackEntity)
  {
    if (readbackPending)
    {
      if (!d3d::get_event_query_status(counterReadbackFence.get(), false))
        return -1;
      auto readbackFinish = lock_sbuffer<const uint>(counterBuffer.getBuf(), 0, COUNTERS_NUM, 0);
      readbackPending = false;
    }
    if (!gatherGeometryInBox(transform, min_triangle_size, triangle_edge_length_ratio_cutoff, up_vector_threshold, density,
          surface_riex_handles))
      return 0;
    if (counterBuffer->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
      counterBuffer->unlock();
    readbackPending = true;
    d3d::issue_event_query(counterReadbackFence.get());
    waitReadbackEntity = eid;
  }
  return -1;
}

void VolumePlacer::resetReadbackEntity(ecs::EntityId eid)
{
  if (eid == waitReadbackEntity)
    waitReadbackEntity.reset();
}

void VolumePlacer::drawDebugGeometry(const TMatrix &transform, float min_triangle_size, float triangle_edge_length_ratio_cutoff,
  const Point4 &up_vector_threshold, float density, riex_handles &surface_riex_handles)
{
  if (!gatherGeometryInBox(transform, min_triangle_size, triangle_edge_length_ratio_cutoff, up_vector_threshold, density,
        surface_riex_handles))
    return;
  if (!debugGatheredTrianglesRenderer.shader)
  {
    debugGatheredTrianglesRenderer.init("gpu_object_placer_draw_geometry_debug", nullptr, 0, "gpu_object_placer_draw_geometry_debug");
  }
  set_placer_transform(TMatrix4(transform));
  ShaderGlobal::set_color4(gpu_objects_debug_colorVarId, Color4(1, 0, 0, 0.3));
  debugGatheredTrianglesRenderer.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw(PRIM_TRILIST, 0, gatheredTrianglesBufferSize);

  d3d::setwire(true);
  ShaderGlobal::set_color4(gpu_objects_debug_colorVarId, Color4(1, 0, 0, 1));
  debugGatheredTrianglesRenderer.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw(PRIM_TRILIST, 0, gatheredTrianglesBufferSize);
  d3d::setwire(false);
}

void VolumePlacer::updateVisibility(const Frustum &frustum, ShadowPass for_shadow)
{
  for (Visibility &v : visibilityByLod)
    v.clear();
  for (Visibility &v : visibilityByLodDecal)
    v.clear();
  for (int &numInstances : numInstancesToDraw)
    numInstances = 0;

  gpu_object_placer_visibility_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag box_zone, eastl::true_type gpu_object_placer__filled) int gpu_object_placer__ri_asset_idx,
      int gpu_object_placer__buffer_size, float gpu_object_placer__current_distance_squared, int gpu_object_placer__buffer_offset,
      int gpu_object_placer__distance_emitter_buffer_size, int gpu_object_placer__distance_emitter_decal_buffer_size,
      bool gpu_object_placer__opaque, bool gpu_object_placer__decal, bool gpu_object_placer__distorsion,
      bool gpu_object_placer__render_into_shadows, ecs::EntityId gpu_object_placer__distance_emitter_eid, const TMatrix &transform) {
      if (for_shadow == ShadowPass::YES)
        gpu_object_placer__decal = false; // Do not render decals during shadow pass
      if (!gpu_object_placer__opaque && !gpu_object_placer__decal)
        return;
      if (for_shadow == ShadowPass::YES && (!gpu_object_placer__opaque || !gpu_object_placer__render_into_shadows))
        return;

      mat44f tm;
      v_mat44_make_from_43cu_unsafe(tm, transform.array);
      vec4f extent2 = v_add(v_abs(tm.col0), v_add(v_abs(tm.col1), v_abs(tm.col2)));
      vec4f center2 = v_add(tm.col3, tm.col3);
      if (frustum.testBoxExtentB(center2, extent2))
      {
        int riIdx = gpu_object_placer__ri_asset_idx;
        uint32_t layerBits = (gpu_object_placer__opaque << LAYER_OPAQUE) | (gpu_object_placer__decal << LAYER_DECAL) |
                             (gpu_object_placer__distorsion << LAYER_DISTORSION);
        uint32_t key = (riIdx & RI_IDX_MASK) | (layerBits << LAYER_BITS_SHIFT);
        int lod = rendinst::getRIGenExtraPreferrableLod(riIdx, gpu_object_placer__current_distance_squared);

        G_UNUSED(gpu_object_placer__distance_emitter_eid);
        if (gpu_object_placer__distance_emitter_buffer_size > 0)
          G_ASSERT(gpu_object_placer__distance_emitter_eid != INVALID_ENTITY_ID);

        int offset = gpu_object_placer__buffer_offset +
                     (gpu_object_placer__distance_emitter_buffer_size > 0 ? gpu_object_placer__buffer_size : 0);
        visibilityByLod[lod][key].emplace_back(offset, gpu_object_placer__buffer_size);
        G_ASSERT(!(gpu_object_placer__distance_emitter_buffer_size == 0 && gpu_object_placer__distance_emitter_decal_buffer_size > 0));
        int decalOffset =
          gpu_object_placer__buffer_offset + (gpu_object_placer__distance_emitter_decal_buffer_size > 0
                                                 ? gpu_object_placer__buffer_size + gpu_object_placer__distance_emitter_buffer_size
                                                 : 0);
        visibilityByLodDecal[lod][key].emplace_back(decalOffset, gpu_object_placer__buffer_size);

        if (gpu_object_placer__opaque)
          numInstancesToDraw[LAYER_OPAQUE] += gpu_object_placer__buffer_size;
        if (gpu_object_placer__decal)
          numInstancesToDraw[LAYER_DECAL] += gpu_object_placer__buffer_size;
        if (gpu_object_placer__distorsion)
          numInstancesToDraw[LAYER_DISTORSION] += gpu_object_placer__buffer_size;
      }
    });
}

void VolumePlacer::copyMatrices(Sbuffer *dst_buffer, uint32_t &dst_buffer_offset, int lod, int layer,
  eastl::vector<IPoint2> &offsets_and_counts, eastl::vector<uint16_t> &ri_ids)
{
  d3d::driver_command(Drv3dCommand::DELAY_SYNC);
  auto visibility = layer == Layers::LAYER_DECAL ? visibilityByLodDecal[lod] : visibilityByLod[lod];
  for (auto &pair : visibility)
  {
    uint16_t riIdx = pair.first & RI_IDX_MASK;
    uint16_t layerBits = pair.first >> LAYER_BITS_SHIFT;
    if (!(layerBits & (1 << layer)))
      continue;
    int count = 0;
    int offset = dst_buffer_offset;
    for (BufferRec bufRec : pair.second)
    {
      matricesBuffer.getBuf()->copyTo(dst_buffer, dst_buffer_offset * MATRIX_SIZE, bufRec.offset * MATRIX_SIZE,
        bufRec.size * MATRIX_SIZE);
      dst_buffer_offset += bufRec.size;
      count += bufRec.size;
    }
    ri_ids.emplace_back(riIdx);
    offsets_and_counts.emplace_back(offset * ROWS_IN_MATRIX, count);
  }
  d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
}

void VolumePlacer::RingBuffer::invalidate()
{
  if (dataStart != dataEnd || dataLastUsed)
  {
    gpu_object_placer_invalidate_ecs_query(
      [&](bool &gpu_object_placer__filled, int &gpu_object_placer__buffer_offset, int &gpu_object_placer__distance_emitter_buffer_size,
        int &gpu_object_placer__distance_emitter_decal_buffer_size) {
        gpu_object_placer__filled = false;
        gpu_object_placer__buffer_offset = -1;
        gpu_object_placer__distance_emitter_buffer_size = 0;
        gpu_object_placer__distance_emitter_decal_buffer_size = 0;
      });
    releasedBufferRecords.clear();
  }
  dataStart = 0;
  dataEnd = 0;
  dataLastUsed = 0;
}

static int load_ri_asset(const ecs::string &ri_asset_name)
{
  using namespace rendinst;
  const char *name = ri_asset_name.c_str();
  int id = getRIGenExtraResIdx(name);
  if (id < 0)
  {
    debug("GPUObjectsPlacer: auto adding <%s> as riExtra.", name);
    auto riaddf = AddRIFlag::UseShadow | AddRIFlag::GameresPreLoaded; // Expected to be preloaded by `GpuObjectRiResourcePreload`
    id = addRIGenExtraResIdx(name, -1, -1, riaddf);
    if (id < 0)
      logerr("loading <%s> riExtra failed", name);
  }
  return id;
}

static DistanceEmitterParameters get_distance_emitter_entity_params(ecs::EntityId gpu_object_placer__distance_emitter_eid)
{
  DistanceEmitterParameters params{};
  if (gpu_object_placer__distance_emitter_eid)
  {
    gpu_object_placer_distance_emitter_update_after_placing_ecs_query(gpu_object_placer__distance_emitter_eid,
      [&](const TMatrix &transform, float gpu_objects_distance_emitter__range, float gpu_objects_distance_emitter__scale_factor,
        float gpu_objects_distance_emitter__rotation_factor) {
        params.range = gpu_objects_distance_emitter__range;
        params.scale_factor = gpu_objects_distance_emitter__scale_factor;
        params.rotation_factor = gpu_objects_distance_emitter__rotation_factor;
        params.position = transform.getcol(3);
      });
  }
  return params;
}

static void selectClosestDistanceEmitter(ecs::EntityId &gpu_object_placer__distance_emitter_eid,
  bool &gpu_object_placer__distance_emitter_is_dirty, const Point3 &volumePlacerPosition)
{
  float smallestDistanceSquared;
  ecs::EntityId smallestDistanceId = INVALID_ENTITY_ID;
  gpu_object_placer_select_closest_distance_emitter_ecs_query(
    [&](ECS_REQUIRE(float gpu_objects_distance_emitter__range) const ecs::EntityId &eid, const TMatrix &transform) {
      float distanceSquared = (volumePlacerPosition - transform.getcol(3)).lengthSq();
      if (smallestDistanceId == INVALID_ENTITY_ID || smallestDistanceSquared > distanceSquared)
      {
        smallestDistanceId = eid;
        smallestDistanceSquared = distanceSquared;
      }
    });
  gpu_object_placer__distance_emitter_eid = smallestDistanceId;
  gpu_object_placer__distance_emitter_is_dirty = true;
}

void VolumePlacer::updateDistanceEmitterMatrices(const ecs::EntityId changed_distance_emitter_id,
  const TMatrix &changed_distance_emitter_transform)
{
  gpu_object_placer_distance_emitter_update_volume_placers_ecs_query(
    [&](ecs::EntityId &gpu_object_placer__distance_emitter_eid, bool &gpu_object_placer__distance_emitter_is_dirty,
      bool gpu_object_placer__use_distance_emitter, const TMatrix &transform) {
      if (gpu_object_placer__use_distance_emitter)
      {
        Point3 volumePlacerPosition = transform.getcol(3);
        if (changed_distance_emitter_id == gpu_object_placer__distance_emitter_eid)
        {
          // the closest emitter changed, it might not be the closest anymore, so we need to find the new closest one
          selectClosestDistanceEmitter(gpu_object_placer__distance_emitter_eid, gpu_object_placer__distance_emitter_is_dirty,
            volumePlacerPosition);
        }
        else
        {
          // not the closest emitter changed, so we need to check if it's the closest now, or the currently selected remains the
          // closest
          Point3 currentDistanceEmitterPosition;
          gpu_object_placer_get_current_distance_emitter_position_ecs_query(gpu_object_placer__distance_emitter_eid,
            [&](const TMatrix &transform) { currentDistanceEmitterPosition = transform.getcol(3); });
          float changedDistanceEmitterDistanceSquared =
            (volumePlacerPosition - changed_distance_emitter_transform.getcol(3)).lengthSq();
          float currentDistanceEmitterDistanceSquared = (volumePlacerPosition - currentDistanceEmitterPosition).lengthSq();
          if (changedDistanceEmitterDistanceSquared < currentDistanceEmitterDistanceSquared)
          {
            gpu_object_placer__distance_emitter_is_dirty = true;
            gpu_object_placer__distance_emitter_eid = changed_distance_emitter_id;
          }
        }
      }
    });
}

void VolumePlacer::updateDistanceEmitterMatrix(int gpu_object_placer__buffer_offset,
  int gpu_object_placer__distance_emitter_buffer_size, int gpu_object_placer__buffer_size,
  const ecs::EntityId gpu_object_placer__distance_emitter_eid, const Point3 &gpu_object_placer__distance_to_scale_from,
  const Point3 &gpu_object_placer__distance_to_scale_to, float gpu_object_placer__distance_to_rotation_from,
  float gpu_object_placer__distance_to_rotation_to, const Point3 &gpu_object_placer__distance_to_scale_pow,
  float gpu_object_placer__distance_to_rotation_pow, bool gpu_object_placer__use_distance_emitter,
  bool gpu_object_placer__distance_affect_decals, bool gpu_object_placer__distance_out_of_range)
{
  TIME_D3D_PROFILE(VolumePlacer_updateDistanceEmitterMatrix)
  if (gpu_object_placer__buffer_size > 0 && gpu_object_placer__distance_emitter_buffer_size > 0)
  {
    G_UNUSED(gpu_object_placer__use_distance_emitter);
    G_ASSERT(gpu_object_placer__use_distance_emitter && gpu_object_placer__distance_emitter_eid != INVALID_ENTITY_ID);
    auto params = get_distance_emitter_entity_params(gpu_object_placer__distance_emitter_eid);
    Point3 finalScaleTo = gpu_object_placer__distance_to_scale_to * params.scale_factor;
    Point3 minScale = min(gpu_object_placer__distance_to_scale_from, gpu_object_placer__distance_to_scale_to);
    Point3 maxScale = max(gpu_object_placer__distance_to_scale_from, gpu_object_placer__distance_to_scale_to);
    float finalRotationFrom = DegToRad(gpu_object_placer__distance_to_rotation_from);
    float rotationToRadians = DegToRad(gpu_object_placer__distance_to_rotation_to);
    float finalRotationTo = rotationToRadians * params.rotation_factor;
    float minRotation = min(finalRotationFrom, rotationToRadians);
    float maxRotation = max(finalRotationFrom, rotationToRadians);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), matricesBuffer.getBuf());
    d3d::resource_barrier({matricesBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    ShaderGlobal::set_int(gpu_objects_distance_affect_decalVarId, gpu_object_placer__distance_affect_decals ? 1 : 0);
    ShaderGlobal::set_color4(gpu_objects_distance_to_scale_fromVarId, gpu_object_placer__distance_to_scale_from);
    ShaderGlobal::set_color4(gpu_objects_distance_to_scale_toVarId, finalScaleTo);
    ShaderGlobal::set_color4(gpu_objects_distance_to_min_scaleVarId, minScale);
    ShaderGlobal::set_color4(gpu_objects_distance_to_max_scaleVarId, maxScale);
    ShaderGlobal::set_real(gpu_objects_distance_to_rotation_fromVarId, finalRotationFrom);
    ShaderGlobal::set_real(gpu_objects_distance_to_rotation_toVarId, finalRotationTo);
    ShaderGlobal::set_real(gpu_objects_distance_to_min_rotationVarId, minRotation);
    ShaderGlobal::set_real(gpu_objects_distance_to_max_rotationVarId, maxRotation);
    ShaderGlobal::set_color4(gpu_objects_distance_to_scale_powVarId, gpu_object_placer__distance_to_scale_pow);
    ShaderGlobal::set_real(gpu_objects_distance_to_rotation_powVarId, gpu_object_placer__distance_to_rotation_pow);
    ShaderGlobal::set_int(gpu_objects_distance_out_of_rangeVarId, gpu_object_placer__distance_out_of_range ? 1 : 0);
    ShaderGlobal::set_real(gpu_objects_distance_emitter__rangeVarId, params.range);
    ShaderGlobal::set_color4(gpu_objects_distance_emitter__positionVarId, params.position);
    ShaderGlobal::set_int(gpu_objects_buffer_offsetVarId, gpu_object_placer__buffer_offset);
    ShaderGlobal::set_int(gpu_objects_distance_emitter_buffer_offsetVarId,
      gpu_object_placer__buffer_offset + gpu_object_placer__buffer_size);
    ShaderGlobal::set_int(gpu_objects_distance_emitter_decal_buffer_offsetVarId,
      gpu_object_placer__buffer_offset + gpu_object_placer__buffer_size + gpu_object_placer__buffer_size);
    ShaderGlobal::set_int(gpu_objects_matrices_countVarId, gpu_object_placer__buffer_size);
    updateMatrices->dispatchThreads(gpu_object_placer__buffer_size, 1, 1);
  }
}

ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(transform, gpu_objects_distance_emitter__range, gpu_objects_distance_emitter__scale_factor,
  gpu_objects_distance_emitter__rotation_factor)
ECS_REQUIRE(ecs::EntityId eid, TMatrix transform, float gpu_objects_distance_emitter__range,
  float gpu_objects_distance_emitter__scale_factor, float gpu_objects_distance_emitter__rotation_factor)
static __forceinline void gpu_object_distance_emitter_es_event_handler(const ecs::Event &, ecs::EntityId eid, const TMatrix &transform)
{
  G_ASSERT(gpu_objects::volume_placer_mgr);
  gpu_objects::volume_placer_mgr->updateDistanceEmitterMatrices(eid, transform);
}

ECS_TAG(render, dev)
ECS_TRACK(ri_gpu_object__name, gpu_object_placer__place_on_geometry, gpu_object_placer__object_max_count,
  gpu_object_placer__object_density, gpu_object_placer__min_gathered_triangle_size, gpu_object_placer__object_scale_range,
  gpu_object_placer__distance_based_scale, gpu_object_placer__min_scale_radius, gpu_object_placer__object_up_vector_threshold,
  gpu_object_placer__opaque, transform, gpu_object_placer__decal, gpu_object_placer__distorsion,
  gpu_object_placer__distance_to_scale_from, gpu_object_placer__distance_to_scale_to)
ECS_TRACK(gpu_object_placer__distance_to_rotation_from, gpu_object_placer__distance_to_rotation_to,
  gpu_object_placer__distance_to_scale_pow, gpu_object_placer__distance_to_rotation_pow, gpu_object_placer__use_distance_emitter,
  gpu_object_placer__distance_out_of_range, gpu_object_placer__distance_affect_decals, gpu_object_placer__boxBorderX,
  gpu_object_placer__boxBorderY, gpu_object_placer__boxBorderZ)
ECS_REQUIRE(bool gpu_object_placer__place_on_geometry, int gpu_object_placer__object_max_count,
  float gpu_object_placer__object_density, float gpu_object_placer__min_gathered_triangle_size,
  Point2 gpu_object_placer__object_scale_range, Point2 gpu_object_placer__distance_based_scale,
  float gpu_object_placer__min_scale_radius, Point4 gpu_object_placer__object_up_vector_threshold, bool gpu_object_placer__opaque,
  bool gpu_object_placer__decal, bool gpu_object_placer__distorsion, TMatrix transform,
  ecs::EntityId gpu_object_placer__distance_emitter_eid, Point3 gpu_object_placer__distance_to_scale_from,
  Point3 gpu_object_placer__distance_to_scale_to)
ECS_REQUIRE(float gpu_object_placer__distance_to_rotation_from, float gpu_object_placer__distance_to_rotation_to,
  Point3 gpu_object_placer__distance_to_scale_pow, float gpu_object_placer__distance_to_rotation_pow,
  bool gpu_object_placer__use_distance_emitter, bool gpu_object_placer__distance_out_of_range,
  bool gpu_object_placer__distance_affect_decals, Point2 gpu_object_placer__boxBorderX, Point2 gpu_object_placer__boxBorderY,
  Point2 gpu_object_placer__boxBorderZ)
static __forceinline void gpu_object_placer_changed_es_event_handler(const ecs::Event &, const ecs::string &ri_gpu_object__name,
  int &gpu_object_placer__ri_asset_idx, bool &gpu_object_placer__filled, int &gpu_object_placer__buffer_offset,
  int &gpu_object_placer__distance_emitter_decal_buffer_size, int &gpu_object_placer__distance_emitter_buffer_size,
  int &gpu_object_placer__buffer_size, int &gpu_object_placer__on_rendinst_geometry_count,
  int &gpu_object_placer__on_terrain_geometry_count, Point4 &gpu_object_placer__object_up_vector_threshold,
  ecs::EntityId &gpu_object_placer__distance_emitter_eid, bool &gpu_object_placer__distance_emitter_is_dirty, const TMatrix &transform)
{
  G_ASSERT(volume_placer_mgr);
  if (gpu_object_placer__buffer_offset != -1)
    volume_placer_mgr->matricesBuffer.releaseBufferOffset(gpu_object_placer__buffer_size,
      gpu_object_placer__distance_emitter_buffer_size + gpu_object_placer__distance_emitter_decal_buffer_size,
      gpu_object_placer__buffer_offset);
  gpu_object_placer__filled = false;
  gpu_object_placer__buffer_offset = -1;
  gpu_object_placer__distance_emitter_decal_buffer_size = 0;
  gpu_object_placer__distance_emitter_buffer_size = 0;
  gpu_object_placer__buffer_size = -1;
  gpu_object_placer__on_rendinst_geometry_count = 0;
  gpu_object_placer__on_terrain_geometry_count = 0;

  Point3 upVector = Point3::xyz(gpu_object_placer__object_up_vector_threshold);
  normalizeDef(upVector, Point3(0, 0, 0));
  if (lengthSq(upVector - Point3::xyz(gpu_object_placer__object_up_vector_threshold)) > 0.0001)
    gpu_object_placer__object_up_vector_threshold = Point4::xyzV(upVector, gpu_object_placer__object_up_vector_threshold.w);

  gpu_object_placer__ri_asset_idx = load_ri_asset(ri_gpu_object__name);
  selectClosestDistanceEmitter(gpu_object_placer__distance_emitter_eid, gpu_object_placer__distance_emitter_is_dirty,
    transform.getcol(3));
}

static int gpu_object_placers_count = 0;

ECS_TAG(render)
static __forceinline void gpu_object_placer_create_es_event_handler(const ecs::EventEntityCreated &,
  const ecs::string &ri_gpu_object__name, int &gpu_object_placer__ri_asset_idx, bool &gpu_object_placer__filled,
  int &gpu_object_placer__buffer_size, int &gpu_object_placer__on_rendinst_geometry_count,
  int &gpu_object_placer__on_terrain_geometry_count, int &gpu_object_placer__buffer_offset,
  int &gpu_object_placer__distance_emitter_buffer_size, int &gpu_object_placer__distance_emitter_decal_buffer_size,
  bool gpu_object_placer__opaque, bool gpu_object_placer__decal, bool gpu_object_placer__distorsion, const TMatrix &transform,
  Point4 &gpu_object_placer__object_up_vector_threshold)
{
#if _TARGET_C1


#endif
  gpu_object_placer__filled = false;
  gpu_object_placer__buffer_offset = -1;
  gpu_object_placer__distance_emitter_decal_buffer_size = 0;
  gpu_object_placer__distance_emitter_buffer_size = 0;
  gpu_object_placer__buffer_size = -1;
  gpu_object_placer__on_rendinst_geometry_count = 0;
  gpu_object_placer__on_terrain_geometry_count = 0;
  const Point3 &pos = transform.getcol(3);
  G_UNUSED(pos);
  if (!gpu_object_placer__opaque && !gpu_object_placer__decal && !gpu_object_placer__distorsion)
    logerr("gpu_object_placer at (%f, %f, %f) has non renderable gpu objects because "
           "opaque, decal and distorsion geometry are disabled",
      pos.x, pos.y, pos.z);

  Point3 upVector = Point3::xyz(gpu_object_placer__object_up_vector_threshold);
  normalizeDef(upVector, Point3(0, 0, 0));
  gpu_object_placer__object_up_vector_threshold = Point4::xyzV(upVector, gpu_object_placer__object_up_vector_threshold.w);

  gpu_object_placer__ri_asset_idx = load_ri_asset(ri_gpu_object__name);
  rendinst::gpuobjects::init_r(); // Implicitly depends on it
  if (!volume_placer_mgr)
    init_volume_placer_mgr();
  gpu_object_placers_count++;
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static __forceinline void gpu_object_placer_destroy_es_event_handler(const ecs::Event &, ecs::EntityId eid,
  int gpu_object_placer__buffer_size, int gpu_object_placer__buffer_offset, int gpu_object_placer__distance_emitter_buffer_size,
  int gpu_object_placer__distance_emitter_decal_buffer_size)
{
#if _TARGET_C1


#endif
  G_ASSERT(volume_placer_mgr);
  volume_placer_mgr->resetReadbackEntity(eid);
  if (gpu_object_placer__buffer_offset != -1)
    volume_placer_mgr->matricesBuffer.releaseBufferOffset(gpu_object_placer__buffer_size,
      gpu_object_placer__distance_emitter_buffer_size + gpu_object_placer__distance_emitter_decal_buffer_size,
      gpu_object_placer__buffer_offset);
  gpu_object_placers_count--;
  if (!gpu_object_placers_count)
    close_volume_placer_mgr();
}

static void volume_placer_after_reset(bool /*full_reset*/)
{
  if (volume_placer_mgr)
    volume_placer_mgr->matricesBuffer.invalidate();
}

} // namespace gpu_objects

ECS_TAG(render, dev)
ECS_NO_ORDER
ECS_REQUIRE(ecs::Tag box_zone, eastl::true_type gpu_object_placer__show_geometry)
static __forceinline void gpu_object_placer_draw_debug_geometry_es(const ecs::UpdateStageInfoRenderDebug &,
  float gpu_object_placer__min_gathered_triangle_size, float gpu_object_placer__triangle_edge_length_ratio_cutoff,
  const Point4 &gpu_object_placer__object_up_vector_threshold, const TMatrix &transform, float gpu_object_placer__object_density,
  gpu_objects::riex_handles &gpu_object_placer__surface_riex_handles)
{
  G_ASSERT(gpu_objects::volume_placer_mgr);
  gpu_objects::volume_placer_mgr->drawDebugGeometry(transform, gpu_object_placer__min_gathered_triangle_size,
    gpu_object_placer__triangle_edge_length_ratio_cutoff, gpu_object_placer__object_up_vector_threshold,
    gpu_object_placer__object_density, gpu_object_placer__surface_riex_handles);
}

ECS_TAG(render)
static __forceinline void gpu_object_placer_remove_objects_es_event_handler(const EventOnRendinstDamage &evt,
  gpu_objects::riex_handles &gpu_object_placer__surface_riex_handles)
{
  rendinst::riex_handle_t riex_handle = evt.get<0>();
  auto handle =
    eastl::find(gpu_object_placer__surface_riex_handles.begin(), gpu_object_placer__surface_riex_handles.end(), riex_handle);
  if (handle != gpu_object_placer__surface_riex_handles.end())
  {
    const auto &bbox = evt.get<2>();
    const auto &tm = evt.get<1>();

    auto min = bbox.lim[0] * tm;
    auto max = bbox.lim[1] * tm;

    gpu_objects::volume_placer_mgr->addEraseBox(min, max);
  }
}

using namespace console;

static bool volume_placer_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;

  CONSOLE_CHECK_NAME_EX("volume_placer", "removeInBox", 7, 7, "Removes volume placer gpu objects in a box",
    "[bbox_min_x] [bbox_min_y] [bbox_min_z] [bbox_max_x] [bbox_max_y] [bbox_max_z]")
  {
    gpu_objects::get_volume_placer_mgr()->addEraseBox(Point3(atof(argv[1]), atof(argv[2]), atof(argv[3])),
      Point3(atof(argv[4]), atof(argv[5]), atof(argv[6])));
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(volume_placer_console_handler);

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_AFTER_RESET_FUNC(gpu_objects::volume_placer_after_reset);
