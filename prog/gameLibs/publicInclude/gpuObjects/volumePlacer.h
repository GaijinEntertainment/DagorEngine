//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <util/dag_baseDef.h>
#include <daECS/core/entityId.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include "gpuObjects.h"

class ComputeShaderElement;
class Point3;
class IPoint2;
class TMatrix;
struct Frustum;

namespace gpu_objects
{

using riex_handles = dag::RelocatableFixedVector<rendinst::riex_handle_t, 64, true>;

class VolumePlacer
{
public:
  enum Layers
  {
    LAYER_OPAQUE,
    LAYER_DECAL,
    LAYER_DISTORSION,
    MAX_LAYERS
  };

private:
  eastl::array<int, MAX_LAYERS> numInstancesToDraw;
  eastl::unique_ptr<ComputeShaderElement> boxPlacer;
  eastl::unique_ptr<ComputeShaderElement> updateMatrices, clearTerrainObjects, gatherTriangles, computeTerrainObjectsCount,
    onTerrainGeometryPlacer, onRendinstGeometryPlacer, objectRemover;

  struct PrefixSumShaders
  {
    UniqueBuf groupCountIndirect;
    eastl::unique_ptr<ComputeShaderElement> getGroupCount;
    eastl::unique_ptr<ComputeShaderElement> sumSteps;
    PrefixSumShaders();
    void calculate(Sbuffer *gathered_triangles);
  } prefixSumShaders;

  UniqueBufHolder counterBuffer;
  ecs::EntityId waitReadbackEntity;
  bool readbackPending;
  EventQueryHolder counterReadbackFence;
  UniqueBufHolder gatheredTrianglesBuffer;
  int gatheredTrianglesBufferSize = 0;
  UniqueBufHolder geometryMeshesBuffer;
  int geometryMeshesBufferSize = 0;

  DynamicShaderHelper debugGatheredTrianglesRenderer;

  struct BufferRec
  {
    int offset;
    int size;
    BufferRec(int offset, int size) : offset(offset), size(size) {}
    bool operator<(const BufferRec &r) const { return offset < r.offset; }
  };
  using Visibility = eastl::vector_map<uint32_t, eastl::vector<BufferRec>>;
  eastl::array<Visibility, gpu_objects::MAX_LODS> visibilityByLod, visibilityByLodDecal;
  eastl::fixed_vector<BBox3, 64> removeBoxList;

  bool placeInBox(int count, int ri_idx, int buf_offset, const TMatrix &transform, const Point2 &scale_range,
    const Point4 &up_vector_threshold, const Point2 &distance_based_scale, float min_scale_radius, bool on_geometry,
    float min_triangle_size, float triangle_length_ratio_cutoff, int on_rendinst_geometry_count, int on_terrain_geometry_count,
    float density, riex_handles &surface_riex_handles, bool need_geometry_gather = true);
  bool gatherGeometryInBox(const TMatrix &transform, float min_triangle_size, float triangle_length_ratio_cutoff,
    const Point4 &up_vector_threshold, float density, riex_handles &surface_riex_handles);
  int calculateObjectCount(ecs::EntityId eid, float density, const TMatrix &transform, bool on_geometry, float min_triangle_size,
    float triangle_length_ratio_cutoff, const Point4 &up_vector_threshold, int object_max_count, int &on_rendinst_geometry_count,
    int &on_terrain_geometry_count, riex_handles &surface_riex_handles);
  int getMaxTerrainObjectsCount(const TMatrix &transform, float density) const;
  Point2 getBboxYMinMax(const TMatrix &transform) const;

  VolumePlacer();
  friend void init_volume_placer_mgr();

public:
  class RingBuffer
  {
    UniqueBuf buf;
    int size = 0;
    int dataStart = 0, dataEnd = 0, dataLastUsed = 0;
    eastl::vector_set<BufferRec> releasedBufferRecords;
    int sizeForExpand = 0;

  public:
    bool needExpand = false;
    bool getBufferOffset(int count, int &buf_offset);
    void releaseBufferOffset(int count, int additional_count, int &buf_offset);
    void expandBuffer();
    void invalidate();
    Sbuffer *getBuf() { return buf.getBuf(); }
  } matricesBuffer;

  void performPlacing(const Point3 &camera_pos);
  void addEraseBox(const Point3 &box_min, const Point3 &box_max);
  void updateVisibility(const Frustum &frustum, ShadowPass for_shadow);
  void copyMatrices(Sbuffer *dst_buffer, uint32_t &dst_buffer_offset, int lod, int layer, eastl::vector<IPoint2> &offsets_and_counts,
    eastl::vector<uint16_t> &ri_ids);
  int getNumInstancesToDraw(int layer) { return numInstancesToDraw[layer]; }
  void resetReadbackEntity(ecs::EntityId eid);
  void updateDistanceEmitterMatrices(const ecs::EntityId changed_distance_emitter_id,
    const TMatrix &changed_distance_emitter_transform);
  void updateDistanceEmitterMatrix(int gpu_object_placer__buffer_offset, int gpu_object_placer__distance_emitter_buffer_size,
    int gpu_object_placer__buffer_size, const ecs::EntityId gpu_object_placer__distance_emitter_eid,
    const Point3 &gpu_object_placer__distance_to_scale_from, const Point3 &gpu_object_placer__distance_to_scale_to,
    float gpu_object_placer__distance_to_rotation_from, float gpu_object_placer__distance_to_rotation_to,
    const Point3 &gpu_object_placer__distance_to_scale_pow, float gpu_object_placer__distance_to_rotation_pow,
    bool gpu_object_placer__use_distance_emitter, bool gpu_object_placer__distance_affect_decals,
    bool gpu_object_placer__distance_out_of_range);

  void drawDebugGeometry(const TMatrix &transform, float min_triangle_size, float triangle_length_ratio_cutoff,
    const Point4 &up_vector_threshold, float density, riex_handles &surface_riex_handles);
};

VolumePlacer *get_volume_placer_mgr();
void init_volume_placer_mgr();
void close_volume_placer_mgr();

} // namespace gpu_objects