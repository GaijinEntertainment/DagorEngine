// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresResource.h>

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>

#include <backend/intermediateRepresentation.h>

#include <ska_hash_map/flat_hash_map2.hpp>

#include <id/idIndexedMapping.h>
#include <EASTL/optional.h>
#include <EASTL/span.h>


namespace dafg::visualization
{

class ResourseVisualizer
{
public:
  enum class SizeFormat : int
  {
    Decimal,
    Binary,
    Bytes,
    Hex
  };
  ResourseVisualizer(const InternalRegistry &intRegistry, const NameResolver &nameResolver, const intermediate::Graph &irGraph);

  void draw();

  void drawUI();

  void drawCanvas(eastl::span<const HeapIndex> heapIndicesToDisplay);

  void processPopup();

  void processInput();

  void drawBodyBackground(ImDrawList *drawList, eastl::span<const HeapIndex> heapIndicesToDisplay, float vScale,
    const IdIndexedMapping<HeapIndex, float> &heapVisualHeight, const IdIndexedMapping<HeapIndex, float> &heapBaseY,
    float header_height);
  void drawHeapLabels(ImDrawList *drawList, eastl::span<const HeapIndex> heapIndicesToDisplay,
    const IdIndexedMapping<HeapIndex, float> &heapVisualHeight, const IdIndexedMapping<HeapIndex, float> &heapBaseY,
    float header_height);
  void drawHeader(ImDrawList *drawList, float header_height);


  void updateVisualization();

  void clearResourcePlacements();
  void recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size, bool is_cpu);
  void clearResourceBarriers();
  void recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier);

private:
  const InternalRegistry &registry;
  const NameResolver &nameResolver;
  const intermediate::Graph &intermediateGraph;


  // vars from old visualizator
private:
  static constexpr int RES_RECORD_WINDOW = 2;

  IdIndexedMapping<intermediate::ResourceIndex, IRResourceInfo> irResourceInfos;
  IdIndexedMapping<HeapIndex, dag::Vector<intermediate::ResourceIndex>> heapToResources;
  IdIndexedMapping<HeapIndex, int> resourceHeapSizes;
  IdIndexedMapping<HeapIndex, bool> cpuHeapFlags;
  IdIndexedMapping<intermediate::NodeIndex, eastl::string> nodeNames;
  struct InterestingNodes
  {
    dag::Vector<bool> physical;
    dag::Vector<bool> physicalBarriers;
    dag::Vector<bool> logical;
    dag::Vector<bool> logicalBarriers;
    dag::Vector<bool> ir;
    dag::Vector<bool> irBarriers;
  };
  InterestingNodes gpuInteresting;
  InterestingNodes cpuInteresting;
  ColumnLayout currentLayout;
  const ColumnLayout *activeLayout = nullptr;
  bool layoutIsCpu = false;
  bool layoutCompact = false;
  bool layoutUsages = false;
  bool layoutBarriers = false;
  bool layoutIR = false;
  bool compactView = true;

  dag::Vector<ResourcePlacementEntry> resourcePlacemantEntries;
  dag::Vector<ResourceBarrierEntry> resourceBarrierEntries;

  const float NODE_HORIZONTAL_HALFSIZE = 20.0f;
  const float NODE_HORIZONTAL_SIZE = 2.0f * NODE_HORIZONTAL_HALFSIZE;

  size_t nodeCount = 0;

  ImVec2 viewScrolling = ImVec2(100.f, 100.f);
  float viewScaling = 1.0f;
  float verticalScaling = 400.0f;

  bool showBarriers = true;
  bool showUsages = true;
  bool showIRResources = false;
  bool showTooltips = false;
  SizeFormat sizeFormat = SizeFormat::Decimal;

  eastl::optional<intermediate::ResourceIndex> hoveredIRResource;
  int hoveredFrontendIdx = -1;
  int hoveredResourceOwnerFrame = 0;
  int hoveredResourceHeap = 0;

  ImVec2 initialPos;
  ImVec2 viewOffset;
  float viewWidth;
  float nodeHeaderHeight = 0.f;
};

} // namespace dafg::visualization