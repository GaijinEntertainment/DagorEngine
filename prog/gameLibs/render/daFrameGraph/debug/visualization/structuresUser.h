// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresCommon.h>

#include <backend/passColoring.h>

#include <graphLayouter/graphLayouter.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>
#include <render/daFrameGraph/detail/nameSpaceNameId.h>


namespace dafg::visualization::usergraph
{
// structures from old visualizator

enum class ColorationType
{
  None,
  Framebuffer,
  Pass
};


// new structures

inline constexpr PassColor UNKNOWN_PASS_COLOR = static_cast<PassColor>(-1);
inline constexpr uint32_t CULLED_OUT_NODE = static_cast<uint32_t>(-1);
inline constexpr int UNKNOWN_INDEX = -1;

enum class ResourceFocusType
{
  All,
  Resource,
  ResourceAndRenames,
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

struct DependencyTypeInfo
{
  eastl::string name;
  uint32_t imguiColor = 0;
  bool visible = true;
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

enum class NodeBoxDependencyId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<NodeBoxDependencyId>>(-1)
};

struct NodeBox
{
  NameSpaceNameId nameSpace = NameSpaceNameId::Invalid;

  dag::RelocatableFixedVector<NodeId, 8> nodes;
  dag::RelocatableFixedVector<NodeBoxDependencyId, 8> inDeps;
  dag::RelocatableFixedVector<NodeBoxDependencyId, 8> outDeps;
};

struct NodeBoxDependency
{
  NodeBoxId from = NodeBoxId::Invalid;
  NodeBoxId to = NodeBoxId::Invalid;
  dag::Vector<DependencyId> carriedDeps;
};


struct Edge
{
  NodeIdx from;
  NodeIdx to;
  dag::RelocatableFixedVector<DependencyId, 8> carriedDeps;
};

struct CubicBezierWithNormals
{
  ImVec2 P0;
  ImVec2 P1;
  ImVec2 P2;
  ImVec2 P3;
  ImVec2 norm0;
  ImVec2 norm1;
};

struct Layout
{
  uint32_t objectsCount = 0;                      // Number of all objects in layout
  uint32_t edgesCount = 0;                        // Number of all edges in layout
  dag::Vector<dag::Vector<NodeIdx>> objectsRanks; // Objects indices, separated by columns in top-down order
  dag::Vector<Edge> edges;                        // List of all edges between objects

  ImVec2 size;                        // Layout bounding box size from left-top to right-bottom corner
  dag::Vector<ImVec2> objectsOffsets; // Offsets of all objects
  dag::Vector<ImVec2> objectsSizes;   // Sizes of all objects
  dag::Vector<dag::RelocatableFixedVector<CubicBezierWithNormals, 4>> edgesSplines; // Sets of splines, that represent each edge
};

} // namespace dafg::visualization::usergraph