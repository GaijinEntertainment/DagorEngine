// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerStructuresCommon.h>

#include <backend/intermediateRepresentation.h>


namespace dafg::visualization
{

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

} // namespace dafg::visualization