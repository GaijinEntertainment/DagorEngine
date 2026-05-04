// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMathDecl.h>
#include "passColoring.h"

#include "generic/dag_reverseView.h"
#include <EASTL/queue.h>
#include <memory/dag_framemem.h>
#include <util/dag_stlqsort.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_stlqsort.h>

#include <id/idRange.h>
#include <id/idIndexedFlags.h>
#include <backend/simdBitMatrix.h>
#include <backend/simdBitVector.h>


// Yet another copy of boost::hash_combine
template <class T, class... Ts>
static void hashPack(size_t &hash, T first, const Ts &...other)
{
  hash ^= eastl::hash<T>{}(first) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  (hashPack(hash, other), ...);
}

namespace eastl
{
template <>
class hash<dafg::intermediate::RenderPass>
{
public:
  size_t operator()(const dafg::intermediate::RenderPass &pass) const
  {
    size_t result = 0;
    for (const auto &attachment : pass.colorAttachments)
      if (attachment)
        hashPack(result, attachment->resource, attachment->layer, attachment->mipLevel);
      else
        hashPack(result, dafg::intermediate::RESOURCE_NOT_MAPPED);
    if (pass.depthAttachment)
      hashPack(result, pass.depthAttachment->resource, pass.depthAttachment->layer, pass.depthAttachment->mipLevel);
    else
      hashPack(result, dafg::intermediate::RESOURCE_NOT_MAPPED);
    return result;
  }
};
} // namespace eastl

