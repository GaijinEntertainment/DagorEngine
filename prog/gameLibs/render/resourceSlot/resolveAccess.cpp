// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/resolveAccess.h>
#include <render/resourceSlot/state.h>
#include <render/resourceSlot/actions.h>

#include <detail/storage.h>
#include <detail/graph.h>
#include <detail/slotUsage.h>
#include <detail/unregisterAccess.h>

#include <generic/dag_enumerate.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_graphStat.h>
#include <EASTL/string.h>


CONSOLE_BOOL_VAL("resource_slot", register_nodes, false);
CONSOLE_BOOL_VAL("resource_slot", register_nodes_every_frame, false);
CONSOLE_BOOL_VAL("resource_slot", debug_topological_order, false);

static constexpr int CREATE_PRIORITY = INT_MIN;

template <typename UsageMap>
static inline void add_node_to_usage_list(UsageMap &slot_usage, const resource_slot::detail::Storage &storage,
  const resource_slot::detail::NodeDeclaration &node)
{
  if (debug_topological_order)
    debug("addNode \"%s\"", storage.nodeMap.name(node.id));

  for (const resource_slot::detail::SlotAction &declaration : node.actionList)
  {
    eastl::visit(
      [&slot_usage, &node, &storage](const auto &decl) {
        typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
        if constexpr (eastl::is_same_v<ValueT, resource_slot::Create>)
        {
          slot_usage[decl.slot].push_back({node.id, CREATE_PRIORITY, false});

          if (debug_topological_order)
            debug("node \"%s\" creates slot \"%s\" with resource \"%s\"", storage.nodeMap.name(node.id),
              resource_slot::detail::slot_map.name(decl.slot), resource_slot::detail::resource_map.name(decl.resource));
        }
        else if constexpr (eastl::is_same_v<ValueT, resource_slot::Update>)
        {
          slot_usage[decl.slot].push_back({node.id, decl.priority, false});

          if (debug_topological_order)
            debug("node \"%s\" updates slot \"%s\" with resource \"%s\", priority %d", storage.nodeMap.name(node.id),
              resource_slot::detail::slot_map.name(decl.slot), resource_slot::detail::resource_map.name(decl.resource), decl.priority);
        }
        else if constexpr (eastl::is_same_v<ValueT, resource_slot::Read>)
        {
          slot_usage[decl.slot].push_back({node.id, decl.priority, true});

          if (debug_topological_order)
            debug("node \"%s\" reads slot \"%s\", priority %d", storage.nodeMap.name(node.id),
              resource_slot::detail::slot_map.name(decl.slot), decl.priority);
        }
      },
      declaration);
  }
}

template <typename UsageMap>
static inline void add_all_nodes_to_usage_list(UsageMap &slot_usage, const resource_slot::detail::Storage &storage)
{
  for (const resource_slot::detail::NodeDeclaration &node : storage.registeredNodes)
  {
    if (node.status != resource_slot::detail::NodeStatus::Valid)
      continue;

    add_node_to_usage_list(slot_usage, storage, node);
  }
}


template <typename UsageMap>
static inline void sort_usage_list(UsageMap &slot_usage)
{
  for (int id = 0; id < slot_usage.size(); ++id)
  {
    resource_slot::detail::SlotId slotId = resource_slot::detail::SlotId{id};
    auto &usageList = slot_usage[slotId];
    eastl::sort(usageList.begin(), usageList.end(), resource_slot::detail::LessPriority());
  }
}


template <typename UsageMap>
static inline void invalidate_node(resource_slot::detail::NodeId node_id, UsageMap &slot_usage,
  resource_slot::detail::Storage &storage);


template <typename UsageMap>
static inline void invalidate_slot(resource_slot::detail::SlotId slot_id, UsageMap &slot_usage,
  resource_slot::detail::Storage &storage)
{
  auto &usageList = slot_usage[slot_id];
  for (const resource_slot::detail::SlotUsage &usage : usageList)
    invalidate_node(usage.node, slot_usage, storage);
}


