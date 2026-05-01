// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeStateDeltas.h"

#include <backend/intermediateRepresentation.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_renderPass.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_pairwiseView.h>
#include <memory/dag_framemem.h>


namespace dafg
{

namespace intermediate
{

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

void DeltaCalculator::invalidateCachesForAutoResType(AutoResTypeNameId id)
{
  if (!rpCacheKeysByAutoResType.isMapped(id))
    return;
  auto &keys = rpCacheKeysByAutoResType[id];
  for (const auto &key : keys)
    rpCache.erase(key);
  keys.clear();
}

IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> DeltaCalculator::precomputeResourceState(
  const BarrierScheduler::EventsCollection &events,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed)
{
  using ResourceIndex = intermediate::ResourceIndex;
  const auto totalResources = graph.resources.totalKeys();

  // Allocate the output first so it's at the BOTTOM of the framemem stack. The prev*
  // snapshots below must be freed (at function exit) while the output is still alive
  // and returned to the caller -- which only works LIFO if prev* sit ABOVE the output.
  IdIndexedFlags<ResourceIndex, framemem_allocator> dirtyResources(resources_changed);

  // Snapshot the previous values before we overwrite the class members. New slots
  // (resources that didn't exist on the previous compile) get default "unseen" values
  // so they compare unequal against any actual activation / access below and get
  // flagged dirty automatically. Scoped so they're freed before the function returns.

  IdIndexedMapping<ResourceIndex, uint16_t, framemem_allocator> prevFirstActivationPosition(firstActivationPosition.begin(),
    firstActivationPosition.end());
  if (prevFirstActivationPosition.size() < totalResources)
    prevFirstActivationPosition.resize(totalResources, UINT16_MAX);
  IdIndexedMapping<ResourceIndex, uint16_t, framemem_allocator> prevFirstAccessPosition(firstAccessPosition.begin(),
    firstAccessPosition.end());
  if (prevFirstAccessPosition.size() < totalResources)
    prevFirstAccessPosition.resize(totalResources, UINT16_MAX);
  IdIndexedFlags<ResourceIndex, framemem_allocator> prevBaseInitialized(baseInitialized);
  if (prevBaseInitialized.size() < totalResources)
    prevBaseInitialized.resize(totalResources, false);

  firstActivationPosition.assign(totalResources, UINT16_MAX);
  firstAccessPosition.assign(totalResources, UINT16_MAX);
  baseInitialized.assign(totalResources, false);

  for (auto idx : graph.resources.keys())
  {
    const auto &res = graph.resources[idx];
    if (res.isExternal() || res.isDriverDeferredTexture())
      baseInitialized[idx] = true;
    if (res.isScheduled() && res.asScheduled().isGpuResource() && res.asScheduled().clearStage == intermediate::ClearStage::Activation)
      baseInitialized[idx] = true;
  }

  uint16_t pos = 0;
  for (auto [nodeIdx, nodeState] : graph.nodeStates.enumerate())
  {
    for (int frame = 0; frame < BarrierScheduler::SCHEDULE_FRAME_WINDOW; ++frame)
      for (const auto &ev : events[frame][nodeIdx])
        if (eastl::holds_alternative<BarrierScheduler::Event::Activation>(ev.data))
          firstActivationPosition[ev.resource] = eastl::min(firstActivationPosition[ev.resource], pos);

    if (nodeState.pass.has_value() && !nodeState.pass->isLegacyPass)
    {
      for (const auto &maybeAtt : nodeState.pass->colorAttachments)
        if (maybeAtt)
          firstAccessPosition[maybeAtt->resource] = eastl::min(firstAccessPosition[maybeAtt->resource], pos);
      if (nodeState.pass->depthAttachment)
        firstAccessPosition[nodeState.pass->depthAttachment->resource] =
          eastl::min(firstAccessPosition[nodeState.pass->depthAttachment->resource], pos);
      if (nodeState.pass->vrsRateAttachment)
        firstAccessPosition[nodeState.pass->vrsRateAttachment->resource] =
          eastl::min(firstAccessPosition[nodeState.pass->vrsRateAttachment->resource], pos);
    }

    ++pos;
  }

  // Merge internal state changes into the output.
  for (auto resIdx : graph.resources.keys())
    if (firstActivationPosition[resIdx] != prevFirstActivationPosition[resIdx] ||
        firstAccessPosition[resIdx] != prevFirstAccessPosition[resIdx] || baseInitialized[resIdx] != prevBaseInitialized[resIdx])
      dirtyResources[resIdx] = true;


  return dirtyResources;
}

shaders::UniqueOverrideStateId DeltaCalculator::delta(const eastl::optional<shaders::OverrideState> &current,
  const eastl::optional<shaders::OverrideState> &) const
{
  return current.has_value() ? shaders::overrides::create(*current) : shaders::OverrideStateId{};
}

RenderPassDelta DeltaCalculator::delta(const eastl::optional<intermediate::RenderPass> &current,
  const eastl::optional<intermediate::RenderPass> &previous, uint16_t schedule_position)
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

