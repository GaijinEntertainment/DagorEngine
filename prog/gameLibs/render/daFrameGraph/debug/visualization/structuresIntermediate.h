// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresCommon.h>

#include <backend/intermediateRepresentation.h>

#include <EASTL/fixed_string.h>
#include <EASTL/variant.h>

#include <generic/dag_relocatableFixedVector.h>
#include <dag/dag_vectorSet.h>

#include <id/idIndexedMapping.h>

#include <imgui.h>


namespace dafg::visualization::irgraph
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

struct IntermediateNode : VisualElement
{
  NodeName name;

  eastl::optional<NodeNameId> frontendNode = eastl::nullopt;
  intermediate::NodeIndex intermediateNode = static_cast<intermediate::NodeIndex>(-1);

  dag::RelocatableFixedVector<NodeId, 16> previousNodes;
  dag::RelocatableFixedVector<NodeId, 16> followingNodes;
  dag::RelocatableFixedVector<ResId, 16> resourceUsages;

  uint32_t renderPassNumber = 0;

  uint32_t multiplexingCount = 0;
};

struct IntermediateResource : VisualElement
{
  ResName name;

  dag::RelocatableFixedVector<ResNameId, 8> frontendResources;
  intermediate::ResourceIndex intermediateResource = static_cast<intermediate::ResourceIndex>(-1);

  dag::RelocatableFixedVector<NodeId, 16> requestedBy;

  NodeId firstUsage = NodeId::INVALID;
  NodeId lastUsage = NodeId::INVALID;

  uint32_t line = 0;

  uint32_t multiplexingCount = 0;
};

struct IntermediateEdge : VisualElement
{
  NodeId fromNode = NodeId::INVALID;
  NodeId toNode = NodeId::INVALID;
  ResId toRes = ResId::INVALID;
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

} // namespace dafg::visualization::irgraph