template <typename UsageMap>
static inline void invalidate_node(resource_slot::detail::NodeId node_id, UsageMap &slot_usage,
  resource_slot::detail::Storage &storage)
{
  resource_slot::detail::NodeDeclaration &node = storage.registeredNodes[node_id];

  if (node.status == resource_slot::detail::NodeStatus::Valid)
  {
    node.status = resource_slot::detail::NodeStatus::UsingInvalidSlot;
    --storage.validNodeCount;

    if (node.createsSlot)
      for (const resource_slot::detail::SlotAction &declaration : node.actionList)
        if (const resource_slot::Create *createDecl = eastl::get_if<resource_slot::Create>(&declaration))
          invalidate_slot(createDecl->slot, slot_usage, storage);
  }
}


template <typename NodeList, typename UsageMap>
static inline void validate_usage_list(UsageMap &slot_usage, resource_slot::detail::Storage &storage)
{
  for (int id = 0; id < slot_usage.size(); ++id)
  {
    resource_slot::detail::SlotId slotId = resource_slot::detail::SlotId{id};
    auto &usageList = slot_usage[slotId];

    if (usageList.empty())
      continue;

    const resource_slot::detail::SlotUsage *prevUsage = usageList.begin();
    if (storage.registeredNodes[prevUsage->node].status != resource_slot::detail::NodeStatus::Valid ||
        usageList.front().priority != CREATE_PRIORITY)
    {
      logerr("Missed 'Create' declaration for slot \"%s\" requested in node \"%s\" \nRerun with resource_slot.debug_topological_order "
             "to get more info",
        resource_slot::detail::slot_map.name(slotId), storage.nodeMap.name(usageList[0].node));
      invalidate_slot(slotId, slot_usage, storage);
      continue; // Next slot
    }

    for (const resource_slot::detail::SlotUsage &usage : usageList)
    {
      if (storage.registeredNodes[usage.node].status == resource_slot::detail::NodeStatus::UsingInvalidSlot)
        continue;

      G_ASSERT(storage.registeredNodes[usage.node].status == resource_slot::detail::NodeStatus::Valid);

      if (prevUsage != &usage)
      {
        if (usage.priority == CREATE_PRIORITY)
          logerr("Double declaration of 'Create' for slot \"%s\" in node \"%s\"; previous declaration in node \"%s\"\nRerun with "
                 "resource_slot.debug_topological_order to get more info",
            resource_slot::detail::slot_map.name(slotId), storage.nodeMap.name(usage.node), storage.nodeMap.name(usageList[0].node));
        else if (!usage.isOnlyRead && usage.priority <= prevUsage->priority)
          logerr("All declaration for slot \"%s\" should have different priority; node \"%s\" and node \"%s\" have the same priority "
                 "%d\nRerun with resource_slot.debug_topological_order to get more info",
            resource_slot::detail::slot_map.name(slotId), storage.nodeMap.name(usage.node), storage.nodeMap.name(prevUsage->node),
            usage.priority);
      }

      prevUsage = &usage;
    }
  }
}

