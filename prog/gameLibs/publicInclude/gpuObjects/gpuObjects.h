//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/span.h>
#include <EASTL/stack.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/string.h>
#include <memory/dag_framemem.h>
#include <render/toroidalHelper.h>
#include <gpuObjects/placingParameters.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <rendInst/renderPass.h>
#include <rendInst/layerFlags.h>
#include <rendInst/riShaderConstBuffers.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_frustum.h>
#include <generic/dag_carray.h>
#include <osApiWrappers/dag_cpuJobs.h>

#include <bvh/bvh_connection.h>

class ComputeShaderElement;
class NodeBasedShader;
class IPoint2;
class Occlusion;
struct BVHConnection;

namespace gpu_objects
{

void setup_parameter_validation(const DataBlock *validationBlock);

constexpr int MAX_LODS = 4;
constexpr int MAX_GPU_OBJECTS = 64;

enum class ShadowPass
{
  NO,
  YES
};

class ObjectManager
{
private:
  ToroidalHelper toroidalGrid;
  BufPtr gatheredBuffer;
  BufPtr countersBuffer, bboxesBuffer;
  BufPtr matricesOffsetsBuffer;
  uint32_t cellDispatchTile = 0;
  uint32_t cellTile = 0, cellTileOrig = 0, cellsSideCount = 0, cellsCount = 0;
  uint32_t maxObjectsCountInCell = 0;
  float cellSize = 0;
  float boundingSphereRadius = 0;

  EventQueryHolder updateBboxesFence;
  union AppendOrder
  {
    struct
    {
      unsigned idx;
      float distSq;
    };
    uint64_t sortKey;
    bool operator<(const AppendOrder &o) const { return sortKey < o.sortKey; }
  };
  eastl::vector<AppendOrder> appendOrder;
  eastl::vector<uint32_t> counters;
  eastl::vector<bbox3f> cellsBboxes;
  eastl::vector<uint32_t> matricesOffsets;
  eastl::unique_ptr<ComputeShaderElement> inCellPlacer, counterAndBboxCleaner;
  eastl::unique_ptr<ComputeShaderElement> gatherMatrices;
  bool waitingForGpuData = false;
  Point3 lastOrigin = Point3(0, 0, 0);
  PlacingParameters parameters;

  struct CellToUpdateData
  {
    uint32_t idx;
    float x, z;
  };
  eastl::stack<CellToUpdateData> updateQueue;

  void dispatchCells(const ToroidalQuadRegion &region);
  bool readyToCpuAccess() const;
  void updateGrid(const Point3 &origin);
  void updateBuffer();
  void setShaderVarsAndConsts();
  bool dispatchNextCell();
  void makeMatricesOffsetsBuffer();

  void onLandPlacing();
  void setBombHoleShaderGlobals(const CellToUpdateData cellData);

  void processBboxes(const eastl::span<int32_t> &raw_bboxes);
  void copyMatrices(const eastl::vector<uint32_t> &cells_to_copy, const eastl::vector<uint32_t> &cell_counters, Sbuffer *dst_buffer,
    Sbuffer *src_buffer, uint32_t max_in_cell, uint32_t dst_offset_bytes, uint32_t lod);
  eastl::string assetName;
  uint32_t riPoolId;
  uint32_t riPoolOffset;
  SharedTexHolder mapTexId;
  int numLods;
  eastl::array<float, MAX_LODS> distSqLod;
  eastl::array<eastl::vector<uint32_t>, MAX_LODS> cellIndexesByLods;

  eastl::vector<Point4> bomb_holes;

public:
  rendinst::LayerFlag layer = rendinst::LayerFlag::Opaque;
  ObjectManager(const char *name, uint32_t ri_pool_id, int cell_tile, int cells_size_count, float cell_size,
    float bounding_sphere_radius, dag::ConstSpan<float> dist_sq_lod, const PlacingParameters &parameters);
  ObjectManager(const ObjectManager &) = delete;
  ObjectManager(ObjectManager &&);
  ~ObjectManager();

  void getCellData(int &cell_tile, int &cells_size_count, float &cell_size)
  {
    cell_tile = cellTileOrig;
    cells_size_count = cellsSideCount;
    cell_size = cellSize;
  }
  float getLodRange(int lod) { return sqrt(distSqLod[lod]); }
  int getCellsSizeCount() { return cellsSideCount; }

  uint32_t getRiId() const { return riPoolId; }

