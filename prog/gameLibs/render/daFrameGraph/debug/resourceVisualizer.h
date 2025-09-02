// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerStructuresResource.h>

#include <frontend/internalRegistry.h>

#include <backend/intermediateRepresentation.h>

#include <ska_hash_map/flat_hash_map2.hpp>


namespace dafg::visualization
{

class ResourseVisualizer
{
public:
  ResourseVisualizer(const InternalRegistry &intRegistry, const intermediate::Graph &irGraph);

  void draw();

  void drawUI();

  void drawCanvas();

  void processPopup();

  void processInput();


  void updateVisualization();

  void clearResourcePlacements();
  void recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size);
  void clearResourceBarriers();
  void recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier);

private:
  const InternalRegistry &registry;
  const intermediate::Graph &intermediateGraph;


  // vars from old visualizator
private:
  static constexpr int RES_RECORD_WINDOW = 2;

  eastl::array<ska::flat_hash_map<ResNameId, ResourceInfo>, RES_RECORD_WINDOW> perResourceInfos;
  dag::Vector<int> resourceHeapSizes;
  dag::Vector<eastl::string> nodeNames;

  dag::Vector<ResourcePlacementEntry> resourcePlacemantEntries;
  dag::Vector<ResourceBarrierEntry> resourceBarrierEntries;

  const float NODE_HORIZONTAL_HALFSIZE = 50.0f;
  const float NODE_HORIZONTAL_SIZE = 2.0f * NODE_HORIZONTAL_HALFSIZE;

  size_t nodeCount = 0;

  ImVec2 viewScrolling = ImVec2(NODE_HORIZONTAL_HALFSIZE, 0.0f);
  float viewScaling = 1.0f;
  float verticalScaling = 400.0f;

  bool showBarriers = false;
  bool showTooltips = false;

  dafg::ResNameId hoveredResourceId = dafg::ResNameId::Invalid;
  int hoveredResourceOwnerFrame = 0;

  ImVec2 initialPos;
  ImVec2 viewOffset;
  float viewWidth;
};

} // namespace dafg::visualization