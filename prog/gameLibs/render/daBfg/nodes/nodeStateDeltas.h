#pragma once

#include <intermediateRepresentation.h>


namespace dabfg
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

struct StopVrs
{};
using VrsStateDelta = eastl::variant<StopVrs, intermediate::VrsState>;
struct FinishRenderPass
{};
using RenderPassDelta = eastl::variant<FinishRenderPass, intermediate::RenderPass>;

struct NodeStateDelta
{
  eastl::optional<bool> wire;
  eastl::optional<VrsStateDelta> vrs;
  eastl::optional<RenderPassDelta> pass;
  eastl::optional<shaders::UniqueOverrideStateId> shaderOverrides;
  intermediate::BindingsMap bindings;
  ShaderBlockLayersInfoDelta shaderBlockLayers;
};

using NodeStateDeltas = IdIndexedMapping<intermediate::NodeIndex, NodeStateDelta>;

NodeStateDeltas calculate_per_node_state_deltas(const intermediate::Graph &graph);

} // namespace dabfg
