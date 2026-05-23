// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <debug/visualization/structuresCommon.h>

#include <backend/passColoring.h>

#include <graphLayouter/graphLayouter.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>
#include <render/daFrameGraph/detail/nameSpaceNameId.h>
#include <imgui-node-editor/imgui_bezier_math.h>


namespace dafg::visualization::usergraph
{
inline constexpr PassColor UNKNOWN_PASS_COLOR = static_cast<PassColor>(-1);
inline constexpr uint32_t CULLED_OUT_NODE = static_cast<uint32_t>(-1);
inline constexpr NodeIdx INVALID_NODE_IDX = static_cast<NodeIdx>(-1);
inline constexpr int UNKNOWN_INDEX = -1;


enum class NodeColorationType
{
  None,
  Framebuffer,
  Pass,
  COUNT
};

enum class ResourceFocusType
{
  All,
  Resource,
  ResourceAndRenames,
  COUNT
};

struct Rectangle
{
  ImVec2 offset = {};
  ImVec2 size = {};
};


struct NameSpace
{
  dag::RelocatableFixedVector<NameSpaceNameId, 8> subNameSpaces;
  dag::RelocatableFixedVector<NodeNameId, 8> nodes;
  dag::RelocatableFixedVector<ResNameId, 8> resources;

  uint16_t totalNodesInSubtree = 0;
  uint16_t totalResourcesInSubtree = 0;
  uint16_t visibleResourcesInSubtree = 0;
};


enum class DependencyType : uint16_t
{
  EXPLICIT_PREVIOUS,
  EXPLICIT_FOLLOW,
  IMPLICIT_RES_MODIFY,
  IMPLICIT_RES_READ,
  IMPLICIT_RES_CONSUME,
  IMPLICIT_RES_HIST,
  COUNT
};


enum class NodeId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<NodeId>>(-1)
};

enum class ResourceId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<ResourceId>>(-1)
};

enum class DependencyId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<DependencyId>>(-1)
};

struct Node
{
  NodeNameId regId = NodeNameId::Invalid;

  PassColor passColor = UNKNOWN_PASS_COLOR;
  uint32_t executionTime = CULLED_OUT_NODE;

  ImU32 fbImColor = 0;
  ImU32 passImColor = 0;
  bool cycled = false;

  dag::RelocatableFixedVector<DependencyId, 4> inDeps;
  dag::RelocatableFixedVector<DependencyId, 4> outDeps;
  dag::RelocatableFixedVector<DependencyId, 4> inHistDeps;
  dag::RelocatableFixedVector<DependencyId, 4> outHistDeps;
};

struct Resource
{
  ResNameId regId = ResNameId::Invalid;
  bool hidden = false;
};

struct Dependency
{
  DependencyType type;
  NodeId from = NodeId::Invalid;
  NodeId to = NodeId::Invalid;
  ResourceId resource = ResourceId::Invalid;
  bool disabled = false;
  bool cycled = false;
};


enum class NodeBoxId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<NodeBoxId>>(-1)
};

struct NodeBox
{
  NameSpaceNameId nameSpace = NameSpaceNameId::Invalid;
  Rectangle rect = {};

  dag::RelocatableFixedVector<NodeId, 8> nodes;
  dag::RelocatableFixedVector<DependencyId, 16> inDeps;
  dag::RelocatableFixedVector<DependencyId, 16> outDeps;
};


enum class AnchorId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<AnchorId>>(-1)
};

using Anchor = ImVec2;

enum class RawEdgeId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<RawEdgeId>>(-1)
};

struct RawEdge
{
  NodeIdx from = INVALID_NODE_IDX;
  NodeIdx to = INVALID_NODE_IDX;
  AnchorId fromAnchor = AnchorId::Invalid;
  AnchorId toAnchor = AnchorId::Invalid;
  dag::RelocatableFixedVector<DependencyId, 8> carriedDeps;
};

struct RawObject
{
  dag::RelocatableFixedVector<RawEdgeId, 8> inEdges;
  dag::RelocatableFixedVector<RawEdgeId, 8> outEdges;
  dag::RelocatableFixedVector<AnchorId, 8> inAnchors;
  dag::RelocatableFixedVector<AnchorId, 8> outAnchors;
};

struct RawLayout
{
  dag::Vector<NodeId> userNodeIds;
  dag::Vector<NodeBoxId> nodeBoxIds;
  dag::Vector<dag::Vector<DependencyId>> outDeps;

  GraphLayout layouterResult;
  dag::Vector<RawObject> rawObjects;
  dag::Vector<ImVec2> objectsOffsets;
  dag::Vector<ImVec2> objectsSizes;
  dag::Vector<float> objectsColumnSizes;
  ImVec2 size = {};

  IdIndexedMapping<RawEdgeId, RawEdge> rawEdges;
  IdIndexedMapping<AnchorId, Anchor> anchors;
};


enum class EdgeId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<EdgeId>>(-1)
};

struct Edge
{
  dag::RelocatableFixedVector<DependencyId, 2> carriedDeps;
  dag::RelocatableFixedVector<ImCubicBezierPoints, 2> splines;
};

struct CanvasLayout
{
  ImVec2 size = {};
  IdIndexedMapping<NodeId, Rectangle> nodes;
  IdIndexedMapping<EdgeId, Edge> edges;
};

} // namespace dafg::visualization::usergraph