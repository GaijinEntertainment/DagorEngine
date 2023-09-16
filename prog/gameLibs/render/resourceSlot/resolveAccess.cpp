#include <render/resourceSlot/resolveAccess.h>
#include <render/resourceSlot/state.h>

#include <detail/storage.h>
#include <detail/graph.h>
#include <detail/slotUsage.h>

#include <util/dag_convar.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_graphStat.h>

CONSOLE_BOOL_VAL("resource_slot", register_nodes, false);
CONSOLE_BOOL_VAL("resource_slot", register_nodes_every_frame, false);
CONSOLE_BOOL_VAL("resource_slot", debug_topological_order, false);

static constexpr int CREATE_PRIORITY = INT_MIN;

template <typename UsageMap>
static inline void add_node_to_usage_list(UsageMap &slotUsage, const resource_slot::detail::Storage &storage,
  const resource_slot::detail::NodeDeclaration &node)
{
  if (debug_topological_order)
    debug("addNode \"%s\"", storage.nodeMap.name(node.id));

  for (const resource_slot::detail::AccessDecl &declaration : node.action_list)
  {
    eastl::visit(
      [&slotUsage, &node, &storage](const auto &decl) {
        typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
        if constexpr (eastl::is_same_v<ValueT, resource_slot::detail::CreateDecl>)
        {
          slotUsage[decl.slot].push_back({node.id, CREATE_PRIORITY, false});

          if (debug_topological_order)
            debug("node \"%s\" creates slot \"%s\" with resource \"%s\"", storage.nodeMap.name(node.id),
              storage.slotMap.name(decl.slot), storage.resourceMap.name(decl.resource));
        }
        else if constexpr (eastl::is_same_v<ValueT, resource_slot::detail::UpdateDecl>)
        {
          slotUsage[decl.slot].push_back({node.id, decl.priority, false});

          if (debug_topological_order)
            debug("node \"%s\" updates slot \"%s\" with resource \"%s\", priority %d", storage.nodeMap.name(node.id),
              storage.slotMap.name(decl.slot), storage.resourceMap.name(decl.resource), decl.priority);
        }
        else if constexpr (eastl::is_same_v<ValueT, resource_slot::detail::ReadDecl>)
        {
          slotUsage[decl.slot].push_back({node.id, decl.priority, true});

          if (debug_topological_order)
            debug("node \"%s\" reads slot \"%s\", priority %d", storage.nodeMap.name(node.id), storage.slotMap.name(decl.slot),
              decl.priority);
        }
      },
      declaration);
  }
}


template <typename Dependencies, typename UsageMap>
[[nodiscard]] static inline bool build_edges_from_usage_list(Dependencies &dependencies, UsageMap &slotUsage,
  const resource_slot::detail::Storage &storage)
{
  for (int id = 0; id < slotUsage.size(); ++id)
  {
    resource_slot::detail::SlotId slotId = resource_slot::detail::SlotId{id};
    auto &usageList = slotUsage[slotId];

    if (usageList.empty())
      continue;

    eastl::sort(usageList.begin(), usageList.end(), resource_slot::detail::LessPriority());
    G_ASSERTF_RETURN(usageList.front().priority == CREATE_PRIORITY, false, "Missed 'Create' declaration for slot \"%s\"",
      storage.slotMap.name(slotId));

    for (intptr_t i = 1; i < usageList.size(); ++i)
    {
      const resource_slot::detail::SlotUsage &usage = usageList[i];
      const resource_slot::detail::SlotUsage &prevUsage = usageList[i - 1];

      // Validation
      G_ASSERTF_RETURN(usage.priority != CREATE_PRIORITY, false,
        "Double declaration of 'Create' for slot \"%s\" in node \"%s\"; previous declaration in node \"%s\"",
        storage.slotMap.name(slotId), storage.nodeMap.name(usage.node), storage.nodeMap.name(usageList[0].node));

      G_ASSERTF(usage.isOnlyRead || usage.priority > prevUsage.priority,
        "All declaration for slot \"%s\" should have different priority; node \"%s\" and node \"%s\" have the same priority %d",
        storage.slotMap.name(slotId), storage.nodeMap.name(usage.node), storage.nodeMap.name(prevUsage.node), usage.priority);

      dependencies.addEdge(usage.node, prevUsage.node);

      if (debug_topological_order)
        debug("addEdge \"%s\" -> \"%s\" for slot \"%s\"", storage.nodeMap.name(usage.node), storage.nodeMap.name(prevUsage.node),
          storage.slotMap.name(slotId));
    }
  }

  return true;
}


