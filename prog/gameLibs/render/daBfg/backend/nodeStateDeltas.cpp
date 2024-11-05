// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeStateDeltas.h"

#include <EASTL/algorithm.h>
#include <EASTL/iterator.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <util/dag_globDef.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_renderPass.h>


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
  return first.rateX == second.rateX && first.rateY == second.rateY && first.vertexCombiner == second.vertexCombiner &&
         first.pixelCombiner == second.pixelCombiner;
}

static bool operator==(const SubresourceRef &first, const SubresourceRef &second)
{
  return first.resource == second.resource && first.mipLevel == second.mipLevel && first.layer == second.layer;
}

static bool operator==(const RenderPass &first, const RenderPass &second)
{
  return first.colorAttachments == second.colorAttachments && first.depthAttachment == second.depthAttachment &&
         first.depthReadOnly == second.depthReadOnly && first.isLegacyPass == second.isLegacyPass &&
         first.vrsRateAttachment == second.vrsRateAttachment;
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

namespace sd
{

struct DeltaCalculator
{
  // Use this overload set to add special processing for fields
  // that change type.
  const intermediate::Graph &graph;
  const char *nodeName = "";

  template <class U>
  U delta(const U &current, const U &previous) const
  {
    G_UNUSED(previous);
    return current;
  }

  shaders::UniqueOverrideStateId delta(const eastl::optional<shaders::OverrideState> &current,
    const eastl::optional<shaders::OverrideState> &) const
  {
    return current.has_value() ? shaders::overrides::create(*current) : shaders::OverrideStateId{};
  }

  RenderPassDelta delta(const eastl::optional<intermediate::RenderPass> &current,
    const eastl::optional<intermediate::RenderPass> &previous) const
  {
    PassChange result;

    if (previous.has_value())
    {
      if (previous->isLegacyPass)
        result.endAction = LegacyBackbufferPass{};
      else
        result.endAction = FinishRenderPass{};
    }

    if (current.has_value())
    {
      if (current->isLegacyPass)
      {
        result.beginAction = LegacyBackbufferPass{};
      }
      else
      {
        auto &start = result.beginAction.emplace<BeginRenderPass>();

        // +1 here is for the depth/stencil attachment
        const auto approxTotalAttachments = current->colorAttachments.size() + current->resolves.size() + 1;
        dag::Vector<RenderPassTargetDesc, framemem_allocator> targets;
        targets.reserve(approxTotalAttachments);
        dag::Vector<RenderPassBind, framemem_allocator> binds;
        targets.reserve(approxTotalAttachments);

        const auto processAttachment = [&targets, &binds, &start, &current, this](int32_t slot, RenderPassTargetAction main_action,
                                         RenderPassTargetAction final_action, const intermediate::SubresourceRef &subres) {
          const auto clearIt = current->clears.find(subres.resource);
          const bool shouldClear = clearIt != current->clears.end();
          {
            const auto attIdx = static_cast<int32_t>(targets.size());

            const auto totalMainAction = main_action | (shouldClear ? RP_TA_LOAD_CLEAR : RP_TA_LOAD_READ);
            binds.push_back(RenderPassBind{attIdx, 0, slot, totalMainAction, {}});

            constexpr auto ESP = RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END;
            binds.push_back(RenderPassBind{attIdx, ESP, slot, final_action, {}});
          }

          {
            const auto &res = graph.resources[subres.resource];
            const auto texcf = res.isScheduled() ? res.asScheduled().getGpuDescription().asBasicTexRes.cFlags
                                                 : eastl::get<TextureInfo>(res.asExternal().info).cflg;
            targets.push_back(RenderPassTargetDesc{nullptr, texcf, false});
          }

          start.attachments.push_back(
            {subres.resource, subres.mipLevel, subres.layer, shouldClear ? clearIt->second : ResourceClearValue{}});

          if (auto it = current->resolves.find(subres.resource); it != current->resolves.end())
          {
            const auto resolveTgtResIdx = it->second;

            {
              const auto resolveAttIdx = static_cast<int32_t>(targets.size());
              const auto flags = RP_TA_SUBPASS_RESOLVE | RP_TA_LOAD_NO_CARE | RP_TA_STORE_WRITE;
              binds.push_back(RenderPassBind{resolveAttIdx, 0, slot, flags, {}});
            }

            {
              const auto &resolveTgt = graph.resources[resolveTgtResIdx];
              const auto texcf = resolveTgt.isScheduled() ? resolveTgt.asScheduled().getGpuDescription().asBasicTexRes.cFlags
                                                          : eastl::get<TextureInfo>(resolveTgt.asExternal().info).cflg;
              targets.push_back(RenderPassTargetDesc{nullptr, texcf, false});
            }

            start.attachments.push_back({resolveTgtResIdx, subres.mipLevel, subres.layer, ResourceClearValue{}});
          }
        };

        // TODO: we should optimize out unnecessary loads/stores in the future.

        for (int32_t i = 0; i < current->colorAttachments.size(); ++i)
          if (auto maybeAtt = current->colorAttachments[i])
            processAttachment(i, RP_TA_SUBPASS_WRITE, RP_TA_STORE_WRITE, *maybeAtt);

        if (auto maybeAtt = current->depthAttachment)
        {
          const auto mainDepthAction = current->depthReadOnly ? RP_TA_SUBPASS_READ : RP_TA_SUBPASS_WRITE;
          const auto finalDepthAction = current->depthReadOnly ? RP_TA_STORE_NONE : RP_TA_STORE_WRITE;
          constexpr auto DS = RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;
          processAttachment(DS, mainDepthAction, finalDepthAction, *maybeAtt);
        }

        start.rp = eastl::unique_ptr<d3d::RenderPass, RpDestroyer>{
          d3d::create_render_pass(RenderPassDesc{nodeName, targets.size(), binds.size(), targets.data(), binds.data(), 0})};

        // TODO: when NRPs support VRS, use that instead of this hackaround
        if (auto maybeAtt = current->vrsRateAttachment)
        {
          start.vrsRateAttachment = Attachment{maybeAtt->resource, maybeAtt->mipLevel, maybeAtt->layer, {}};
        }
      }
    }

    return result;
  }

  intermediate::BindingsMap delta(const intermediate::BindingsMap &old_binding, const intermediate::BindingsMap &new_binding) const
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
    const intermediate::ShaderBlockBindings &new_info) const
  {
    ShaderBlockLayersInfoDelta result;
    result.objectLayer = new_info.objectLayer != old_info.objectLayer ? eastl::optional<int>(new_info.objectLayer) : eastl::nullopt;
    result.sceneLayer = new_info.sceneLayer != old_info.sceneLayer ? eastl::optional<int>(new_info.sceneLayer) : eastl::nullopt;
    result.frameLayer = new_info.frameLayer != old_info.frameLayer ? eastl::optional<int>(new_info.frameLayer) : eastl::nullopt;
    return result;
  }
};

NodeStateDelta getStateDelta(const DeltaCalculator &calculator, const intermediate::RequiredNodeState &first,
  const intermediate::RequiredNodeState &second, bool force_pass_break)
{
  NodeStateDelta result;

#define DELTA(field)                                                     \
  do                                                                     \
    if (!(first.field == second.field))                                  \
      result.field.emplace(calculator.delta(second.field, first.field)); \
  while (false)

  DELTA(asyncPipelines);
  DELTA(wire);
  DELTA(vrs);
  DELTA(shaderOverrides);

#undef DELTA

  if (force_pass_break || first.pass != second.pass)
    result.pass.emplace(calculator.delta(second.pass, first.pass));

  result.bindings = calculator.delta(first.bindings, second.bindings);
  result.shaderBlockLayers = calculator.delta(first.shaderBlockLayers, second.shaderBlockLayers);

  return result;
}

NodeStateDeltas calculate_per_node_state_deltas(const intermediate::Graph &graph, const BarrierScheduler::EventsCollectionRef &events)
{
  NodeStateDeltas result;
  result.reserve(graph.nodeStates.size() + 1);

  if (graph.nodeStates.empty())
  {
    result.appendNew();
    return result;
  }

  DeltaCalculator calculator{graph};

  calculator.nodeName = graph.nodeNames.front().c_str();
  result.appendNew(getStateDelta(calculator, {}, graph.nodeStates.front(), true));
  for (uint32_t i = 1; i < graph.nodeStates.size(); ++i)
  {
    bool forcePassBreak = false;
    for (int frame = 0; frame < BarrierScheduler::SCHEDULE_FRAME_WINDOW; ++frame)
      for (const auto &ev : events[frame][i])
        if (eastl::holds_alternative<BarrierScheduler::Event::Barrier>(ev.data))
          forcePassBreak = true;
    const auto &prevState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i - 1)];
    const auto &currState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i)];
    calculator.nodeName = graph.nodeNames[static_cast<intermediate::NodeIndex>(i)].c_str();
    result.appendNew(getStateDelta(calculator, prevState, currState, forcePassBreak));
  }
  result.appendNew(getStateDelta(calculator, graph.nodeStates.back(), {}, true));

  return result;
}

} // namespace sd

} // namespace dabfg
