// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerStructuresCommon.h>

#include <backend/passColoring.h>

#include <graphLayouter/graphLayouter.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>
#include <render/daFrameGraph/detail/nameSpaceNameId.h>


namespace dafg::visualization
{
// structures from old visualizator
enum class EdgeType
{
  EXPLICIT_PREVIOUS,
  EXPLICIT_FOLLOW,
  IMPLICIT_RES_DEP,
  IMPLICIT_MOD_CHAIN,
  VISIBLE_COUNT,
  INVISIBLE = VISIBLE_COUNT, // Use for some implicit ordering in visualization
  COUNT
};

struct EdgeTypeDebugInfo
{
  eastl::string idName;
  eastl::string readableName;
  // Color string as per dotfile format
  eastl::string dotColor;
  uint32_t imguiColor = 0;
  bool visible = true;
};

struct EdgeData
{
  NodeIdx destination;
  EdgeType type;
  ResNameId resId;
};

struct VisualizedEdge
{
  float thickness = 0.0f;
  eastl::fixed_vector<EdgeType, 4> types;
  eastl::fixed_vector<ResNameId, 8> resIds;
  NodeIdx source = -1;
  NodeIdx destination = -1;
};

using DebugPassColoration = IdIndexedMapping<NodeNameId, PassColor>;

struct NameSpaceContents
{
  dag::RelocatableFixedVector<NameSpaceNameId, 8> subNameSpaces;
  dag::RelocatableFixedVector<ResNameId, 16> resources;

  uint16_t totalResourcesInSubtree = eastl::numeric_limits<uint16_t>::max();
  uint16_t visibleResourcesInSubtree = 0;
};

enum class FocusedResourceAction
{
  ShowAll,
  FocusOnResource,
  FocusOnResourceAndRenames,
};

struct FocusedResource
{
  dafg::ResNameId id;
  bool hasRenames;
  FocusedResourceAction action;
};

enum class ColorationType
{
  None,
  Framebuffer,
  Pass
};


// structures to write visualization with
struct UserNodeV : VisualElement
{};

struct UserResourceV : VisualElement
{};

struct UserEdgeV : VisualElement
{};

} // namespace dafg::visualization