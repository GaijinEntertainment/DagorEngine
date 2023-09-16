#pragma once

#include <dag/dag_vector.h>

namespace resource_slot::detail
{

enum class NodeId : int
{
  Invalid = -1
};

typedef dag::Vector<NodeId> NodeList;

enum class SlotId : int
{
  Invalid = -1
};

enum class ResourceId : int
{
  Invalid = -1
};

} // namespace resource_slot::detail