template <typename Dependencies, typename UsageMap>
static inline void build_edges_from_usage_list(Dependencies &dependencies, const UsageMap &slot_usage,
  const resource_slot::detail::Storage &storage)
{
  for (int id = 0; id < slot_usage.size(); ++id)
  {
    resource_slot::detail::SlotId slotId = resource_slot::detail::SlotId{id};
    auto &usageList = slot_usage[slotId];
    const resource_slot::detail::SlotUsage *prevModification = usageList.begin();
    dag::RelocatableFixedVector<const resource_slot::detail::SlotUsage *, resource_slot::detail::EXPECTED_MAX_SLOT_USAGE, true,
      framemem_allocator>
      prevReads;

    for (const resource_slot::detail::SlotUsage &usage : usageList)
    {
      if (storage.registeredNodes[usage.node].status == resource_slot::detail::NodeStatus::UsingInvalidSlot)
        continue;

      G_ASSERT(storage.registeredNodes[usage.node].status == resource_slot::detail::NodeStatus::Valid);

      if (usage.priority == CREATE_PRIORITY)
        continue;

      G_ASSERT(&usage != prevModification);
      dependencies.addEdge(usage.node, prevModification->node);
      if (debug_topological_order)
        debug("addEdge \"%s\" -> modify \"%s\" for slot \"%s\"", storage.nodeMap.name(usage.node),
          storage.nodeMap.name(prevModification->node), resource_slot::detail::slot_map.name(slotId));

      if (usage.isOnlyRead)
        prevReads.push_back(&usage);
      else
      {
        for (const resource_slot::detail::SlotUsage *read : prevReads)
        {
          dependencies.addEdge(usage.node, read->node);

          debug("addEdge modify \"%s\" -> read \"%s\" for slot \"%s\"", storage.nodeMap.name(usage.node),
            storage.nodeMap.name(read->node), resource_slot::detail::slot_map.name(slotId));
        }
        prevReads.clear();

        prevModification = &usage;
      }
    }
  }
}


template <typename Dependencies, typename UsageMap, typename NodeList>
[[nodiscard]] static inline bool make_topological_sort(Dependencies &dependencies, UsageMap &slot_usage,
  resource_slot::detail::Storage &storage, NodeList &top_sort_order)
{
  // Execute topological sort
  // From https://en.wikipedia.org/wiki/Topological_sorting
  NodeList cycle;
  for (const resource_slot::detail::NodeDeclaration &node : storage.registeredNodes)
  {
    if (node.status != resource_slot::detail::NodeStatus::Valid)
      continue;

    if (dependencies.topologicalSort(node.id, top_sort_order, cycle) != Dependencies::TopologicalSortResult::SUCCESS)
    {
      eastl::string cycleString;
      for (resource_slot::detail::NodeId nodeId : cycle)
      {
        invalidate_node(nodeId, slot_usage, storage);
        cycleString.append_sprintf("%s -> ", storage.nodeMap.name(nodeId));
      }
      cycleString.resize(cycleString.size() - 4); // Remove last arrow

      logerr("Found a cycle in resource_slot graph: %s\nRerun with resource_slot.debug_topological_order to get more info",
        cycleString.c_str());

      top_sort_order.clear();
      return false;
    }
  }

  if (debug_topological_order)
  {
    debug("topological order");
    for (resource_slot::detail::NodeId nodeId : top_sort_order)
      debug("- node \"%s\"", storage.nodeMap.name(nodeId));
    debug("end of topological order");
  }

  return true;
}


template <typename NodeList>
static inline void fill_slots_state(resource_slot::detail::Storage &storage, NodeList &top_sort_order)
{
  storage.currentSlotsState = decltype(storage.currentSlotsState){resource_slot::detail::slot_map.nameCount()};

  for (resource_slot::detail::NodeId nodeId : top_sort_order)
  {
    resource_slot::detail::NodeDeclaration &node = storage.registeredNodes[nodeId];
    G_ASSERT(node.id != resource_slot::detail::NodeId::Invalid);
    G_ASSERT(node.status == resource_slot::detail::NodeStatus::Valid);

    node.resourcesBeforeNode.clear();

    // Update state after node
    for (const resource_slot::detail::SlotAction &declaration : node.actionList)
    {
      eastl::visit(
        [&storage, &node](const auto &decl) {
          typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
          if constexpr (eastl::is_same_v<ValueT, resource_slot::Create>)
            storage.currentSlotsState[decl.slot] = decl.resource;
          else if constexpr (eastl::is_same_v<ValueT, resource_slot::Update>)
          {
            node.resourcesBeforeNode[decl.slot] = storage.currentSlotsState[decl.slot];
            storage.currentSlotsState[decl.slot] = decl.resource;
          }
          else if constexpr (eastl::is_same_v<ValueT, resource_slot::Read>)
            node.resourcesBeforeNode[decl.slot] = storage.currentSlotsState[decl.slot];
        },
        declaration);
    }
  }
}