namespace dafg
{

PassColorer::GpuRequestUid PassColorer::makeGpuRequestUid(intermediate::ResourceIndex idx, bool from_last_frame)
{
  using IdT = eastl::underlying_type_t<intermediate::ResourceIndex>;
  static constexpr auto HIGHEST_BIT = IdT{1} << (sizeof(IdT) * CHAR_BIT - 1);
  // This is actually enforced on the frontend, but lets check it again to be safe
  G_ASSERT((eastl::to_underlying(idx) & HIGHEST_BIT) == 0);
  return static_cast<GpuRequestUid>(eastl::to_underlying(idx) | (from_last_frame ? HIGHEST_BIT : 0));
}

void PassColorer::updateGpuRequestsMap(const intermediate::Graph &graph, const NodesChanged &node_changes)
{
  if (gpuRequestsMap.size() < graph.nodes.totalKeys())
    gpuRequestsMap.resize(graph.nodes.totalKeys());

  for (auto idx : graph.nodes.keys())
    gpuRequestsMap[idx].reserve(graph.nodes[idx].resourceRequests.size());

  for (auto nodeIdx : node_changes.trueKeys())
  {
    auto &node = graph.nodes[nodeIdx];
    auto &reqs = gpuRequestsMap[nodeIdx];
    reqs.clear();
    reqs.reserve(node.resourceRequests.size()); // TODO: don't overallocate
    for (const auto &req : node.resourceRequests)
      if (graph.resources[req.resource].getResType() != ResourceType::Blob)
        reqs[makeGpuRequestUid(req.resource, req.fromLastFrame)] = req.usage;
  }
}

bool PassColorer::requiresBarrierBetween(intermediate::NodeIndex fst, intermediate::NodeIndex snd) const
{
  const auto &fstRequests = gpuRequestsMap[fst];
  const auto &sndRequests = gpuRequestsMap[snd];

  // Fine-tuned to be faster than eastl::set_intersection
  auto fstIt = fstRequests.begin();
  auto sndIt = sndRequests.begin();
  const auto fstEnd = fstRequests.end();
  const auto sndEnd = sndRequests.end();
  while ((fstIt != fstEnd) && (sndIt != sndEnd))
  {
    if (fstIt->first < sndIt->first)
      ++fstIt;
    else if (fstIt->first > sndIt->first)
      ++sndIt;
    else
    {
      // Same resource but different usage => conflict, except for
      // different stages, which is fine. If two nodes use the resource
      // in exactly the same manner but on different stages, we don't
      // really need a barrier between them, we simply need to make
      // the previous barrier "stronger" by adding the new stage to it,
      // which is preformed later down the graph compilation pipeline.
      if (fstIt->second.access != sndIt->second.access || fstIt->second.type != sndIt->second.type)
        return true;

      ++fstIt;
      ++sndIt;
    }
  }

  return false;
}

void PassColorer::recalcFbLabels(const intermediate::Graph &graph)
{
  fbLabels.assign(graph.nodeStates.totalKeys(), FbLabel{0});

  FRAMEMEM_VALIDATE;
  using MaybePass = eastl::optional<intermediate::RenderPass>;
  ska::flat_hash_map<MaybePass, FbLabel, eastl::hash<MaybePass>, eastl::equal_to<MaybePass>, framemem_allocator> passes;
  passes.reserve(graph.nodeStates.totalKeys());
  for (auto [idx, state] : graph.nodeStates.enumerate())
    if (auto it = passes.find(state.pass); it != passes.end())
      fbLabels[idx] = it->second;
    else
    {
      const auto next = passes.size();
      fbLabels[idx] = passes[state.pass] = static_cast<FbLabel>(next);
    }
}

void PassColorer::performSubpassColoring(const intermediate::Graph &graph, const NodesChanged &node_changes)
{
  if (subpassColoring.size() < graph.nodes.totalKeys())
    subpassColoring.resize(graph.nodes.totalKeys(), SubpassColor::Unknown);
  for (auto idx : node_changes.trueKeys())
  {
    subpassColoring[idx] = SubpassColor::Unknown;
    ++changesSinceLastFullColoring;
  }

  FRAMEMEM_VALIDATE;

  // Needed for framemem tricks
  IdIndexedMapping<intermediate::NodeIndex, uint16_t, framemem_allocator> inDegree(graph.nodes.totalKeys());
  for (auto [nodeId, node] : graph.nodes.enumerate())
    for (auto pred : node.predecessors)
      ++inDegree[pred];

  using FmemNodeSet = dag::VectorSet<intermediate::NodeIndex, eastl::less<intermediate::NodeIndex>, framemem_allocator>;
  IdIndexedMapping<intermediate::NodeIndex, FmemNodeSet, framemem_allocator> successors(graph.nodes.totalKeys());
  for (auto nodeId : dag::ReverseView(graph.nodes.keys()))
    successors[nodeId].reserve(inDegree[nodeId]);
  for (auto [nodeId, node] : graph.nodes.enumerate())
    for (auto pred : node.predecessors)
      successors[pred].insert(nodeId);

  if (changesSinceLastFullColoring == 0)
  {
    // TODO: we can probably not do a full recolor in more cases here, but that's quite complicated
    // due to the global "condensability" requirement for colors. Checking that it's not violated requires
    // checking for barriers with every single node in a pass, which can be overly expensive.
    // For now we simply skip recoloring if nodes only disappeared.
    return;
  }

  changesSinceLastFullColoring = 0;
  subpassColoring.assign(graph.nodes.totalKeys(), SubpassColor::Unknown);

  FRAMEMEM_VALIDATE;

  IdIndexedMapping<intermediate::NodeIndex, uint16_t, framemem_allocator> outDegree(graph.nodes.totalKeys());
  for (auto [nodeId, node] : graph.nodes.enumerate())
    outDegree[nodeId] = node.predecessors.size();

  // The frontier contains all nodes with out-degree 0
  dag::Vector<intermediate::NodeIndex, framemem_allocator> frontier;
  frontier.reserve(graph.nodes.totalKeys());
  frontier.push_back(static_cast<intermediate::NodeIndex>(0));

  const auto fbLabelCount = 1 + eastl::to_underlying(*eastl::max_element(fbLabels.begin(), fbLabels.end(),
                                  [](auto fst, auto snd) { return eastl::to_underlying(fst) < eastl::to_underlying(snd); }));
  IdIndexedMapping<FbLabel, SubpassColor, framemem_allocator> lastColorInFb(fbLabelCount, SubpassColor::Unknown);

  // Imagine that we condense the graph into a DAG where each node is
  // a color and edges between colors are drawn whenever there was at
  // least a single edge between nodes of respective colors in the
  // initial graph. This structure encodes that graph.
  // Size is reserved for worst case.
  // Note that this intentionally does not account for paths of length 0,
  // this is convenienet for the algorithm below.
  SimdBitMatrix<SubpassColor, framemem_allocator> colorReachability(graph.nodes.totalKeys());

  nextColor = 0;

  // Traversal occurs from front to back by chipping away nodes with no successors
  while (!frontier.empty())
  {
    // TODO: there should be a heuristic here for chosing the best next node to color
    const auto curr = frontier.back();
    frontier.pop_back();

    SimdBitVector<SubpassColor, framemem_allocator> reachableFromCurrent(graph.nodes.totalKeys(), false);

    // Grab cached condensed graph reachability info from previous
    // iterations to avoid having to run a dfs every iteration
    {
      dag::VectorSet<SubpassColor, eastl::less<SubpassColor>, framemem_allocator> predecessorColors;
      predecessorColors.reserve(graph.nodes[curr].predecessors.size());
      for (auto pred : graph.nodes[curr].predecessors)
      {
        G_FAST_ASSERT(subpassColoring[pred] != SubpassColor::Unknown);
        predecessorColors.insert(subpassColoring[pred]);
      }

      // Note that our graph is already topsorted, so if there is a path
      // v <- u, then v_idx < u_idx. Therefore, the reachability matrix
      // of the graph will be lower-triangular. Same applies to the
      // color reachability matrix by it's construction, so we only have
      // to OR the rows of predecessor colors up to the diagonal, hence
      // the .first call here.
      for (auto color : predecessorColors)
        reachableFromCurrent |= colorReachability.row(color).first(eastl::to_underlying(color));
    }

    dag::VectorSet<SubpassColor, eastl::less<SubpassColor>, framemem_allocator> conflictingColors;
    conflictingColors.reserve(graph.nodes[curr].predecessors.size());
    for (auto pred : graph.nodes[curr].predecessors)
      if (requiresBarrierBetween(curr, pred))
        conflictingColors.insert(subpassColoring[pred]);

    const auto currFb = fbLabels[curr];
    // We can reuse color C iff:
    // - there is no path v <~ u <~ `curr` with v in C and u not in C,
    // - and there are no conflict edges between curr and any nodes in C.
    //
    // First condition is equivalent to the fact that we cannot have
    // cycles in the condensation graph, second contition is equivalent
    // to the fact that we cannot have conflicts between nodes of the
    // same color.
    // Note that the first condition implies that such paths must
    // contain at least 2 edges, which is why we don't account for
    // trivial paths with 0 edges inside the colorReachability matrix.

    // TODO: using lastColorInFb as the only candidate is suboptimal for
    // chains of compute nodes, there we want to use the first usable
    // color, not the last one.
    // On the other hand, if we have a lot of nodes inside the frontier,
    // chosing the first usable color could result in creating a new
    // color in folowing situations:
    //   condensed DAG    frontier
    // fb1:  C1   C2        u
    // fb2:  C3             v
    // And there are edges C1 <- v and C3 <- u.
    // If we first pick u from the frontier and pack it into the first
    // color, which is C1, we will not be able to pack v into C3 due to
    // a potential cycle in the condensed dag, so will have to create
    // a new color. On the other hand, packing u into C2 will not
    // cause such problems, which is exactly what we do for now.
    const auto candidateColor = lastColorInFb[currFb];
    if (candidateColor != SubpassColor::Unknown && !reachableFromCurrent.test(candidateColor) &&
        conflictingColors.find(candidateColor) == conflictingColors.end())
    {
      subpassColoring[curr] = lastColorInFb[currFb];
    }
    else
    {
      const auto newColor = static_cast<SubpassColor>(nextColor++);
      lastColorInFb[currFb] = newColor;
      subpassColoring[curr] = newColor;
    }

    // Now that we determined the color for this node, we need to update
    // reachability for that color with everything that was reachable
    // from this node.
    // Currently, reachableFromCurrent only contains paths of length 2
    // and above, so we have to manually add single-edge paths that
    // lead into a different color.
    for (auto pred : graph.nodes[curr].predecessors)
      if (subpassColoring[pred] != subpassColoring[curr])
        reachableFromCurrent.set(subpassColoring[pred], true);
    colorReachability.row(subpassColoring[curr]) |= reachableFromCurrent;

    // Recalculate the frontier
    for (auto pred : successors[curr])
      if (--outDegree[pred] == 0)
        frontier.push_back(pred);
  }
}

PassColoring PassColorer::performColoring(const intermediate::Graph &graph, const NodesChanged &node_changes)
{
  PassColoring result(graph.nodes.totalKeys());

  updateGpuRequestsMap(graph, node_changes);
  recalcFbLabels(graph);

  performSubpassColoring(graph, node_changes);

  // TODO: implement proper merging of subpasses into passes.
  // My current thoughts are that it should be enough to simply merge
  // stuff that uses subpass reads and ignore everything else.
  for (auto i : graph.nodes.keys())
  {
    G_ASSERT(subpassColoring[i] != SubpassColor::Unknown);
    result[i] = static_cast<PassColor>(eastl::to_underlying(subpassColoring[i]));
  }

  return result;
}

} // namespace dafg