  void getParameters(PlacingParameters &p) { p = parameters; }

  void update(const Point3 &origin);
  void gatherBuffers();
  void updateVisibilityAndLods(const Frustum &frustum, const Occlusion *occlusion, ShadowPass for_shadow);
  void addMatricesToBuffer(Sbuffer *buffer, uint32_t offset_in_bytes, int lod);
  void onLandGpuInstancing(Sbuffer *indirection_buffer, int offset);
  uint32_t getInstancesToDraw(uint32_t lod) const;
  uint32_t getInstancesInGridToDraw(uint32_t lod) const;
  uint32_t getCellDispatchTile() const { return cellDispatchTile; }
  uint32_t getMaxPossibleInstances() const { return maxObjectsCountInCell * cellsCount; }
  void invalidate()
  {
    updateGrid(Point3(-10000, -10000, -10000));
    counters.assign(counters.size(), 0);
  }
  void resetGrid() { recreateGrid(cellTileOrig, cellsSideCount, cellSize); }

  void invalidateBBox(const BBox2 &bbox);
  void setParameters(const PlacingParameters &params);

  void recreateGrid(int cell_tile, int cells_size_count, float cell_size);

  void validateParams() const;
  bool isRenderedIntoShadows() const { return parameters.renderIntoShadows && layer == rendinst::LayerFlag::Opaque; }

  void addBombHole(const Point3 &point, const float radius);
};

class GpuObjects
{
private:
  struct LayerData
  {
    eastl::vector<IPoint2> offsetsAndCounts;
    eastl::vector<uint16_t> objectIds;
    uint32_t objectLodOffsets[MAX_LODS];
  };

  enum
  {
    GPUOBJ_LAYER_OPAQUE,
    GPUOBJ_LAYER_DECAL,
    GPUOBJ_LAYER_TRANSPARENT,
    GPUOBJ_LAYER_DISTORSION,
    GPUOBJ_LAYER_COUNT
  };

  struct CascadeData
  {
    ////// generate indirect input
    BufPtr generateIndirectParamsBuffer; // lod ranges, tile sizes, vertex/index counts per lod
                                         ////// dispatch indirect input
    BufPtr dispatchCountBuffer;          // counts for groupthreads during indirect generation
    BufPtr lodOffsetsBuffer;             // lod offsets buffer for indirect drawing and indirect generation
    BufPtr indirectParamsBuffer;         // final sizes, corner pos and etc for groupthreads
                                         ////// draw indirect input
    BufPtr drawIndirectBuffer;           // instance counts and vertex/index num for indirect drawing
    BufPtr matricesBuffer;               // final buffer for rendering

    Frustum frustum;

    carray<LayerData, GPUOBJ_LAYER_COUNT> layers;

    bool buffersCreated;
    bool gpuInstancing;
    bool isDeleted;
  };
  eastl::vector<CascadeData> cascades;
  eastl::vector<ObjectManager> objects;
  eastl::vector<uint16_t> objectIds;
  uint32_t maxRowsCountInBuffer = 0;
  eastl::unique_ptr<ComputeShaderElement> gpuInstancingGenerateIndirect;
  eastl::unique_ptr<ComputeShaderElement> gpuInstancingRebuildRelems;

  static BVHConnection *bvhConnection;

  struct GatherBuffersJob final : public cpujobs::IJob
  {
    GpuObjects *thiz = nullptr;
    int threadIx = 0;
    void doJob() override;
  } gatherBuffersJobs[4];

public:
  static void setBVHConnection(BVHConnection *bvh_connection) { bvhConnection = bvh_connection; }

  ~GpuObjects();

