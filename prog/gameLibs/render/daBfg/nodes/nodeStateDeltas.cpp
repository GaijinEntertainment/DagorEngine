#include "nodeStateDeltas.h"

#include <EASTL/algorithm.h>
#include <EASTL/iterator.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <util/dag_globDef.h>
#include <memory/dag_framemem.h>


namespace dag
{

// TODO: move to RelocatableFixedVector
template <class T, size_t first_size, size_t second_size>
static bool operator==(const RelocatableFixedVector<T, first_size> &first, const RelocatableFixedVector<T, second_size> &second)
{
  if (first.size() != second.size())
    return false;
  return eastl::equal(first.begin(), first.end(), second.begin());
}

} // namespace dag

namespace dabfg
{

// TODO: when C++20 comes, purge all of this and replace with default operator==
// NOTE: ADL does not find these operators unless they are in the same
// namespace as the types they operate on.
namespace intermediate
{

static bool operator==(const VrsState &first, const VrsState &second)
{
  return first.rateX == second.rateX && first.rateY == second.rateY && first.rateTexture == second.rateTexture &&
         first.vertexCombiner == second.vertexCombiner && first.pixelCombiner == second.pixelCombiner;
}

static bool operator==(const SubresourceRef &first, const SubresourceRef &second)
{
  return first.resource == second.resource && first.mipLevel == second.mipLevel && first.layer == second.layer;
}

static bool operator==(const RenderPass &first, const RenderPass &second)
{
  return first.colorAttachments == second.colorAttachments && first.depthAttachment == second.depthAttachment &&
         first.depthReadOnly == second.depthReadOnly;
}

static bool operator<(const Binding &first, const Binding &second)
{
  if (first.type != second.type)
    return first.type < second.type;
  if (first.resource != second.resource)
    return first.resource < second.resource;
  if (first.history != second.history)
    return first.history < second.history;
  if (first.projectedTag != second.projectedTag)
    return first.projectedTag < second.projectedTag;
  const auto projector1 = reinterpret_cast<void *>(first.projector);
  const auto projector2 = reinterpret_cast<void *>(second.projector);
  if (projector1 != projector2)
    return projector1 < projector2;
  return false;
}

} // namespace intermediate

struct DeltaCalculator
{
  // Use this overload set to add special processing for fields
  // that change type.

  template <class U>
  U delta(const U &current, const U &previous)
  {
    G_UNUSED(previous);
    return current;
  }

  shaders::UniqueOverrideStateId delta(const eastl::optional<shaders::OverrideState> &current,
    const eastl::optional<shaders::OverrideState> &)
  {
    return current.has_value() ? shaders::overrides::create(*current) : shaders::OverrideStateId{};
  }

  VrsStateDelta delta(const eastl::optional<intermediate::VrsState> &current, const eastl::optional<intermediate::VrsState> &)
  {
    return current.has_value() ? VrsStateDelta{*current} : StopVrs{};
  }

  RenderPassDelta delta(const eastl::optional<intermediate::RenderPass> &current, const eastl::optional<intermediate::RenderPass> &)
  {
    return current.has_value() ? RenderPassDelta{*current} : FinishRenderPass{};
  }

  intermediate::BindingsMap delta(const intermediate::BindingsMap &old_binding, const intermediate::BindingsMap &new_binding)
  {
    FRAMEMEM_VALIDATE;

    // Pre-allocate all framemem data for graceful cleanup
    using BindingToSv = eastl::pair<int, intermediate::Binding>;
    using TmpBindingsSet = dag::VectorSet<BindingToSv, eastl::less<BindingToSv>, framemem_allocator>;
    TmpBindingsSet newSet(new_binding.begin(), new_binding.end());
    TmpBindingsSet oldSet(old_binding.begin(), old_binding.end());

    using TmpShaderVarBindingsMap = dag::VectorMap<int, intermediate::Binding, eastl::less<int>, framemem_allocator>;
    TmpShaderVarBindingsMap oldWithoutNew;
    oldWithoutNew.reserve(oldSet.size());
    TmpShaderVarBindingsMap result;
    result.reserve(newSet.size() + oldSet.size());

    eastl::set_difference_2(newSet.begin(), newSet.end(), oldSet.begin(), oldSet.end(), eastl::inserter(result, result.begin()),
      eastl::inserter(oldWithoutNew, oldWithoutNew.begin()));

    // Unbind stuff
    for (auto &[k, v] : oldWithoutNew)
      if (result.find(k) == result.end())
        result[k] = {v.type, eastl::nullopt, false, v.projectedTag};

    // Transfer to regular heap memory
    return {result.begin(), result.end()};
  }

  ShaderBlockLayersInfoDelta delta(const intermediate::ShaderBlockBindings &old_info,
    const intermediate::ShaderBlockBindings &new_info)
  {
    ShaderBlockLayersInfoDelta result;
    result.objectLayer = new_info.objectLayer != old_info.objectLayer ? eastl::optional<int>(new_info.objectLayer) : eastl::nullopt;
    result.sceneLayer = new_info.sceneLayer != old_info.sceneLayer ? eastl::optional<int>(new_info.sceneLayer) : eastl::nullopt;
    result.frameLayer = new_info.frameLayer != old_info.frameLayer ? eastl::optional<int>(new_info.frameLayer) : eastl::nullopt;
    return result;
  }
};

NodeStateDelta getStateDelta(const intermediate::RequiredNodeState &first, const intermediate::RequiredNodeState &second)
{
  NodeStateDelta result;
  DeltaCalculator calculator;

#define DELTA(field)                                                     \
  do                                                                     \
    if (!(first.field == second.field))                                  \
      result.field.emplace(calculator.delta(second.field, first.field)); \
  while (false)

  DELTA(wire);
  DELTA(vrs);
  DELTA(pass);
  DELTA(shaderOverrides);

#undef DELTA

  result.bindings = calculator.delta(first.bindings, second.bindings);
  result.shaderBlockLayers = calculator.delta(first.shaderBlockLayers, second.shaderBlockLayers);

  return result;
}

dag::Vector<NodeStateDelta> calculate_per_node_state_deltas(const intermediate::Graph &graph)
{
  dag::Vector<NodeStateDelta> result;
  result.reserve(graph.nodeStates.size() + 1);

  if (graph.nodeStates.empty())
  {
    result.emplace_back();
    return result;
  }

  result.emplace_back(getStateDelta({}, graph.nodeStates.front()));
  for (uint32_t i = 1; i < graph.nodeStates.size(); ++i)
  {
    const auto &prevState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i - 1)];
    const auto &currState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i)];
    result.emplace_back(getStateDelta(prevState, currState));
  }
  result.emplace_back(getStateDelta(graph.nodeStates.back(), {}));

  return result;
}

} // namespace dabfg
