#pragma once

#include <generic/dag_fixedVectorMap.h>

#include <render/daBfg/bfg.h>
#include <render/daBfg/externalResources.h>

#include <common/autoResolutionData.h>
#include <common/bindingType.h>
#include <frontend/resourceProvider.h>
#include <id/idIndexedMapping.h>
#include <id/idHierarchicalNameMap.h>


namespace dabfg
{

struct ResourceUsage
{
  Access access = Access::UNKNOWN;
  Usage type = Usage::UNKNOWN;
  Stage stage = Stage::UNKNOWN;
};

struct ResourceRequest
{
  ResourceUsage usage;
  bool slotRequest{false};
  bool optional{false};
  ResourceSubtypeTag subtypeTag{ResourceSubtypeTag::Unknown};
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

struct ShaderBlockLayersInfo
{
  int objectLayer = -1;
  int frameLayer = -1;
  int sceneLayer = -1;
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
  eastl::optional<VrsStateRequirements> vrsState = {};

  // Toggles C++ overrides for rendering pipeline state
  // (which is usually configured in dagor shader code)
  eastl::optional<shaders::OverrideState> pipelineStateOverride = {};
};

struct Binding
{
  BindingType type = BindingType::Invalid;
  ResNameId resource = ResNameId::Invalid;
  bool history = false;

  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Invalid;
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

using BindingsMap = dag::FixedVectorMap<int, Binding, 8>;

struct NodeData
{
  priority_t priority = PRIO_DEFAULT;
  multiplexing::Mode multiplexingMode = multiplexing::Mode::FullMultiplex;
  SideEffects sideEffect = SideEffects::Internal;
  // For debug purposes only
  bool enabled = true;

  // Keeps track of how many times the node was recreated
  uint16_t generation{0};
  detail::DeclarationCallback declare;
  detail::ExecutionCallback execute;

  dag::FixedVectorSet<NodeNameId, 4> precedingNodeIds;
  dag::FixedVectorSet<NodeNameId, 4> followingNodeIds;

  dag::FixedVectorSet<ResNameId, 8> createdResources;
  dag::FixedVectorSet<ResNameId, 8> modifiedResources;
  dag::FixedVectorSet<ResNameId, 8> readResources;
  // Maps renamed resource to previous name.
  dag::FixedVectorMap<ResNameId, ResNameId, 8> renamedResources;

  // Every resource present in maps/sets above shall have a
  // corresponding element here.
  dag::FixedVectorMap<ResNameId, ResourceRequest, 16> resourceRequests;

  // History resources are handled separately with a crutch:
  // we only allow history reads and list them separately from
  // everything above. The reason for this being is that we cannot
  // actually differentiate between a history and a non-history resource
  // dependency with only ResNameId. A universal solution would require
  // storing an additional flag for all structs above.
  // This is implementable, but not worth it right now.
  dag::FixedVectorMap<ResNameId, ResourceRequest, 16> historyResourceReadRequests;


  eastl::optional<NodeStateRequirements> stateRequirements;
  eastl::optional<VirtualPassRequirements> renderingRequirements;
  BindingsMap bindings;
  ShaderBlockLayersInfo shaderBlockLayers;

  // For debug purposes only. Source location where this node was defined.
  eastl::string nodeSource;
};

using ResourceCreationInfo = eastl::variant<eastl::monostate, ResourceDescription, BlobDescription, ExternalResourceProvider>;

struct ResourceData
{
  ResourceCreationInfo creationInfo = eastl::monostate{};
  eastl::optional<AutoResolutionData> resolution = eastl::nullopt;
  ResourceType type = ResourceType::Invalid;
  History history = History::No;
};

struct AutoResType
{
  IPoint2 staticResolution{};
  IPoint2 dynamicResolution{};
  // This counts down the frames we need to fully change the dynamic resolution.
  // Only non-history physical resources are resized on each frame, so 2 are needed
  // to change everything.
  uint32_t dynamicResolutionCountdown = 0;
};

struct SlotData
{
  ResNameId contents;
};

struct InternalRegistry
{
  // A provider for a handle is acquired through this reference.
  const ResourceProvider &resourceProviderReference;

  IdIndexedMapping<ResNameId, ResourceData> resources;
  IdIndexedMapping<NodeNameId, NodeData> nodes;
  IdIndexedMapping<AutoResTypeNameId, AutoResType> autoResTypes;
  IdIndexedMapping<ResNameId, eastl::optional<SlotData>> resourceSlots;

  // Set of external resources that are considered to be sinks and are
  // therefore never pruned.
  dag::FixedVectorSet<ResNameId, 8> sinkExternalResources;

  // Keeps track of all name space, node, resource and auto resolution
  // type names and their IDs ever known to FG in a hierarchical manner.
  // The name <-> id mapping is persistent.
  IdHierarchicalNameMap<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId> knownNames;
};

} // namespace dabfg

DAG_DECLARE_RELOCATABLE(dabfg::VirtualSubresourceRef);
DAG_DECLARE_RELOCATABLE(dabfg::VirtualPassRequirements);
DAG_DECLARE_RELOCATABLE(dabfg::Binding);
