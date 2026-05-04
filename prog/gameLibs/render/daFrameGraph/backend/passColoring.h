// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <id/idIndexedMapping.h>
#include <id/idIndexedFlags.h>
#include <backend/intermediateRepresentation.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <dag/dag_vectorMap.h>
#include <memory/dag_framemem.h>


namespace dafg
{

enum class PassColor : uint32_t
{
};

using PassColoring = IdIndexedMapping<intermediate::NodeIndex, PassColor>;

// Performs a best-effort coloring of nodes such that nodes that should
// be within a single render pass will be colored the same
class PassColorer
{
public:
  using NodesChanged = IdIndexedFlags<intermediate::NodeIndex, framemem_allocator>;

  PassColoring performColoring(const intermediate::Graph &graph, const NodesChanged &node_changes);

private:
  enum class SubpassColor : uint32_t
  {
    Unknown = static_cast<uint32_t>(-1),
  };

  using SubpassColoring = IdIndexedMapping<intermediate::NodeIndex, SubpassColor>;
  void performSubpassColoring(const intermediate::Graph &graph, const NodesChanged &node_changes);

  void updateGpuRequestsMap(const intermediate::Graph &graph, const NodesChanged &node_changes);
  void recalcFbLabels(const intermediate::Graph &graph);

  bool requiresBarrierBetween(intermediate::NodeIndex fst, intermediate::NodeIndex snd) const;

  // intermediate::ResourceIndex + history flag packed into one uint16_t
  enum class GpuRequestUid : eastl::underlying_type_t<intermediate::ResourceIndex>
  {
  };
  static GpuRequestUid makeGpuRequestUid(intermediate::ResourceIndex idx, bool from_last_frame);

  struct GpuRequestUidCmp
  {
    bool operator()(GpuRequestUid fst, GpuRequestUid snd) const { return eastl::to_underlying(fst) < eastl::to_underlying(snd); }
  };
  using PerNodeGpuRequestMap = dag::VectorMap<GpuRequestUid, intermediate::ResourceUsage, GpuRequestUidCmp>;
  using GpuRequestMap = IdIndexedMapping<intermediate::NodeIndex, PerNodeGpuRequestMap>;

  enum class FbLabel : uint16_t
  {
  };
  using FbLabels = IdIndexedMapping<intermediate::NodeIndex, FbLabel>;

  GpuRequestMap gpuRequestsMap;
  FbLabels fbLabels;
  SubpassColoring subpassColoring;
  eastl::underlying_type_t<SubpassColor> nextColor = 0;
  size_t changesSinceLastFullColoring = 0;
};

} // namespace dafg