      RPsDescKey rpDescKey{};

      dag::RelocatableFixedVector<RenderPassTargetDesc, 8> &targets = rpDescKey.targets;
      targets.reserve(approxTotalAttachments);
      dag::RelocatableFixedVector<RenderPassBind, 16> &binds = rpDescKey.binds;
      binds.reserve(approxTotalAttachments);

      dag::VectorSet<intermediate::ResourceIndex, eastl::less<intermediate::ResourceIndex>, framemem_allocator> localAccessedThisNode;
      localAccessedThisNode.reserve(approxTotalAttachments);

      dag::VectorSet<AutoResTypeNameId, eastl::less<AutoResTypeNameId>, framemem_allocator> usedAutoResTypes;
      usedAutoResTypes.reserve(approxTotalAttachments);

      const auto collectAutoResType = [&usedAutoResTypes](const intermediate::Resource &r) {
        if (r.isScheduled() && r.asScheduled().resolutionType.has_value())
          usedAutoResTypes.insert(r.asScheduled().resolutionType->id);
      };

      const auto processAttachment = [&targets, &binds, &start, &current, &localAccessedThisNode, &collectAutoResType,
                                       schedule_position, this](int32_t slot, RenderPassTargetAction main_action,
                                       RenderPassTargetAction final_action, const intermediate::SubresourceRef &subres) {
        const bool cleared = firstAccessPosition[subres.resource] < schedule_position || localAccessedThisNode.count(subres.resource);
        const auto &res = graph.resources[subres.resource];
        localAccessedThisNode.insert(subres.resource);
        collectAutoResType(res);
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

          const bool initialized =
            baseInitialized.test(subres.resource, false) || firstActivationPosition[subres.resource] < schedule_position;

          auto targetClearAction = RP_TA_LOAD_CLEAR;

          if (shouldClear)
          {
            if (slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
            {
              const auto scheduledResClearFlags = res.asScheduled().clearFlags;

              bool clearDepth = (scheduledResClearFlags & ResourceClearFlags::RESOURCE_CLEAR_DEPTH);
              bool clearStencil = (scheduledResClearFlags & ResourceClearFlags::RESOURCE_CLEAR_STENCIL);

              G_ASSERT_LOG(clearDepth || clearStencil,
                "invalid resource clear flags: clear stage is RenderPass, but DEPTH and STENCIL flags are both not set");

              if (!clearDepth || !clearStencil)
              {
                if (clearDepth)
                {
                  targetClearAction = RP_TA_LOAD_STENCIL_READ | RP_TA_LOAD_CLEAR;
                }
                else
                {
                  // stencil only clear
                  targetClearAction = RP_TA_LOAD_READ | RP_TA_LOAD_STENCIL_CLEAR;
                }
              }
            }
          }


          const auto totalMainAction =
            main_action | (shouldClear ? targetClearAction : (initialized ? RP_TA_LOAD_READ : RP_TA_LOAD_NO_CARE));

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
          collectAutoResType(resolveTgt);
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

      start.useVrs = false;
      if (auto maybeAtt = current->vrsRateAttachment)
      {
        constexpr auto VRSslot = RenderPassExtraIndexes::RP_SLOT_VRS_TEXTURE;
        processAttachment(VRSslot, RP_TA_SUBPASS_VRS_READ, RP_TA_STORE_NONE, *maybeAtt);
        start.useVrs = true;
      }

      {
        d3d::RenderPass *newRP = nullptr;

        auto cacheIterator = rpCache.find(rpDescKey);
        if (cacheIterator == rpCache.end())
        {
          newRP = d3d::create_render_pass(
            {nodeName, rpDescKey.targets.size(), rpDescKey.binds.size(), rpDescKey.targets.data(), rpDescKey.binds.data(), 0});
          rpCache.emplace(rpDescKey, eastl::unique_ptr<d3d::RenderPass, RpDestroyer>(newRP));
          for (auto typeId : usedAutoResTypes)
            rpCacheKeysByAutoResType.get(typeId).insert(rpDescKey);
        }
        else
        {
          newRP = cacheIterator->second.get();
        }

        start.rp = newRP;
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
      result[k] = {v.type, eastl::nullopt, false, v.reset, v.optional, v.projectedTag};

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
  const intermediate::RequiredNodeState &second, bool force_pass_break, uint16_t schedule_position)
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
    result.pass.emplace(delta(second.pass, first.pass, schedule_position));

  result.bindings = delta(first.bindings, second.bindings);
  result.shaderBlockLayers = delta(first.shaderBlockLayers, second.shaderBlockLayers);
  for (auto [stream, vertexSource] : enumerate(result.vertexSources))
    vertexSource = delta(first.vertexSources[stream], second.vertexSources[stream]);
  result.indexSource = delta(first.indexSource, second.indexSource);

  return result;
}

IdIndexedFlags<intermediate::NodeIndex> DeltaCalculator::computeDirtyDeltas(const NodeStateDeltas &result,
  const BarrierScheduler::EventsCollection &events, const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &dirty_resources)
{
  FRAMEMEM_VALIDATE;

  using NodeIndex = intermediate::NodeIndex;

  IdIndexedFlags<NodeIndex> dirtyDeltas(eastl::max(graph.nodeStates.totalKeys(), result.totalKeys()), false);

  const auto needsPassBreak = [&](NodeIndex node_idx) {
    if (node_idx == graph.nodeStates.frontKey())
      return true;
    for (int frame = 0; frame < BarrierScheduler::SCHEDULE_FRAME_WINDOW; ++frame)
      for (const auto &ev : events[frame][node_idx])
        if (eastl::holds_alternative<BarrierScheduler::Event::Barrier>(ev.data))
          return true;
    return false;
  };

  // First run: everything dirty
  if (result.empty())
  {
    dirtyDeltas.assign(graph.nodeStates.totalKeys(), true);

    cachedForcePassBreak.resize(graph.nodeStates.totalKeys(), false);
    for (auto nodeIdx : graph.nodeStates.keys())
      cachedForcePassBreak[nodeIdx] = needsPassBreak(nodeIdx);

    return dirtyDeltas;
  }

  for (auto [cur, next] : dag::PairwiseView(result.keys()))
    if (!graph.nodeStates.isMapped(cur))
      dirtyDeltas[cur] = dirtyDeltas[next] = true;

  const auto requestedResChanged = [&](intermediate::NodeIndex cur) {
    for (const auto &req : graph.nodes[cur].resourceRequests)
      if (dirty_resources[req.resource])
        return true;
    return false;
  };

  for (auto [cur, next] : dag::PairwiseView(graph.nodeStates.keys()))
    if (nodes_changed[cur] || requestedResChanged(cur))
      dirtyDeltas[cur] = dirtyDeltas[next] = true;

  cachedForcePassBreak.resize(graph.nodeStates.totalKeys(), false);
  for (auto nodeIdx : graph.nodeStates.keys())
  {
    const bool fpb = needsPassBreak(nodeIdx);
    if (cachedForcePassBreak[nodeIdx] != fpb)
      dirtyDeltas[nodeIdx] = true;
    cachedForcePassBreak[nodeIdx] = fpb;
  }

  return dirtyDeltas;
}

void DeltaCalculator::calculatePerNodeStateDeltas(NodeStateDeltas &result, const BarrierScheduler::EventsCollection &events,
  const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed)
{
  if (graph.nodeStates.empty())
  {
    result.clear();
    result.emplaceAt(intermediate::NodeIndex{0});
    return;
  }

  auto dirtyResources = precomputeResourceState(events, resources_changed);
  auto dirtyDeltas = computeDirtyDeltas(result, events, nodes_changed, dirtyResources);

  // Erase entries for removed nodes and dirty deltas (will be recomputed below).
  {
    auto enumView = result.enumerate();
    for (auto it = enumView.begin(); it != enumView.end();)
    {
      auto [key, _] = *it;
      if (!graph.nodeStates.isMapped(key) || dirtyDeltas[key])
        it = result.erase(key);
      else
        ++it;
    }
  }

  // First node: no predecessor in the schedule, so its delta transitions from {} to
  // its own state. In practice the first node is the IR graph's source sentinel with
  // empty state, making this a no-op -- but computing it explicitly keeps the code
  // correct for any ordering.
  const auto firstNodeIdx = graph.nodeStates.frontKey();
  if (dirtyDeltas[firstNodeIdx])
  {
    nodeName = graph.nodeNames[firstNodeIdx].c_str();
    result.emplaceAt(firstNodeIdx, getStateDelta({}, graph.nodeStates[firstNodeIdx], true, 0));
  }

  // Remaining nodes: each delta compares against its schedule predecessor. The final
  // pair is (last_real_node, destination_sentinel); since the dst sentinel's state is
  // empty, that delta naturally encodes the end-of-frame state reset -- no separate
  // cleanup slot is needed.
  uint16_t schedulePos = 1;
  for (auto [prev, cur] : dag::PairwiseView(graph.nodeStates.enumerate()))
  {
    const auto curIdx = eastl::get<0>(cur);
    if (dirtyDeltas[curIdx])
    {
      nodeName = graph.nodeNames[curIdx].c_str();
      const bool fpb = cachedForcePassBreak[curIdx];
      result.emplaceAt(curIdx, getStateDelta(eastl::get<1>(prev), eastl::get<1>(cur), fpb, schedulePos));
    }
    ++schedulePos;
  }
}

} // namespace sd

} // namespace dafg
