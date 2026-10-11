// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "intermediateRepresentation.h"

#include <memory/dag_framemem.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/bitvector.h>
#include <EASTL/algorithm.h>

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

  for (const auto &[idx, used] : resourceStillUsed.enumerate())
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

  for (const auto &[i, res] : resources.enumerate())
    for (auto resNameId : res.frontendResources)
      result.mapRes(resNameId, res.multiplexingIndex) = i;

  for (const auto &[i, node] : nodes.enumerate())
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

  for (const auto &[nodeIdx, state] : nodeStates.enumerate())
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

  for (const auto &res : resources.values())
    G_ASSERT(!res.frontendResources.empty());
#endif
}

namespace
{

constexpr uint32_t NO_VALUE = UINT32_MAX;

// Classic O(n log n) LIS
eastl::bitvector<framemem_allocator> longest_increasing_subsequence(const dag::Vector<uint32_t, framemem_allocator> &values)
{
  const uint32_t n = static_cast<uint32_t>(values.size());
  eastl::bitvector<framemem_allocator> selected(n, false);

  dag::Vector<uint32_t, framemem_allocator> tails;
  tails.reserve(n);
  dag::Vector<uint32_t, framemem_allocator> previousInChain(n, NO_VALUE);

  const auto byValue = [&values](uint32_t idx, uint32_t v) { return values[idx] < v; };

  for (uint32_t i = 0; i < n; ++i)
  {
    const uint32_t value = values[i];
    if (value == NO_VALUE)
      continue;

    const size_t k = eastl::lower_bound(tails.begin(), tails.end(), value, byValue) - tails.begin();
    if (k > 0)
      previousInChain[i] = tails[k - 1];
    if (k == tails.size())
      tails.push_back(i);
    else
      tails[k] = i;
  }

  for (uint32_t i = tails.empty() ? NO_VALUE : tails.back(); i != NO_VALUE; i = previousInChain[i])
    selected[i] = true;

  return selected;
}

IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> remap_node_order(
  const IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> &source_to_desired,
  const IdIndexedMapping<NodeIndex, NodeIndex> &source_to_prev)
{
  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> result(source_to_desired.size(), NODE_NOT_MAPPED);

  uint32_t maxDesired = 0;
  for (auto pos : source_to_desired)
    if (pos != NODE_NOT_MAPPED)
      maxDesired = eastl::max(maxDesired, eastl::to_underlying(pos) + 1u);

  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desiredToSource(maxDesired, NODE_NOT_MAPPED);
  dag::Vector<uint32_t, framemem_allocator> desiredToPrev(maxDesired, NO_VALUE);
  for (const auto &[source, pos] : source_to_desired.enumerate())
    if (pos != NODE_NOT_MAPPED)
    {
      G_ASSERTF(desiredToSource[pos] == NODE_NOT_MAPPED, "daFG: IR nodes %d and %d both want desired slot %d!",
        eastl::to_underlying(desiredToSource[pos]), eastl::to_underlying(source), eastl::to_underlying(pos));
      desiredToSource[pos] = source;
      if (source_to_prev.isMapped(source) && source_to_prev[source] != NODE_NOT_MAPPED)
        desiredToPrev[eastl::to_underlying(pos)] = eastl::to_underlying(source_to_prev[source]);
    }

  // LIS of desiredToPrev gives us the biggest set of currently existing nodes that can be kept in previous positions
  auto kept = longest_increasing_subsequence(desiredToPrev);

  const auto nextKeptAfter = [&kept, maxDesired](uint32_t pos) {
    while (pos < maxDesired && !kept[pos])
      ++pos;
    return pos;
  };

  uint32_t nextFree = 0;
  uint32_t nextKeptPos = nextKeptAfter(0);
  for (uint32_t desiredPos = 0; desiredPos < maxDesired; ++desiredPos)
  {
    const auto source = desiredToSource[static_cast<NodeIndex>(desiredPos)];
    if (source == NODE_NOT_MAPPED)
      continue;

    uint32_t chosenPos = nextFree;
    if (kept[desiredPos])
    {
      chosenPos = desiredToPrev[desiredPos];
      G_FAST_ASSERT(chosenPos >= nextFree);
      nextKeptPos = nextKeptAfter(desiredPos + 1);
    }
    else if (nextKeptPos < maxDesired && desiredToPrev[nextKeptPos] == chosenPos)
    {
      kept[nextKeptPos] = false;
      nextKeptPos = nextKeptAfter(nextKeptPos + 1);
    }

    result[source] = static_cast<NodeIndex>(chosenPos);
    nextFree = chosenPos + 1;
  }

#if DAGOR_DBGLEVEL > 0
  for (const auto &[source, pos] : source_to_desired.enumerate())
    G_ASSERT(pos == NODE_NOT_MAPPED || result[source] != NODE_NOT_MAPPED);
#endif

  return result;
}

using NodeMappingFmem = IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator>;
using NodeFlagsFmem = IdIndexedFlags<NodeIndex, framemem_allocator>;

void erase_moved_nodes(Graph &graph, const IdIndexedMapping<NodeIndex, NodeIndex> &source_to_prev_dst,
  const NodeFlagsFmem &kept_in_place)
{
  for (const auto &[source, prevDst] : source_to_prev_dst.enumerate())
  {
    if (prevDst == NODE_NOT_MAPPED || kept_in_place.test(source, false))
      continue;
    const bool nodeMapped = graph.nodes.isMapped(prevDst);
    G_FAST_ASSERT(graph.nodeStates.empty() || graph.nodeStates.isMapped(prevDst) == nodeMapped);
    G_FAST_ASSERT(graph.nodeNames.empty() || graph.nodeNames.isMapped(prevDst) == nodeMapped);
    if (nodeMapped)
      graph.nodes.erase(prevDst);
    if (graph.nodeStates.isMapped(prevDst))
      graph.nodeStates.erase(prevDst);
    if (graph.nodeNames.isMapped(prevDst))
      graph.nodeNames.erase(prevDst);
  }
}

void emplace_moved_nodes(Graph &graph, const Graph &source_graph, const NodeMappingFmem &source_to_dst,
  const NodeFlagsFmem &kept_in_place, NodeFlagsFmem &out_nodes_changed)
{
  size_t dstCount = graph.nodes.totalKeys();
  for (auto dst : source_to_dst)
    if (dst != NODE_NOT_MAPPED)
      dstCount = eastl::max(dstCount, static_cast<size_t>(eastl::to_underlying(dst)) + 1);
  out_nodes_changed.resize(dstCount, false);

  for (const auto &[source, dst] : source_to_dst.enumerate())
  {
    if (dst == NODE_NOT_MAPPED || kept_in_place.test(source, false))
      continue;

    G_ASSERTF(!graph.nodes.isMapped(dst), "daFG: IR node slot %d was claimed twice while remapping!", eastl::to_underlying(dst));
    graph.nodes.emplaceAt(dst, source_graph.nodes[source]);
    if (source_graph.nodeStates.isMapped(source))
      graph.nodeStates.emplaceAt(dst, source_graph.nodeStates[source]);
    if (source_graph.nodeNames.isMapped(source))
      graph.nodeNames.emplaceAt(dst, source_graph.nodeNames[source]);

    out_nodes_changed.set(dst, true);
  }
}

void refresh_predecessors(Graph &graph, const Graph &source_graph, const NodeMappingFmem &source_to_dst,
  const NodeFlagsFmem &kept_in_place)
{
  const auto rebuild = [&](NodeIndex source, NodeIndex dst) {
    auto &preds = graph.nodes[dst].predecessors;
    preds.clear();
    for (const auto sourcePred : source_graph.nodes[source].predecessors)
      if (source_to_dst[sourcePred] != NODE_NOT_MAPPED)
        preds.insert(source_to_dst[sourcePred]);
  };

  const auto isStale = [&](NodeIndex source, NodeIndex dst) {
    const auto &stored = graph.nodes[dst].predecessors;
    uint32_t mappedPreds = 0;
    for (const auto sourcePred : source_graph.nodes[source].predecessors)
    {
      const auto dstPred = source_to_dst[sourcePred];
      if (dstPred == NODE_NOT_MAPPED)
        continue;
      if (!stored.contains(dstPred))
        return true;
      ++mappedPreds;
    }
    return stored.size() != mappedPreds;
  };

  for (const auto &[source, dst] : source_to_dst.enumerate())
  {
    if (dst == NODE_NOT_MAPPED)
      continue;

    if (!kept_in_place.test(source, false) || isStale(source, dst))
    {
      rebuild(source, dst);
      continue;
    }

#if DAGOR_DBGLEVEL > 0
    const auto actual = graph.nodes[dst].predecessors;
    rebuild(source, dst);
    G_ASSERTF(actual == graph.nodes[dst].predecessors,
      "daFG: selective predecessor refresh went stale for IR node slot %d (source %d)!", eastl::to_underlying(dst),
      eastl::to_underlying(source));
#endif
  }
}

} // namespace

IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> apply_node_remap(Graph &graph, const Graph &source_graph,
  const IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> &source_to_desired,
  const IdIndexedMapping<NodeIndex, NodeIndex> &source_to_prev_dst,
  const IdIndexedFlags<NodeIndex, framemem_allocator> &source_nodes_changed,
  IdIndexedFlags<NodeIndex, framemem_allocator> &out_nodes_changed)
{
  // The assignment reuses slots that are about to be vacated, so it runs before any erasure.
  auto sourceToDst = remap_node_order(source_to_desired, source_to_prev_dst);

  NodeFlagsFmem keptInPlace(source_to_prev_dst.totalKeys(), false);
  for (const auto &[source, prevDst] : source_to_prev_dst.enumerate())
    if (prevDst != NODE_NOT_MAPPED && !source_nodes_changed.test(source, false) && sourceToDst.isMapped(source) &&
        sourceToDst[source] == prevDst)
      keptInPlace.set(source, true);

  // Relocated nodes routinely land on slots that other relocated nodes vacate,
  // so all erasures come before all emplacements.
  erase_moved_nodes(graph, source_to_prev_dst, keptInPlace);
  emplace_moved_nodes(graph, source_graph, sourceToDst, keptInPlace, out_nodes_changed);
  refresh_predecessors(graph, source_graph, sourceToDst, keptInPlace);

  return sourceToDst;
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
