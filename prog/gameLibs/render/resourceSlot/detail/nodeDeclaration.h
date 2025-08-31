// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/resourceSlot/detail/registerAccess.h>
#include <render/resourceSlot/detail/actionList.h>
#include <render/resourceSlot/actions.h>
#include <detail/expectedLimits.h>

#include <render/daFrameGraph/nodeHandle.h>
#include <generic/dag_fixedVectorMap.h>


namespace das
{
class Context;
} // namespace das

namespace resource_slot::detail
{

enum struct NodeStatus : int
{
  Empty,
  Valid,
  UsingInvalidSlot,
  Pruned
};

struct NodeDeclaration
{
  NodeStatus status = NodeStatus::Empty;
  NodeId id = NodeId::Invalid;
  unsigned generation = 0;
  bool createsSlot = false;
  ActionList actionList;
  resource_slot::detail::AccessCallback declaration_callback;
  dafg::NodeHandle nodeHandle;
  dag::FixedVectorMap<SlotId, ResourceId, EXPECTED_MAX_DECLARATIONS> resourcesBeforeNode;
  const das::Context *context = nullptr;

  explicit NodeDeclaration(unsigned gen) : generation(gen) {}
  NodeDeclaration(NodeId node_id, unsigned gen, ActionList &&action_list, resource_slot::detail::AccessCallback decl_cb) :
    status(NodeStatus::Valid),
    id(node_id),
    generation(gen),
    actionList(eastl::move(action_list)),
    declaration_callback(eastl::move(decl_cb))
  {}

  NodeDeclaration() = default;
  NodeDeclaration(NodeDeclaration &&) = default;
  NodeDeclaration &operator=(NodeDeclaration &&) = default;
  NodeDeclaration(const NodeDeclaration &) = delete;
  NodeDeclaration &operator=(const NodeDeclaration &) = delete;
};
} // namespace resource_slot::detail
DAG_DECLARE_RELOCATABLE(resource_slot::detail::NodeDeclaration);