void resource_slot::resolve_access()
{
  // TODO: move all dump to separate function and dump graph on error

  for (auto &[ns, storage] : detail::storage_list)
  {

    if (register_nodes || register_nodes_every_frame)
    {
      register_nodes.set(false);
      storage.isNodeRegisterRequired = true;
    }

    if (!storage.isNodeRegisterRequired)
      continue;

    TIME_PROFILE(resource_slot__perform_access);
    storage.isNodeRegisterRequired = false;
    storage.validNodeCount = 0;

    // Reset nodes state
    for (int i = 0; i < storage.registeredNodes.size(); ++i)
    {
      detail::NodeId nodeId = detail::NodeId{i};
      detail::NodeDeclaration &node = storage.registeredNodes[nodeId];

      if (node.status == detail::NodeStatus::Pruned)
        detail::unregister_access(storage, nodeId, node.generation);

      if (node.status == resource_slot::detail::NodeStatus::Empty)
        continue;

      G_ASSERTF_CONTINUE(node.id == nodeId, "Unexpected node.id=%d for node %s<%d>", int(node.id), storage.nodeMap.name(nodeId), i);

      node.nodeHandle = dafg::NodeHandle{};
      node.status = resource_slot::detail::NodeStatus::Valid;
      ++storage.validNodeCount;
    }

    // Make topological sort
    {
      FRAMEMEM_REGION;

      typedef dag::RelocatableFixedVector<detail::SlotUsage, detail::EXPECTED_MAX_SLOT_USAGE, true, framemem_allocator> UsageList;
      typedef detail::AutoGrowVector<detail::SlotId, UsageList, detail::EXPECTED_MAX_SLOT_COUNT, false, framemem_allocator> UsageMap;
      UsageMap slotUsage{resource_slot::detail::slot_map.nameCount()};

      if (debug_topological_order)
        debug("resource_slot::resolve_access(): start graph generation");

      add_all_nodes_to_usage_list(slotUsage, storage);
      sort_usage_list(slotUsage);

      typedef dag::RelocatableFixedVector<detail::NodeId, detail::EXPECTED_MAX_NODE_COUNT, true, framemem_allocator> NodeList;
      NodeList topSortOrder;

      int prevIterationValidNodeCount = storage.validNodeCount;
      bool isFirstIteration = true;
      while (true)
      {
        G_ASSERT_RETURN(isFirstIteration || prevIterationValidNodeCount > storage.validNodeCount, );
        isFirstIteration = false;
        prevIterationValidNodeCount = storage.validNodeCount;

        validate_usage_list<NodeList, UsageMap>(slotUsage, storage);

        typedef detail::Graph<framemem_allocator> Dependencies;
        Dependencies dependencies{storage.registeredNodes.size()};
        build_edges_from_usage_list(dependencies, slotUsage, storage);

        // Topological sort
        if (!make_topological_sort<Dependencies, UsageMap, NodeList>(dependencies, slotUsage, storage, topSortOrder))
          continue; // Some nodes were removed. Have to rebuild dependency graph

        break;
      }

      fill_slots_state(storage, topSortOrder);
    }

    // Call declarations outside FRAMEMEM_REGION
    G_ASSERT(storage.registeredNodes.size() < eastl::numeric_limits<uint16_t>::max());
    for (auto [i, node] : enumerate(storage.registeredNodes))
    {
      if (node.status != resource_slot::detail::NodeStatus::Valid)
        continue;

      node.nodeHandle = node.declaration_callback(resource_slot::State{
        ns, static_cast<int>(node.id), static_cast<uint16_t>(i), static_cast<uint16_t>(storage.registeredNodes.size())});
    }
  }
}