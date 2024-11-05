// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <render/toroidalHelper.h>
#include <render/daBfg/nodeHandle.h>
#include <render/lightProbe.h>


class Occlusion;

namespace scene
{
class TiledScene;
}

class IndoorProbeScenes;

class IndoorProbeManager
{
  eastl::unique_ptr<IndoorProbeScenes> shapesContainer;
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
  EventQueryHolder cullingDoneEvent;
  UniqueBuf readbackBuffer;
  Ptr<ShaderMaterial> cullingMat;
  Ptr<ShaderElement> cullingElem;
  bool needsToReadBackCulling = false;
  int framesSinceLastCulling = 0;
  static constexpr int GPU_CULLING_FREQUENCY = 6;

  UniqueBufHolder indoorProbeVisibilityMask; // For debug

  void initLightProbes();

  using UintLocalVector = eastl::vector<uint32_t, framemem_allocator>;
  using Point4LocalVector = eastl::vector<Point4, framemem_allocator>;
  using MatrixLocalVector = eastl::vector<mat44f, framemem_allocator>;
  void updateActiveProbes(const UintLocalVector &node_indices);
  bool tryToActivateProbe(uint32_t probe_index);
  void setActiveProbe(int probe_idx, uint32_t active_slot);
  Point4LocalVector packMatricesAndProbesData(
    dag::ConstSpan<uint32_t> probe_indices, dag::ConstSpan<mat44f> matrices, dag::ConstSpan<uint32_t> shapeTypes, const int max_count);
  void setVisibleBoxesData(
    dag::ConstSpan<uint32_t> probe_indices, dag::ConstSpan<mat44f> matrices, dag::ConstSpan<uint32_t> shapeTypes);
  void fillCellsBuffer();
  void initBuffers();

  enum
  {
    PROBE_IN_PROGRESS,
    LAST_UPDATED_PROBE,
    ACTIVE_PROBE
  };
  eastl::vector<uint32_t> activeProbesState;

  dabfg::NodeHandle tryStartUpdateNode;
  dabfg::NodeHandle cpuCheckNode;
  dabfg::NodeHandle completePreviousFrameReadbacksNode;
  dabfg::NodeHandle createVisiblePixelsCountNode;
  dabfg::NodeHandle calculateVisiblePixelsCountNode;
  dabfg::NodeHandle initiateVisiblePixelsCountReadbackNode;
  dabfg::NodeHandle setVisibleBoxesDataNode;

public:
  explicit IndoorProbeManager(LightProbeSpecularCubesContainer *container);
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

private:
  void registerNodes(uint32_t allProbesOnLevel);

  void updateVisibleProbesData(Occlusion *occlusion,
    const Point3 &view_pos,
    const TMatrix4 &view_mat,
    const Driver3dPerspective &persp,
    ManagedBufView visible_pixels_count);
  auto cpuCheck(Occlusion *occlusion, const Point3 &view_pos, const TMatrix4 &view_mat, const Driver3dPerspective &persp);
  void completeCullingReadback();
};
