#pragma once

#include <render/resourceSlot/detail/registerAccess.h>
#include <detail/accessDecl.h>
#include <detail/expectedLimits.h>

#include <render/daBfg/nodeHandle.h>
#include <generic/dag_fixedVectorMap.h>

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
  const char *source_location = nullptr;
  bool createsSlot = false;
  AccessDeclList action_list;
  resource_slot::detail::AccessCallback declaration_callback;
  dabfg::NodeHandle nodeHandle;
  dag::FixedVectorMap<SlotId, ResourceId, EXPECTED_MAX_DECLARATIONS> resourcesBeforeNode;

  explicit NodeDeclaration(unsigned gen) : generation(gen) {}
  NodeDeclaration(NodeId node_id, unsigned gen, const char *source_loc, resource_slot::detail::AccessCallback decl_cb) :
    status(NodeStatus::Valid), id(node_id), generation(gen), source_location(source_loc), declaration_callback(eastl::move(decl_cb))
  {}

  NodeDeclaration() = default;
  NodeDeclaration(NodeDeclaration &&) = default;
  NodeDeclaration &operator=(NodeDeclaration &&) = default;
  NodeDeclaration(const NodeDeclaration &) = delete;
  NodeDeclaration &operator=(const NodeDeclaration &) = delete;
};

} // namespace resource_slot::detail