  void invalidate()
  {
    for (ObjectManager &object : objects)
    {
      object.resetGrid();
      object.invalidate();
    }

    for (CascadeData &cascade : cascades)
    {
      cascade.buffersCreated = false;
    }
  }
  void invalidateBBox(const BBox2 &bbox);
  int addCascade();
  void releaseCascade(int cascade);
  int layerRIOnLayerGpuobjRemap(rendinst::LayerFlag layer_ri) const
  {
    switch (layer_ri)
    {
      case rendinst::LayerFlag::Opaque: return GPUOBJ_LAYER_OPAQUE;
      case rendinst::LayerFlag::Transparent: return GPUOBJ_LAYER_TRANSPARENT;
      case rendinst::LayerFlag::Decals: return GPUOBJ_LAYER_DECAL;
      case rendinst::LayerFlag::Distortion: return GPUOBJ_LAYER_DISTORSION;
      default: break;
    }
    G_ASSERT_FAIL("GPU objects don't handle this rendinst layer: %d", eastl::to_underlying(layer_ri));
    return GPUOBJ_LAYER_OPAQUE;
  }
  void addObject(const char *name, const int id, int cell_tile, int grid_size, float cell_size, float bounding_sphere_radius,
    dag::ConstSpan<float> dist_sq_lod, const PlacingParameters &parameters)
  {
    if (eastl::find(objectIds.begin(), objectIds.end(), id) != objectIds.end())
      return;

    if (objects.size() == MAX_GPU_OBJECTS)
    {
      logerr("GPUObjects: Trying to add object %s with id %d, but reached maximum number of objects (%d)", name, id, MAX_GPU_OBJECTS);
      return;
    }

    objects.emplace_back(name, id, cell_tile, grid_size, cell_size, bounding_sphere_radius, dist_sq_lod, parameters);
    objectIds.push_back(id);
    for (CascadeData &cascade : cascades)
    {
      cascade.matricesBuffer.close();
      cascade.generateIndirectParamsBuffer.close();
      cascade.dispatchCountBuffer.close();
      cascade.lodOffsetsBuffer.close();
      cascade.indirectParamsBuffer.close();
      cascade.drawIndirectBuffer.close(); // instance counts and vertex/index num for indirect drawing
      cascade.buffersCreated = false;
    }
  }
  void clearObjects()
  {
    objects.clear();
    objectIds.clear();
  }

  void addBombHole(const Point3 &point, const float radius)
  {
    for (ObjectManager &object : objects)
      object.addBombHole(point, radius);
  }

  void update(const Point3 &origin);

  void prepareGpuInstancing(int cascade);
  void setGpuInstancingRelemParams(int cascade);
  void rebuildGpuInstancingRelemParams();

  void clearCascade(int cascade);
  void beforeDraw(rendinst::RenderPass render_pass, int cascade, const Frustum &frustum, const Occlusion *occlusion,
    const char *mission_name, const char *map_name, bool gpu_instancing = false);

  Sbuffer *getBuffer(int cascade, rendinst::LayerFlag layer);
  Sbuffer *getIndirectionBuffer(int cascade);
  Sbuffer *getOffsetsBuffer(int cascade);
  bool getGpuInstancing(int cascade);

  dag::ConstSpan<IPoint2> getObjectsOffsetsAndCount(int cascade, rendinst::LayerFlag layer) const
  {
    int layerIdx = layerRIOnLayerGpuobjRemap(layer);
    return cascades[cascade].layers[layerIdx].offsetsAndCounts;
  }
  dag::ConstSpan<uint16_t> getObjectsIds(int cascade, rendinst::LayerFlag layer) const
  {
    int layerIdx = layerRIOnLayerGpuobjRemap(layer);
    return cascades[cascade].layers[layerIdx].objectIds;
  }
  dag::ConstSpan<uint32_t> getObjectsLodOffsets(int cascade, rendinst::LayerFlag layer) const
  {
    int layerIdx = layerRIOnLayerGpuobjRemap(layer);
    return make_span_const(cascades[cascade].layers[layerIdx].objectLodOffsets, MAX_LODS);
  }

  void changeParameters(int idx, const PlacingParameters &parameters)
  {
    for (int i = 0; i < objectIds.size(); ++i)
      if (objectIds[i] == idx)
      {
        objects[i].setParameters(parameters);
        return;
      }
  }

  void changeGrid(int idx, int cell_tile, int grid_size, float cell_size)
  {
    for (int i = 0; i < objectIds.size(); ++i)
      if (objectIds[i] == idx)
      {
        objects[i].recreateGrid(cell_tile, grid_size, cell_size);
        objects[i].invalidate();
        return;
      }
  }

  void getCellData(int idx, int &cell_tile, int &cells_size_count, float &cell_size)
  {
    for (int i = 0; i < objectIds.size(); ++i)
      if (objectIds[i] == idx)
      {
        objects[i].getCellData(cell_tile, cells_size_count, cell_size);
        return;
      }
  }

  void getParameters(int idx, PlacingParameters &p)
  {
    for (int i = 0; i < objectIds.size(); ++i)
      if (objectIds[i] == idx)
      {
        objects[i].getParameters(p);
        return;
      }
  }
};

} // namespace gpu_objects