template <typename Dependencies>
[[nodiscard]] static inline bool make_topological_sort(Dependencies &dependencies, const resource_slot::detail::Storage &storage,
  resource_slot::detail::NodeList &top_sorted_nodes)
{
  // Execute topological sort
  // From https://en.wikipedia.org/wiki/Topological_sorting
  resource_slot::detail::NodeList cycle;
  for (const resource_slot::detail::NodeDeclaration &node : storage.registeredNodes)
  {
    if (node.id == resource_slot::detail::NodeId::Invalid)
      continue;

    if (dependencies.topologicalSort(node.id, top_sorted_nodes, cycle))
    {
      eastl::string cycleString;
      for (resource_slot::detail::NodeId node : cycle)
      {
        cycleString.append_sprintf("%s -> ", storage.nodeMap.name(node));
      }
      cycleString.resize(cycleString.size() - 4); // Remove last arrow
      fatal("Found a cycle in resource_slot graph: %s", cycleString.c_str());
      return false;
    }
  }

  if (debug_topological_order)
  {
    debug("topological order");
    for (resource_slot::detail::NodeId nodeId : top_sorted_nodes)
    {
      debug("- node \"%s\"", storage.nodeMap.name(nodeId));
    }
    debug("end of topological order");
  }

  return true;
}


static inline void fill_slots_state(resource_slot::detail::Storage &storage, resource_slot::detail::NodeList &top_sorted_nodes)
{
  resource_slot::detail::AutoGrowVector<resource_slot::detail::SlotId, resource_slot::detail::ResourceId,
    resource_slot::detail::EXPECTED_MAX_SLOT_COUNT, false, framemem_allocator>
    currentSlotsState{storage.slotMap.nameCount()};


  for (resource_slot::detail::NodeId nodeId : top_sorted_nodes)
  {
    resource_slot::detail::NodeDeclaration &node = storage.registeredNodes[nodeId];
    G_ASSERT(node.id != resource_slot::detail::NodeId::Invalid);

    node.resourcesBeforeNode.clear();

    // Update state after node
    for (const resource_slot::detail::AccessDecl &declaration : node.action_list)
    {
      eastl::visit(
        [&currentSlotsState, &node](const auto &decl) {
          typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
          if constexpr (eastl::is_same_v<ValueT, resource_slot::detail::CreateDecl>)
          {
            currentSlotsState[decl.slot] = decl.resource;
          }
          else if constexpr (eastl::is_same_v<ValueT, resource_slot::detail::UpdateDecl>)
          {
            node.resourcesBeforeNode[decl.slot] = currentSlotsState[decl.slot];
            currentSlotsState[decl.slot] = decl.resource;
          }
          else if constexpr (eastl::is_same_v<ValueT, resource_slot::detail::ReadDecl>)
          {
            node.resourcesBeforeNode[decl.slot] = currentSlotsState[decl.slot];
          }
        },
        declaration);
    }
  }
}


void resource_slot::resolve_access(unsigned storage_id)
{
  detail::Storage &storage = detail::storage_list[storage_id];

  if (register_nodes || register_nodes_every_frame)
  {
    register_nodes.set(false);
    storage.isNodeRegisterRequired = true;
  }

  if (!storage.isNodeRegisterRequired)
    return;

  TIME_PROFILE(resource_slot__perform_access);
  storage.isNodeRegisterRequired = false;

  {
    FRAMEMEM_REGION;

    typedef dag::RelocatableFixedVector<detail::SlotUsage, detail::EXPECTED_MAX_SLOT_USAGE, true, framemem_allocator> UsageList;
    detail::AutoGrowVector<detail::SlotId, UsageList, detail::EXPECTED_MAX_SLOT_COUNT, false, framemem_allocator> slotUsage{
      storage.slotMap.nameCount()};

    if (debug_topological_order)
      debug("resource_slot::resolve_access(): start graph generation");

    for (const detail::NodeDeclaration &node : storage.registeredNodes)
    {
      if (node.id == detail::NodeId::Invalid)
        continue;

      add_node_to_usage_list(slotUsage, storage, node);
    }

    detail::Graph<framemem_allocator> dependencies{storage.registeredNodes.size()};
    if (!build_edges_from_usage_list(dependencies, slotUsage, storage))
      return;

    // Topological sort
    detail::NodeList topSortedNodes;
    if (!make_topological_sort(dependencies, storage, topSortedNodes))
      return;

    fill_slots_state(storage, topSortedNodes);
  }

  // Call declarations outside FRAMEMEM_REGION
  for (resource_slot::detail::NodeDeclaration &node : storage.registeredNodes)
  {
    if (node.id == resource_slot::detail::NodeId::Invalid)
      continue;

    node.nodeHandle = dabfg::NodeHandle{};
    node.nodeHandle = node.declaration_callback(resource_slot::State{storage_id, static_cast<int>(node.id)});
  }
}