// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_string.h>
#include <EASTL/variant.h>

#include <generic/dag_relocatableFixedVector.h>
#include <dag/dag_vectorSet.h>

#include <id/idIndexedMapping.h>

#include <imgui.h>


[[maybe_unused]] constexpr auto IMGUI_WINDOW_GROUP_FG2 = "FrameGraph WIP";
[[maybe_unused]] constexpr auto IMGUI_USG_WIN_NAME = "User Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_IRG_WIN_NAME = "IR Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_RES_WIN_NAME = "Resourse Visualizer";
[[maybe_unused]] constexpr auto IMGUI_TEX_WIN_NAME = "Texture Visualizer";


namespace dafg::visualization
{

enum class NodeId : uint16_t
{
  INVALID = static_cast<eastl::underlying_type_t<NodeId>>(-1)
};

enum class ResId : uint16_t
{
  INVALID = static_cast<eastl::underlying_type_t<ResId>>(-1)
};

enum class EdgeId : uint16_t
{
  INVALID = static_cast<eastl::underlying_type_t<EdgeId>>(-1)
};

using ElementID = eastl::variant<eastl::monostate, NodeId, ResId, EdgeId>;

using NodeName = eastl::fixed_string<char, 64>;

using ResName = eastl::fixed_string<char, 128>;


struct VisualElement
{
  bool isVisible = true;
  bool focused = false;
  bool outOfFocus = false;
};


struct CanvasCamera
{
  const ImVec2 initCanvasOffset = ImVec2(0.f, 0.f);
  const int initCanvasScaleId = 0;

  ImVec2 canvasOffset = initCanvasOffset;
  int curCanvasScaleId = initCanvasScaleId;

  const dag::RelocatableFixedVector<float, 16> canvasScales = {1.0f};

public:
  float getZoom() const { return canvasScales[curCanvasScaleId]; }

  void zoomIn() { curCanvasScaleId = min<int>(curCanvasScaleId + 1, canvasScales.size() - 1); }
  void zoomOut() { curCanvasScaleId = max<int>(curCanvasScaleId - 1, 0); }

  void reset()
  {
    canvasOffset = initCanvasOffset;
    curCanvasScaleId = initCanvasScaleId;
  }
};

struct GraphSubset
{
  dag::VectorSet<NodeId> nodes;
  dag::VectorSet<ResId> resources;
  dag::VectorSet<EdgeId> edges;

public:
  void reset()
  {
    nodes.clear();
    resources.clear();
    edges.clear();
  }
};

struct SetLayout
{
  IdIndexedMapping<NodeId, ImVec2> nodesGridCoords;
  IdIndexedMapping<ResId, eastl::pair<ImVec2, ImVec2>> resGridBounds;
  IdIndexedMapping<EdgeId, eastl::pair<ImVec2, ImVec2>> edgesFrTo;

public:
  void reset()
  {
    nodesGridCoords.clear();
    resGridBounds.clear();
    edgesFrTo.clear();
  }
};


struct GraphView
{
  CanvasCamera camera;
  GraphSubset elementsSet;
  SetLayout elementsLayout;

public:
  ImVec2 &getOffset() { return camera.canvasOffset; }
  float getZoom() const { return camera.getZoom(); }
  void zoomIn() { camera.zoomIn(); }
  void zoomOut() { camera.zoomOut(); }

  void resetView() { camera.reset(); }

  void clearData()
  {
    camera.reset();
    elementsSet.reset();
    elementsLayout.reset();
  }
};

} // namespace dafg::visualization