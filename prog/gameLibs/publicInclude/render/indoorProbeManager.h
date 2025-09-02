//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <render/toroidalHelper.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <render/lightProbe.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/dag_hlsl_floatx.h>
#include "indoor_probes_const.hlsli"


class Occlusion;

namespace scene
{
class TiledScene;
}

class IndoorProbeScenes;

using CpuIndices = dag::RelocatableFixedVector<uint32_t, NON_CELL_PROBES_COUNT>;
using CpuMatrices = dag::RelocatableFixedVector<mat44f, NON_CELL_PROBES_COUNT>;

class IndoorProbeManager;
class IIndoorProbeNodes
{
public:
  virtual void registerNodes(uint32_t allProbesOnLevel, IndoorProbeManager *owner) = 0;
  virtual void clearNodes() = 0;
};

class IndoorProbeManager
{
public:
  eastl::unique_ptr<IndoorProbeScenes> shapesContainer;

  EventQueryHolder cullingDoneEvent;
  UniqueBuf readbackBuffer;
  Ptr<ShaderMaterial> cullingMat;
  Ptr<ShaderElement> cullingElem;
  bool needsToReadBackCulling = false;
  int framesSinceLastCulling = 0;
  static constexpr int GPU_CULLING_FREQUENCY = 6;

private:
  LightProbeSpecularCubesContainer *cubesContainer;
  UniqueBufHolder indoorActiveProbesData;
  UniqueBufHolder indoorVisibleProbesData;
  UniqueBufHolder cellClusters;
  BufPtr indoorActiveProbesStaging;
  eastl::vector<uint32_t> probeIdxToNodeIdx;
  eastl::vector<Point3> probesPositions;
  eastl::vector<LightProbe> activeProbes;
  eastl::vector<uint32_t> activeProbesUsageCounter;
  eastl::vector<int> activeProbeIds;
  ToroidalHelper toroidalInfo;
  eastl::unique_ptr<ComputeShaderElement> clusterizator;
  float lastY;

  UniqueBufHolder allIndoorProbeBoxes;
  UniqueBufHolder allIndoorProbePosAndCubes;

  UniqueBufHolder indoorProbeVisibilityMask; // For debug

  void initLightProbes();

  using UintLocalVector = eastl::vector<uint32_t, framemem_allocator>;
  using Point4LocalVector = eastl::vector<Point4, framemem_allocator>;
  using MatrixLocalVector = eastl::vector<mat44f, framemem_allocator>;
  void updateActiveProbes(const UintLocalVector &node_indices);
  bool tryToActivateProbe(uint32_t probe_index);
  void setActiveProbe(int probe_idx, uint32_t active_slot);
  Point4LocalVector packMatricesAndProbesData(dag::ConstSpan<uint32_t> probe_indices, dag::ConstSpan<mat44f> matrices,
    dag::ConstSpan<uint32_t> shapeTypes, const int max_count);
  void fillCellsBuffer();
  void initBuffers();

  enum
  {
    PROBE_IN_PROGRESS,
    LAST_UPDATED_PROBE,
    ACTIVE_PROBE
  };
  eastl::vector<uint32_t> activeProbesState;


public:
  IndoorProbeManager(LightProbeSpecularCubesContainer *container, IIndoorProbeNodes *nodes);
  ~IndoorProbeManager();

  // `this` is captured inside of a framegraph node on construction,
  // so this class has to be "pinned".
  IndoorProbeManager(const IndoorProbeManager &) = delete;
  IndoorProbeManager(IndoorProbeManager &&) = delete;
  IndoorProbeManager &operator=(const IndoorProbeManager &) = delete;
  IndoorProbeManager &operator=(IndoorProbeManager &&) = delete;

  void resetScene(eastl::unique_ptr<IndoorProbeScenes> &&scene_ptr);
  void invalidate();
  void update(const Point3 &new_origin);
  int getActiveProbesCount() const { return activeProbes.size(); }
  const scene::TiledScene *getEnviProbeBoxes() const;

  const IndoorProbeScenes *getIndoorProbeScenes() const;
  eastl::unique_ptr<IndoorProbeScenes> unloadScene();
  bool isWorldPosInIndoorProbe(const Point3 &world_pos) const;
  void ensureDebugBuffersExist();

  void setVisibleBoxesData(dag::ConstSpan<uint32_t> probe_indices, dag::ConstSpan<mat44f> matrices,
    dag::ConstSpan<uint32_t> shapeTypes);
  eastl::tuple<CpuMatrices, CpuIndices, CpuIndices, bool> cpuCheck(Occlusion *occlusion, const Point3 &view_pos,
    const TMatrix4 &view_mat, const Driver3dPerspective &persp);
  void completeCullingReadback();

private:
  IIndoorProbeNodes *indoorProbeNodes = nullptr;

  void updateVisibleProbesData(Occlusion *occlusion, const Point3 &view_pos, const TMatrix4 &view_mat,
    const Driver3dPerspective &persp, ManagedBufView visible_pixels_count);
};
