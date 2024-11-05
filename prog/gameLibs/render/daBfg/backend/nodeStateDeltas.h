// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <drv/3d/dag_renderPass.h>


// sd stands for state deltas
namespace dabfg::sd
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
  eastl::variant<ResourceClearValue, intermediate::DynamicPassParameter> clearValue;
};

struct BeginRenderPass
{
  eastl::unique_ptr<d3d::RenderPass, RpDestroyer> rp;
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

struct NodeStateDelta
{
  eastl::optional<bool> asyncPipelines;
  eastl::optional<bool> wire;
  eastl::optional<intermediate::VrsState> vrs;
  eastl::optional<RenderPassDelta> pass;
  eastl::optional<shaders::UniqueOverrideStateId> shaderOverrides;
  intermediate::BindingsMap bindings;
  ShaderBlockLayersInfoDelta shaderBlockLayers;
};

using NodeStateDeltas = IdIndexedMapping<intermediate::NodeIndex, NodeStateDelta>;

NodeStateDeltas calculate_per_node_state_deltas(const intermediate::Graph &graph, const BarrierScheduler::EventsCollectionRef &events);

} // namespace dabfg::sd

DAG_DECLARE_RELOCATABLE(dabfg::sd::Attachment);
