#pragma once

#include <EASTL/span.h>
#include <EASTL/optional.h>

#include <dag/dag_vector.h>

#include <generic/dag_fixedVectorMap.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_overrideStates.h>
#include <memory/dag_framemem.h>

#include <render/daBfg/stage.h>
#include <3d/dag_drv3dConsts.h>

#include <render/daBfg/detail/resourceType.h>
#include <render/daBfg/detail/resNameId.h>
#include <render/daBfg/detail/projectors.h>


namespace dabfg
{

struct ShaderBlockLayersInfo
{
  int objectLayer = -1;
  int frameLayer = -1;
  int sceneLayer = -1;
};

struct VirtualSubresourceRef
{
  ResNameId nameId = ResNameId::Invalid;
  uint32_t mipLevel = 0;
  uint32_t layer = 0;
};

struct VirtualPassRequirements
{
  // Also known as "multiple render targets" -- correspond to pixel
  // shader outputs in the specified order.
  dag::RelocatableFixedVector<VirtualSubresourceRef, 8> colorAttachments;
  VirtualSubresourceRef depthAttachment;
  bool depthReadOnly = true;
};

struct VrsStateRequirements
{
  uint32_t rateX;
  uint32_t rateY;
  ResNameId rateTextureResId;
  VariableRateShadingCombiner vertexCombiner;
  VariableRateShadingCombiner pixelCombiner;
};

struct NodeStateRequirements
{
  // Toggles whether d3d::setwire should be turned on for this node
  // when wirefrime debug mode is enabled.
  bool supportsWireframe = false;

  // Toggles VRS
  VrsStateRequirements vrsState = {};

  // Toggles C++ overrides for rendering pipeline state
  // (which is usually configured in dagor shader code)
  eastl::optional<shaders::OverrideState> pipelineStateOverride = {};

  void setOverride(const shaders::OverrideState &state) { pipelineStateOverride = state; }
};

enum class BindingType
{
  ShaderVar,
  ViewMatrix,
  ProjMatrix,
  Invalid,
};

struct BindingInfo
{
  BindingType type = BindingType::Invalid;
  ResNameId nameId = ResNameId::Invalid;
  uint32_t multiplexingIdx = ~0u; // Workaround. TODO: reimplement binding logic with explicit public and internal binding info.
  bool history = false;
  bool optional = false;
  Stage stage = Stage::UNKNOWN;
  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Unknown;
  detail::TypeErasedProjector projector = [](void *data) { return data; };
};

using BindingsMap = dag::FixedVectorMap<int, BindingInfo, 8>;

struct VrsStateInternal
{
  uint32_t rateX = 1;
  uint32_t rateY = 1;
  ResNameId rateTexture = ResNameId::Invalid;
  VariableRateShadingCombiner vertexCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
  VariableRateShadingCombiner pixelCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
};

struct NodeStateInternal
{
  bool wire{};
  VrsStateInternal vrs{};
  eastl::optional<VirtualPassRequirements> rendering;
  eastl::optional<shaders::OverrideState> shaderOverrides;
  BindingsMap bindings;
  ShaderBlockLayersInfo shaderBlockLayers;
};

struct ShaderBlockLayersInfoDelta
{
  eastl::optional<int> objectLayer;
  eastl::optional<int> frameLayer;
  eastl::optional<int> sceneLayer;
};

// nullopt here means "no change"
struct NodeStateDelta
{
  eastl::optional<bool> wire;
  eastl::optional<VrsStateInternal> vrs;
  eastl::optional<VirtualPassRequirements> rendering;
  eastl::optional<shaders::UniqueOverrideStateId> shaderOverrides;
  BindingsMap bindings;
  ShaderBlockLayersInfoDelta shaderBlockLayers;
};

dag::Vector<NodeStateDelta> calculate_per_node_state_deltas(eastl::span<NodeStateInternal const> nodeStates);

} // namespace dabfg
