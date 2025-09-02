// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <drv/3d/dag_renderPass.h>
#include <id/idIndexedFlags.h>
#include <common/cycAlloc.h>

#include <ska_hash_map/flat_hash_map2.hpp>


template <typename T>
static void hash_combine(std::size_t &seed, const T &v)
{
  eastl::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// sd stands for state deltas
namespace dafg::sd
{

struct ShaderBlockLayersInfoDelta
{
  eastl::optional<int> objectLayer;
  eastl::optional<int> frameLayer;
  eastl::optional<int> sceneLayer;
};

// This more akin to "commands" that tell the executor that some state
// needs to be updated in the driver. `nullopt` means that that
// particular piece of state does not need updating.

struct RpDestroyer
{
  void operator()(d3d::RenderPass *rp) const { d3d::delete_render_pass(rp); }
};

struct FinishRenderPass
{};

struct Attachment
{
  intermediate::ResourceIndex res;
  uint32_t mipLevel;
  uint32_t layer;
  eastl::variant<ResourceClearValue, intermediate::DynamicParameter> clearValue;
};

struct BeginRenderPass
{
  d3d::RenderPass *rp;
  dag::RelocatableFixedVector<Attachment, 9> attachments;
  eastl::optional<Attachment> vrsRateAttachment; // TODO: remove me when NRPs support VRS rate attachments
};

struct NextSubpass
{};

struct LegacyBackbufferPass
{};

struct PassChange
{
  eastl::variant<eastl::monostate, FinishRenderPass, LegacyBackbufferPass> endAction;
  eastl::variant<eastl::monostate, BeginRenderPass, LegacyBackbufferPass> beginAction;
};

using RenderPassDelta = eastl::variant<PassChange, NextSubpass>;

class DeltaCalculator;
using Alloc = CycAlloc<DeltaCalculator>;
using BindingsMap = dag::FixedVectorMap<int, intermediate::Binding, 8, true, Alloc>;

struct NodeStateDelta
{
  eastl::optional<bool> asyncPipelines;
  eastl::optional<bool> wire;
  eastl::optional<intermediate::VrsState> vrs;
  eastl::optional<RenderPassDelta> pass;
  eastl::optional<shaders::UniqueOverrideStateId> shaderOverrides;
  eastl::array<eastl::optional<intermediate::VertexSource>, intermediate::MAX_VERTEX_STREAMS> vertexSources;
  eastl::optional<intermediate::IndexSource> indexSource;
  BindingsMap bindings;
  ShaderBlockLayersInfoDelta shaderBlockLayers;
};

using NodeStateDeltas = IdIndexedMapping<intermediate::NodeIndex, NodeStateDelta, Alloc>;

struct RPsDescKey
{
  dag::RelocatableFixedVector<RenderPassTargetDesc, 8> targets;
  dag::RelocatableFixedVector<RenderPassBind, 16> binds;
  uint32_t subpassBindingOffset;

  bool operator==(const RPsDescKey &other) const = default;
};

class RPDescHasher
{
public:
  size_t operator()(const RPsDescKey &rpDesc) const
  {
    size_t result = 0;

    hash_combine(result, rpDesc.targets.size());
    hash_combine(result, rpDesc.binds.size());
    hash_combine(result, rpDesc.subpassBindingOffset);

    for (auto &prTarget : rpDesc.targets)
      hash_combine(result, (*this)(prTarget));
    for (auto &rpBind : rpDesc.binds)
      hash_combine(result, (*this)(rpBind));

    return result;
  }

private:
  size_t operator()(const RenderPassBind &rpBind) const
  {
    size_t result = 0;

    hash_combine(result, rpBind.target);
    hash_combine(result, rpBind.subpass);
    hash_combine(result, rpBind.slot);
    hash_combine(result, rpBind.action);
    hash_combine(result, rpBind.dependencyBarrier);

    return result;
  }

  size_t operator()(const RenderPassTargetDesc &rptDesc) const
  {
    size_t result = 0;
    if (rptDesc.templateResource)
      hash_combine(result, rptDesc.templateResource);
    else
      hash_combine(result, rptDesc.texcf);
    hash_combine(result, rptDesc.aliased);

    return result;
  }
};

class DeltaCalculator
{
public:
  DeltaCalculator(const intermediate::Graph &graph);

  NodeStateDeltas calculatePerNodeStateDeltas(const BarrierScheduler::EventsCollectionRef &events);

  // On some platforms, driver objects can have internal caches and aggregate
  // too much elements over time, so wiping at an opportune time is a good idea.
  void invalidateCaches() { rpCache.clear(); }

private:
  // Use this overload set to add special processing for fields
  // that change type.
  template <class U>
  U delta(const U &current, const U &previous) const
  {
    G_UNUSED(previous);
    return current;
  }

  shaders::UniqueOverrideStateId delta(const eastl::optional<shaders::OverrideState> &current,
    const eastl::optional<shaders::OverrideState> &) const;

  RenderPassDelta delta(const eastl::optional<intermediate::RenderPass> &current,
    const eastl::optional<intermediate::RenderPass> &previous);

  sd::BindingsMap delta(const intermediate::BindingsMap &old_binding, const intermediate::BindingsMap &new_binding) const;

  ShaderBlockLayersInfoDelta delta(const intermediate::ShaderBlockBindings &old_info,
    const intermediate::ShaderBlockBindings &new_info) const;

  eastl::optional<intermediate::VertexSource> delta(const eastl::optional<intermediate::VertexSource> &old_source,
    const eastl::optional<intermediate::VertexSource> &new_source) const;

  eastl::optional<intermediate::IndexSource> delta(const eastl::optional<intermediate::IndexSource> &old_source,
    const eastl::optional<intermediate::IndexSource> &new_source) const;

  NodeStateDelta getStateDelta(const intermediate::RequiredNodeState &first, const intermediate::RequiredNodeState &second,
    bool force_pass_break);

private:
  const intermediate::Graph &graph;
  ska::flat_hash_map<RPsDescKey, eastl::unique_ptr<d3d::RenderPass, RpDestroyer>, RPDescHasher> rpCache;
  const char *nodeName = "";
  IdIndexedFlags<intermediate::ResourceIndex> resourceInitialized;
  IdIndexedFlags<intermediate::ResourceIndex> resourceAccessed;
};

} // namespace dafg::sd

DAG_DECLARE_RELOCATABLE(dafg::sd::Attachment);
