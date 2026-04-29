// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "intermediateRepresentation.h"

#include <memory/dag_framemem.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/bitvector.h>

#include <id/idIndexedFlags.h>
#include <id/idRange.h>
#include <id/idExtentsFinder.h>


namespace dafg
{

namespace intermediate
{

void Graph::pruneResources()
{
  IdIndexedFlags<ResourceIndex, framemem_allocator> resourceStillUsed(resources.totalKeys(), false);

  for (const auto &node : nodes.values())
    for (const auto &req : node.resourceRequests)
      resourceStillUsed.set(req.resource, true);

  for (auto [idx, used] : resourceStillUsed.enumerate())
    if (!used)
    {
      if (resources.isMapped(idx))
        resources.erase(idx);
      if (resourceNames.isMapped(idx))
        resourceNames.erase(idx);
    }
}

Mapping Graph::calculateMapping() const
{
  // Jump through hoops to ensure a single allocation

  IdExtentsFinder<MultiplexingIndex> multiIdxExtents;

  IdExtentsFinder<NodeNameId> nodeNameIdExtents;
  for (const auto &node : nodes.values())
  {
    multiIdxExtents.update(node.multiplexingIndex);
    if (node.frontendNode)
      nodeNameIdExtents.update(*node.frontendNode);
  }

  IdExtentsFinder<ResNameId> resNameIdExtents;
  for (const auto &res : resources.values())
  {
    multiIdxExtents.update(res.multiplexingIndex);
    for (auto resNameId : res.frontendResources)
      resNameIdExtents.update(resNameId);
  }

  Mapping result{nodeNameIdExtents.get(), resNameIdExtents.get(), multiIdxExtents.get()};

  for (auto [i, res] : resources.enumerate())
    for (auto resNameId : res.frontendResources)
      result.mapRes(resNameId, res.multiplexingIndex) = i;

  for (auto [i, node] : nodes.enumerate())
    if (node.frontendNode)
      result.mapNode(*node.frontendNode, node.multiplexingIndex) = i;

  // NOTE: This is a bit fragile. The frontend multiplexing code has to
  // guarantee that in case of undermultiplexed nodes/resources each
  // "empty" mapping slot should be mapped to the nearest previous mapped
  // slot.

  for (uint32_t i = 1; i < result.multiplexingExtent(); ++i)
  {
    const auto currMultiIdx = static_cast<MultiplexingIndex>(i);
    const auto prevMultiIdx = static_cast<MultiplexingIndex>(i - 1);

    for (auto resNameId : IdRange<ResNameId>(result.mappedResNameIdCount()))
      if (!result.wasResMapped(resNameId, currMultiIdx))
        result.mapRes(resNameId, currMultiIdx) = result.mapRes(resNameId, prevMultiIdx);

    for (auto nodeNameId : IdRange<NodeNameId>(result.mappedNodeNameIdCount()))
      if (!result.wasNodeMapped(nodeNameId, currMultiIdx))
        result.mapNode(nodeNameId, currMultiIdx) = result.mapNode(nodeNameId, prevMultiIdx);
  }

  return result;
}

void Graph::validate() const
{
  TIME_PROFILE(validate);

#if DAGOR_DBGLEVEL > 0
  G_ASSERTF_RETURN(nodes.usedKeysSameAs(nodeStates), , //
    "Inconsistent IR: nodes (mask %s) and node states (mask %s) should have the same set of keys!",
    nodes.makeUsedKeysBitmaskString().c_str(), nodeStates.makeUsedKeysBitmaskString().c_str());
  G_ASSERTF_RETURN(nodeNames.empty() || nodes.usedKeysSameAs(nodeNames), , //
    "Inconsistent IR: nodes (mask %s) and node names (mask %s) should have the same set of keys if names are present at all!",
    nodes.makeUsedKeysBitmaskString().c_str(), nodeNames.makeUsedKeysBitmaskString().c_str());
  G_ASSERTF_RETURN(resourceNames.empty() || resources.usedKeysSameAs(resourceNames), , //
    "Inconsistent IR: resources (mask %s) and resource names (mask %s) should be parallel if names are present at all!",
    resources.makeUsedKeysBitmaskString().c_str(), resourceNames.makeUsedKeysBitmaskString().c_str());

  for (auto &node : nodes.values())
  {
    for (const auto pred : node.predecessors)
      G_ASSERTF(nodes.isMapped(pred), "Inconsistent IR: missing node with index %d", pred);
    for (auto &req : node.resourceRequests)
      G_ASSERTF(resources.isMapped(req.resource), "Inconsistent IR: missing resource with index %d", req.resource);
  }

  for (auto [nodeIdx, state] : nodeStates.enumerate())
  {
    const auto validateRes = [this, nodeIdx = nodeIdx](ResourceIndex res_idx) {
      G_ASSERTF_RETURN(resources.isMapped(res_idx), , "Inconsistent IR: missing resource with index %d", res_idx);

      if (!nodes[nodeIdx].hasSideEffects)
        return;

      bool found = false;
      for (const auto &req : nodes[nodeIdx].resourceRequests)
        if (req.resource == res_idx)
          found = true;
      G_ASSERTF(found, "Inconsistent IR: resource %s used in node %s state, but was not requested by it!", resourceNames[res_idx],
        nodeNames[nodeIdx]);
    };

    for (const auto &[_, binding] : state.bindings)
      if (binding.resource.has_value())
        validateRes(*binding.resource);

    if (state.pass.has_value())
    {
      if (state.pass->depthAttachment.has_value())
        validateRes(state.pass->depthAttachment->resource);
      if (state.pass->vrsRateAttachment.has_value())
        validateRes(state.pass->vrsRateAttachment->resource);
      for (const auto &color : state.pass->colorAttachments)
        if (color.has_value())
          validateRes(color->resource);
    }
  }

  for (auto [resIdx, res] : resources.enumerate())
    G_ASSERT(!res.frontendResources.empty());
#endif
}

IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> try_remap_node_order(const Graph &graph,
  const IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> &desired, const IdIndexedMapping<NodeIndex, NodeIndex> &prev)
{
  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> result(desired.size(), NODE_NOT_MAPPED);

  // Build inverse of desired: inverseDesired[sortedIdx] = unsortedIdx
  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> inverseDesired;
  for (auto [unsortedIdx, sortedIdx] : desired.enumerate())
    if (sortedIdx != NODE_NOT_MAPPED)
    {
      inverseDesired.expandMapping(sortedIdx, NODE_NOT_MAPPED);
      inverseDesired.set(sortedIdx, unsortedIdx);
    }

  // Pass 1: reuse old sorted positions, maintaining monotonic order
  NodeIndex nextUnusedIdx{0};
  for (auto [desiredSortedIdx, unsortedIdx] : inverseDesired.enumerate())
  {
    if (unsortedIdx == NODE_NOT_MAPPED)
      continue;
    if (!prev.isMapped(unsortedIdx) || prev[unsortedIdx] == NODE_NOT_MAPPED)
      continue;
    if (!graph.nodes.isMapped(prev[unsortedIdx]))
      continue;
    if (eastl::to_underlying(nextUnusedIdx) > eastl::to_underlying(prev[unsortedIdx]))
      continue;

    result[unsortedIdx] = prev[unsortedIdx];
    nextUnusedIdx = static_cast<NodeIndex>(eastl::to_underlying(prev[unsortedIdx]) + 1);
  }

  // Pass 2: fill remaining nodes into free slots
  nextUnusedIdx = NodeIndex{0};
  for (auto [desiredSortedIdx, unsortedIdx] : inverseDesired.enumerate())
  {
    G_FAST_ASSERT(unsortedIdx != NODE_NOT_MAPPED);
    if (result[unsortedIdx] != NODE_NOT_MAPPED)
    {
      nextUnusedIdx = static_cast<NodeIndex>(eastl::to_underlying(result[unsortedIdx]) + 1);
      continue;
    }
    if (!graph.nodes.isMapped(nextUnusedIdx))
    {
      result[unsortedIdx] = nextUnusedIdx;
      nextUnusedIdx = static_cast<NodeIndex>(eastl::to_underlying(nextUnusedIdx) + 1);
    }
  }

  return result;
}

IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> apply_node_remap(Graph &graph, const Graph &source_graph,
  const IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> &desired_mapping,
  const IdIndexedMapping<NodeIndex, NodeIndex> &prev_mapping,
  const IdIndexedFlags<NodeIndex, framemem_allocator> &source_nodes_changed,
  IdIndexedFlags<NodeIndex, framemem_allocator> &out_nodes_changed)
{
  // Step 1: Erase changed/deleted/culled nodes
  for (auto [srcIdx, oldDstIdx] : prev_mapping.enumerate())
  {
    if (oldDstIdx == NODE_NOT_MAPPED)
      continue;
    if (source_nodes_changed.test(srcIdx, false) || !desired_mapping.isMapped(srcIdx) || desired_mapping[srcIdx] == NODE_NOT_MAPPED)
    {
      graph.nodes.erase(oldDstIdx);
      graph.nodeStates.erase(oldDstIdx);
      graph.nodeNames.erase(oldDstIdx);
    }
  }

  // Step 2: Remap to preserve old sorted positions where possible
  auto newMapping = try_remap_node_order(graph, desired_mapping, prev_mapping);

  // Step 3: Erase moved-but-unchanged nodes (old position differs from new)
  for (auto [srcIdx, oldDstIdx] : prev_mapping.enumerate())
  {
    if (oldDstIdx == NODE_NOT_MAPPED)
      continue;
    if (!source_nodes_changed.test(srcIdx, false) && newMapping.isMapped(srcIdx) && newMapping[srcIdx] != NODE_NOT_MAPPED &&
        newMapping[srcIdx] != oldDstIdx)
    {
      graph.nodes.erase(oldDstIdx);
      graph.nodeStates.erase(oldDstIdx);
      graph.nodeNames.erase(oldDstIdx);
    }
  }

  // Step 4: Fallback -- if remap failed for any node, clear graph and use dense mapping
  bool fallback = false;
  for (auto [srcIdx, dstIdx] : newMapping.enumerate())
    if (dstIdx == NODE_NOT_MAPPED && desired_mapping[srcIdx] != NODE_NOT_MAPPED)
    {
      newMapping.assign(desired_mapping.begin(), desired_mapping.end());
      graph.nodes.clear();
      graph.nodeStates.clear();
      graph.nodeNames.clear();
      fallback = true;
      break;
    }

  // Step 5: Initialize output nodesChanged
  out_nodes_changed.resize(graph.nodes.totalKeys(), false);

  // Step 6: Emplace new/moved nodes from source graph
  for (auto [srcIdx, dstIdx] : newMapping.enumerate())
  {
    if (dstIdx == NODE_NOT_MAPPED)
      continue;
    if (graph.nodes.isMapped(dstIdx))
      continue;

    graph.nodes.emplaceAt(dstIdx, source_graph.nodes[srcIdx]);
    if (source_graph.nodeStates.isMapped(srcIdx))
      graph.nodeStates.emplaceAt(dstIdx, source_graph.nodeStates[srcIdx]);
    if (source_graph.nodeNames.isMapped(srcIdx))
      graph.nodeNames.emplaceAt(dstIdx, source_graph.nodeNames[srcIdx]);

    out_nodes_changed.set(dstIdx, true);
  }

  // Step 7: Propagate source changes to output
  if (fallback)
  {
    out_nodes_changed.assign(out_nodes_changed.size(), true);
  }
  else
  {
    for (auto [srcIdx, dstIdx] : newMapping.enumerate())
      if (dstIdx != NODE_NOT_MAPPED && source_nodes_changed.test(srcIdx, false))
        out_nodes_changed.set(dstIdx, true);
    // Previously-pruned-now-restored nodes
    for (auto [srcIdx, dstIdx] : newMapping.enumerate())
      if (dstIdx != NODE_NOT_MAPPED && prev_mapping.isMapped(srcIdx) && prev_mapping[srcIdx] == NODE_NOT_MAPPED)
        out_nodes_changed.set(dstIdx, true);
  }

  // Step 8: Refresh predecessors (remap from source space to destination space)
  for (auto [srcIdx, dstIdx] : newMapping.enumerate())
  {
    if (dstIdx == NODE_NOT_MAPPED)
      continue;
    graph.nodes[dstIdx].predecessors.clear();
    for (const auto pred : source_graph.nodes[srcIdx].predecessors)
      if (newMapping[pred] != NODE_NOT_MAPPED)
        graph.nodes[dstIdx].predecessors.insert(newMapping[pred]);
  }

  return newMapping;
}

Mapping::Mapping(uint32_t node_count, uint32_t res_count, uint32_t multiplexing_extent) :
  mappedNodeNameIdCount_{node_count},
  mappedResNameIdCount_{res_count},
  multiplexingExtent_{multiplexing_extent},
  nodeNameIdMapping_(node_count * multiplexing_extent, NODE_NOT_MAPPED),
  resNameIdMapping_(res_count * multiplexing_extent, RESOURCE_NOT_MAPPED)
{}

bool Mapping::wasResMapped(ResNameId id, MultiplexingIndex multi_idx) const
{
  return eastl::to_underlying(multi_idx) < multiplexingExtent_ && eastl::to_underlying(id) < mappedResNameIdCount_ &&
         mapRes(id, multi_idx) != RESOURCE_NOT_MAPPED;
}

bool Mapping::wasNodeMapped(NodeNameId id, MultiplexingIndex multi_idx) const
{
  return eastl::to_underlying(multi_idx) < multiplexingExtent_ && eastl::to_underlying(id) < mappedNodeNameIdCount_ &&
         mapNode(id, multi_idx) != NODE_NOT_MAPPED;
}

uint32_t Mapping::multiplexingExtent() const { return multiplexingExtent_; }

ResourceIndex &Mapping::mapRes(ResNameId id, MultiplexingIndex multi_idx)
{
  return resNameIdMapping_[eastl::to_underlying(multi_idx) + multiplexingExtent_ * eastl::to_underlying(id)];
}

// Const cast is ok in below cases as we never mutate `this`, only copy a
// value from there. This trick avoids copy-pasta.
ResourceIndex Mapping::mapRes(ResNameId id, MultiplexingIndex multi_idx) const
{
  return const_cast<Mapping *>(this)->mapRes(id, multi_idx);
}

NodeIndex &Mapping::mapNode(NodeNameId id, MultiplexingIndex multi_idx)
{
  return nodeNameIdMapping_[eastl::to_underlying(multi_idx) + multiplexingExtent_ * eastl::to_underlying(id)];
}

NodeIndex Mapping::mapNode(NodeNameId id, MultiplexingIndex multi_idx) const
{
  return const_cast<Mapping *>(this)->mapNode(id, multi_idx);
}

} // namespace intermediate

} // namespace dafg
