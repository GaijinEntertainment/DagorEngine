// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeStateDeltas.h"

#include <backend/intermediateRepresentation.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_renderPass.h>
#include <generic/dag_enumerate.h>
#include <memory/dag_framemem.h>


namespace dafg
{

namespace intermediate
{

// NOTE: clears must be ignored when diffing RenderPasses
static bool operator==(const RenderPass &first, const RenderPass &second)
{
  return first.depthAttachment == second.depthAttachment && first.depthReadOnly == second.depthReadOnly &&
         first.isLegacyPass == second.isLegacyPass && first.vrsRateAttachment == second.vrsRateAttachment &&
         first.colorAttachments == second.colorAttachments && first.resolves == second.resolves;
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

DeltaCalculator::DeltaCalculator(const intermediate::Graph &_graph) : graph(_graph) {}

shaders::UniqueOverrideStateId DeltaCalculator::delta(const eastl::optional<shaders::OverrideState> &current,
  const eastl::optional<shaders::OverrideState> &) const
{
  return current.has_value() ? shaders::overrides::create(*current) : shaders::OverrideStateId{};
}

RenderPassDelta DeltaCalculator::delta(const eastl::optional<intermediate::RenderPass> &current,
  const eastl::optional<intermediate::RenderPass> &previous)
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

      RPsDescKey rpDescKey;
      dag::RelocatableFixedVector<RenderPassTargetDesc, 8> &targets = rpDescKey.targets;
      targets.reserve(approxTotalAttachments);
      dag::RelocatableFixedVector<RenderPassBind, 16> &binds = rpDescKey.binds;
      binds.reserve(approxTotalAttachments);

      const auto processAttachment = [&targets, &binds, &start, &current, this](int32_t slot, RenderPassTargetAction main_action,
                                       RenderPassTargetAction final_action, const intermediate::SubresourceRef &subres) {
        const bool cleared = resourceAccessed[subres.resource];
        const auto &res = graph.resources[subres.resource];
        resourceAccessed[subres.resource] = true;
        eastl::variant<ResourceClearValue, dafg::intermediate::DynamicParameter> clearValue = {ResourceClearValue{}};
        bool shouldClear = false;
        if (!cleared)
        {
          if (res.isScheduled() && res.asScheduled().isGpuResource() &&
              res.asScheduled().clearStage == intermediate::ClearStage::RenderPass)
          {
            clearValue = res.asScheduled().clearValue;
            shouldClear = true;
          }
        }
        {
          const auto attIdx = static_cast<int32_t>(targets.size());

          const bool initialized = resourceInitialized[subres.resource];

          const auto totalMainAction =
            main_action | (shouldClear ? RP_TA_LOAD_CLEAR : (initialized ? RP_TA_LOAD_READ : RP_TA_LOAD_NO_CARE));

          binds.push_back(RenderPassBind{attIdx, 0, slot, totalMainAction, {}});

          constexpr auto ESP = RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END;
          binds.push_back(RenderPassBind{attIdx, ESP, slot, final_action, {}});
        }

        {
          const auto texcf = res.isScheduled() ? res.asScheduled().getGpuDescription().asBasicTexRes.cFlags
                                               : eastl::get<TextureInfo>(res.asExternal().info).cflg;
          targets.push_back(RenderPassTargetDesc{nullptr, texcf, false});
        }

        start.attachments.push_back({subres.resource, subres.mipLevel, subres.layer, clearValue});

        if (auto it = current->resolves.find(subres.resource); it != current->resolves.end())
        {
          const auto resolveTgtResIdx = it->second;
          {
            const auto resolveAttIdx = static_cast<int32_t>(targets.size());
            const auto flags = RP_TA_SUBPASS_RESOLVE | RP_TA_LOAD_NO_CARE | RP_TA_STORE_WRITE;
            binds.push_back(RenderPassBind{resolveAttIdx, 0, slot, flags, {}});
          }

          const auto &resolveTgt = graph.resources[resolveTgtResIdx];
          {
            const auto texcf = resolveTgt.isScheduled() ? resolveTgt.asScheduled().getGpuDescription().asBasicTexRes.cFlags
                                                        : eastl::get<TextureInfo>(resolveTgt.asExternal().info).cflg;
            targets.push_back(RenderPassTargetDesc{nullptr, texcf, false});
          }

          eastl::variant<ResourceClearValue, dafg::intermediate::DynamicParameter> clearValue = {ResourceClearValue{}};
          if (resolveTgt.isScheduled() && resolveTgt.asScheduled().isGpuResource() &&
              resolveTgt.asScheduled().clearStage == intermediate::ClearStage::RenderPass)
            clearValue = resolveTgt.asScheduled().clearValue;

          start.attachments.push_back({resolveTgtResIdx, subres.mipLevel, subres.layer, clearValue});
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

      {
        d3d::RenderPass *newRP = nullptr;

        auto cacheIterator = rpCache.find(rpDescKey);
        if (cacheIterator == rpCache.end())
        {
          newRP = d3d::create_render_pass(
            {nodeName, rpDescKey.targets.size(), rpDescKey.binds.size(), rpDescKey.targets.data(), rpDescKey.binds.data(), 0});
          rpCache.emplace(rpDescKey, eastl::unique_ptr<d3d::RenderPass, RpDestroyer>(newRP));
        }
        else
        {
          newRP = cacheIterator->second.get();
        }

        start.rp = newRP;
      }

      // TODO: when NRPs support VRS, use that instead of this hackaround
      if (auto maybeAtt = current->vrsRateAttachment)
      {
        start.vrsRateAttachment = Attachment{maybeAtt->resource, maybeAtt->mipLevel, maybeAtt->layer, {}};
      }
    }
  }

  return result;
}

sd::BindingsMap DeltaCalculator::delta(const intermediate::BindingsMap &old_binding,
  const intermediate::BindingsMap &new_binding) const
{
  if (old_binding.empty() && new_binding.empty())
    return {};

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

ShaderBlockLayersInfoDelta DeltaCalculator::delta(const intermediate::ShaderBlockBindings &old_info,
  const intermediate::ShaderBlockBindings &new_info) const
{
  ShaderBlockLayersInfoDelta result;
  result.objectLayer = new_info.objectLayer != old_info.objectLayer ? eastl::optional<int>(new_info.objectLayer) : eastl::nullopt;
  result.sceneLayer = new_info.sceneLayer != old_info.sceneLayer ? eastl::optional<int>(new_info.sceneLayer) : eastl::nullopt;
  result.frameLayer = new_info.frameLayer != old_info.frameLayer ? eastl::optional<int>(new_info.frameLayer) : eastl::nullopt;
  return result;
}

eastl::optional<intermediate::VertexSource> DeltaCalculator::delta(const eastl::optional<intermediate::VertexSource> &old_source,
  const eastl::optional<intermediate::VertexSource> &new_source) const
{
  if (!new_source.has_value() && old_source.has_value())
    return intermediate::VertexSource{eastl::nullopt, 0};

  if (new_source.has_value())
    if (!old_source.has_value() || old_source->buffer != new_source->buffer || old_source->stride != new_source->stride)
      return new_source;

  return eastl::nullopt;
}

eastl::optional<intermediate::IndexSource> DeltaCalculator::delta(const eastl::optional<intermediate::IndexSource> &old_source,
  const eastl::optional<intermediate::IndexSource> &new_source) const
{
  if (!new_source.has_value() && old_source.has_value())
    return intermediate::IndexSource{eastl::nullopt};

  if (new_source.has_value())
    if (!old_source.has_value() || old_source->buffer != new_source->buffer)
      return new_source;

  return eastl::nullopt;
}

NodeStateDelta DeltaCalculator::getStateDelta(const intermediate::RequiredNodeState &first,
  const intermediate::RequiredNodeState &second, bool force_pass_break)
{
  NodeStateDelta result;

#define DELTA(field)                                          \
  do                                                          \
    if (!(first.field == second.field))                       \
      result.field.emplace(delta(second.field, first.field)); \
  while (false)

  DELTA(asyncPipelines);
  DELTA(wire);
  DELTA(vrs);
  DELTA(shaderOverrides);

#undef DELTA

  if (force_pass_break || first.pass != second.pass)
    result.pass.emplace(delta(second.pass, first.pass));

  result.bindings = delta(first.bindings, second.bindings);
  result.shaderBlockLayers = delta(first.shaderBlockLayers, second.shaderBlockLayers);
  for (auto [stream, vertexSource] : enumerate(result.vertexSources))
    vertexSource = delta(first.vertexSources[stream], second.vertexSources[stream]);
  result.indexSource = delta(first.indexSource, second.indexSource);

  return result;
}

NodeStateDeltas DeltaCalculator::calculatePerNodeStateDeltas(const BarrierScheduler::EventsCollectionRef &events)
{
  NodeStateDeltas result;
  result.reserve(graph.nodeStates.size() + 1);

  if (graph.nodeStates.empty())
  {
    result.appendNew();
    return result;
  }

  resourceInitialized = {graph.resources.size(), false};
  resourceAccessed = {graph.resources.size(), false};

  // TODO: check whether the external and driver deferred resources were actually activated or not instead of assuming they are.
  for (uint32_t i = 0; i < graph.resources.size(); i++)
  {
    intermediate::ResourceIndex idx = intermediate::ResourceIndex(i);

    const auto &res = graph.resources[idx];
    if (res.isExternal() || res.isDriverDeferredTexture())
      resourceInitialized[idx] = true;
  }

  nodeName = graph.nodeNames.front().c_str();
  result.appendNew(getStateDelta({}, graph.nodeStates.front(), true));
  for (uint32_t i = 1; i < graph.nodeStates.size(); ++i)
  {
    bool forcePassBreak = false;
    for (int frame = 0; frame < BarrierScheduler::SCHEDULE_FRAME_WINDOW; ++frame)
    {
      for (const auto &ev : events[frame][i])
        if (eastl::holds_alternative<BarrierScheduler::Event::Barrier>(ev.data))
          forcePassBreak = true;

      // here we check for the earliest occurence of a resource being initialized by checking
      // if the previous node has activated it
      for (const auto &ev : events[frame][i - 1])
        if (eastl::holds_alternative<BarrierScheduler::Event::Activation>(ev.data))
          resourceInitialized[ev.resource] = true;
    }

    const auto &prevState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i - 1)];
    const auto &currState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i)];
    nodeName = graph.nodeNames[static_cast<intermediate::NodeIndex>(i)].c_str();
    result.appendNew(getStateDelta(prevState, currState, forcePassBreak));
  }
  result.appendNew(getStateDelta(graph.nodeStates.back(), {}, true));

  return result;
}

} // namespace sd

} // namespace dafg
