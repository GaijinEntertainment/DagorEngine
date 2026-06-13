// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresCommon.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>

#include <EASTL/fixed_string.h>
#include <EASTL/variant.h>

#include <generic/dag_relocatableFixedVector.h>
#include <dag/dag_vectorSet.h>

#include <id/idIndexedMapping.h>

#include <imgui.h>


namespace dafg::visualization::irgraph
{
inline constexpr PassColor UNKNOWN_PASS_COLOR = static_cast<PassColor>(-1);
inline constexpr uint32_t UNKNOWN_VALUE = static_cast<uint32_t>(-1);


struct Rectangle
{
  ImVec2 offset = {};
  ImVec2 size = {};
};


enum class NodeId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<NodeId>>(-1)
};

enum class ResourceId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<ResourceId>>(-1)
};

enum class OrderingId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<OrderingId>>(-1)
};

enum class UsageId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<UsageId>>(-1)
};


struct Node
{
  intermediate::NodeIndex irIndex = static_cast<intermediate::NodeIndex>(-1);
  uint32_t executionTime = UNKNOWN_VALUE;
  uint32_t renderPassNumber = UNKNOWN_VALUE;

  dag::RelocatableFixedVector<OrderingId, 16> previous;
  dag::RelocatableFixedVector<OrderingId, 16> following;

  dag::RelocatableFixedVector<UsageId, 16> resourceUsages;
};

struct Resource
{
  intermediate::ResourceIndex irIndex = static_cast<intermediate::ResourceIndex>(-1);

  NodeId firstUser = NodeId::Invalid;
  NodeId lastUser = NodeId::Invalid;

  dag::RelocatableFixedVector<UsageId, 16> usages;
};

struct Ordering
{
  NodeId from = NodeId::Invalid;
  NodeId to = NodeId::Invalid;
};

struct Usage
{
  NodeId node = NodeId::Invalid;
  ResourceId resource = ResourceId::Invalid;
  intermediate::Request irRequest = {};
};


struct CanvasLayout
{
  IdIndexedMapping<NodeId, Rectangle> nodes;
  IdIndexedMapping<ResourceId, Rectangle> resources;
  IdIndexedMapping<OrderingId, Rectangle> orderings;
  IdIndexedMapping<UsageId, Rectangle> usages;
};

} // namespace dafg::visualization::irgraph