// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_fixedVectorMap.h>
#include <shaders/dag_overrideStates.h>

#include <render/daFrameGraph/detail/daFG.h>
#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/detail/shaderNameId.h>

#include <common/autoResolutionData.h>
#include <common/bindingType.h>
#include <frontend/resourceProvider.h>
#include <id/idIndexedMapping.h>
#include <id/idHierarchicalNameMap.h>


namespace dafg
{

struct ResourceUsage
{
  Usage type = Usage::UNKNOWN;
  Access access = Access::UNKNOWN;
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

struct DynamicParameter
{
  ResNameId resource = ResNameId::Invalid;

  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Invalid;
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

struct VirtualPassRequirements
{
  // Also known as "multiple render targets" -- correspond to pixel
  // shader outputs in the specified order.
  dag::RelocatableFixedVector<VirtualSubresourceRef, 8> colorAttachments;
  VirtualSubresourceRef depthAttachment;
  bool depthReadOnly = true;
  // Mapping of a color/depth attachment to a resource that they should
  // be (MSAA) resolved to
  dag::FixedVectorMap<ResNameId, ResNameId, 2> resolves;
  VirtualSubresourceRef vrsRateAttachment;
};

struct ShaderBlockLayersInfo
{
  int objectLayer = -1;
  int frameLayer = -1;
  int sceneLayer = -1;
};

struct VrsStateRequirements
{
  uint32_t rateX = 1;
  uint32_t rateY = 1;
  VariableRateShadingCombiner vertexCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
  VariableRateShadingCombiner pixelCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
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
};

struct BlobArg
{
  ResNameId res = ResNameId::Invalid;
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

struct DispatchAutoResolutionArg
{
  AutoResTypeNameId autoResTypeId = AutoResTypeNameId::Invalid;
  float multiplier = 1.0f;
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

struct DispatchTextureResolutionArg
{
  ResNameId res = ResNameId::Invalid;
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

struct DispatchRequirements
{
  struct DirectArgs
  {
    eastl::variant<uint32_t, BlobArg, DispatchAutoResolutionArg, DispatchTextureResolutionArg> x = 1;
    eastl::variant<uint32_t, BlobArg, DispatchAutoResolutionArg, DispatchTextureResolutionArg> y = 1;
    eastl::variant<uint32_t, BlobArg, DispatchAutoResolutionArg, DispatchTextureResolutionArg> z = 1;
  };
  struct IndirectArgs
  {
    ResNameId res = ResNameId::Invalid;
    eastl::variant<uint32_t, BlobArg> offset = 0;
  };
  struct MeshIndirectArgs : IndirectArgs
  {
    eastl::variant<eastl::monostate, uint32_t, BlobArg> stride;
    eastl::variant<eastl::monostate, uint32_t, BlobArg> count;
  };
  struct MeshIndirectCountArgs : MeshIndirectArgs
  {
    ResNameId countRes = ResNameId::Invalid;
    eastl::variant<uint32_t, BlobArg> countOffset = 0;
    eastl::variant<eastl::monostate, uint32_t, BlobArg> maxCount;
  };
  struct Threads : DirectArgs
  {};
  struct MeshThreads : Threads
  {};
  struct Groups : DirectArgs
  {};
  struct MeshGroups : Groups
  {};

  ShaderNameId shaderId = ShaderNameId::Invalid;
  eastl::variant<Threads, MeshThreads, Groups, MeshGroups, IndirectArgs, MeshIndirectArgs, MeshIndirectCountArgs> arguments;
};

struct DrawRequirements
{
  struct DrawArgs
  {
    DrawPrimitive primitive = DrawPrimitive::TRIANGLE_LIST;
  };
  struct DirectArgs : DrawArgs
  {
    eastl::variant<eastl::monostate, uint32_t, BlobArg> primitiveCount;
    eastl::variant<uint32_t, BlobArg> instanceCount = 1;
    eastl::variant<uint32_t, BlobArg> startInstance = 0;
  };
  struct DirectNonIndexedArgs : DirectArgs
  {
    eastl::variant<uint32_t, BlobArg> startVertex = 0;
  };
  struct DirectIndexedArgs : DirectArgs
  {
    eastl::variant<uint32_t, BlobArg> startIndex = 0;
    eastl::variant<uint32_t, BlobArg> baseVertex = 0;
  };
  struct IndirectArgs : DrawArgs
  {
    ResNameId buffer = ResNameId::Invalid;
    eastl::variant<uint32_t, BlobArg> offset = 0;
  };
  struct IndirectNonIndexedArgs : IndirectArgs
  {};
  struct IndirectIndexedArgs : IndirectArgs
  {};

  struct MultiIndirectArgs : IndirectArgs
  {
    eastl::variant<eastl::monostate, uint32_t, BlobArg> stride;
    eastl::variant<eastl::monostate, uint32_t, BlobArg> drawCount;
  };
  struct MultiIndirectNonIndexedArgs : MultiIndirectArgs
  {};
  struct MultiIndirectIndexedArgs : MultiIndirectArgs
  {};

  ShaderNameId shaderId = ShaderNameId::Invalid;
  eastl::variant<DirectNonIndexedArgs, DirectIndexedArgs, IndirectNonIndexedArgs, IndirectIndexedArgs, MultiIndirectNonIndexedArgs,
    MultiIndirectIndexedArgs>
    arguments;
};

struct Binding
{
  BindingType type = BindingType::Invalid;
  ResNameId resource = ResNameId::Invalid;
  bool history = false;

  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Invalid;
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

struct VertexSource
{
  ResNameId buffer = ResNameId::Invalid;
  uint32_t stride = 0;
};

struct IndexSource
{
  ResNameId buffer = ResNameId::Invalid;
};

using BindingsMap = dag::FixedVectorMap<int, Binding, 8>;

inline constexpr uint32_t MAX_VERTEX_STREAMS = 4;

struct NodeData
{
  priority_t priority = PRIO_DEFAULT;
  eastl::optional<multiplexing::Mode> multiplexingMode;
  SideEffects sideEffect = SideEffects::Internal;
  // For debug purposes only
  bool enabled = true;
  bool allowAsyncPipelines = false;

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
  eastl::variant<eastl::monostate, DispatchRequirements, DrawRequirements> executeRequirements;
  eastl::array<eastl::optional<VertexSource>, MAX_VERTEX_STREAMS> vertexSources;
  eastl::optional<IndexSource> indexSource;
  BindingsMap bindings;
  ShaderBlockLayersInfo shaderBlockLayers;

  // For debug purposes only. Source location where this node was defined.
  eastl::string nodeSource;
};

struct DriverDeferredTexture
{};

using ResourceCreationInfo = eastl::variant<eastl::monostate, Texture2dCreateInfo, Texture3dCreateInfo, BufferCreateInfo,
  BlobDescription, ExternalResourceProvider, DriverDeferredTexture>;

struct ResourceData
{
  ResourceCreationInfo creationInfo = eastl::monostate{};
  // TODO: we can and should remove this and simply use createInfo
  eastl::optional<AutoResolutionData> resolution = eastl::nullopt;
  eastl::variant<eastl::monostate, ResourceClearValue, DynamicParameter> clearValue = eastl::monostate{};
  ResourceType type = ResourceType::Invalid;
  History history = History::No;
};

template <class T>
struct ResolutionValues
{
  T staticResolution{};
  T dynamicResolution{};
  T lastAppliedDynamicResolution{};
};

struct AutoResTypeData
{
  // Initially is monostate, then switches to 3d or 2d ONCE and is not
  // allowed to be changed again (use different names instead).
  eastl::variant<eastl::monostate, ResolutionValues<IPoint2>, ResolutionValues<IPoint3>> values;
  // This counts down the frames we need to fully change the dynamic resolution.
  // Only non-history physical resources are resized on each frame, so 2 are needed
  // to change everything.
  uint32_t dynamicResolutionCountdown = 0;
};

struct SlotData
{
  ResNameId contents;
  mutable ResNameId prevContents;
};

struct InternalRegistry
{
  // A provider for a handle is acquired through this reference.
  const ResourceProvider &resourceProviderReference;

  IdIndexedMapping<ResNameId, ResourceData> resources{};
  IdIndexedMapping<NodeNameId, NodeData> nodes{};
  IdIndexedMapping<AutoResTypeNameId, AutoResTypeData> autoResTypes{};
  IdIndexedMapping<ResNameId, eastl::optional<SlotData>> resourceSlots{};

  // Set of external resources that are considered to be sinks and are
  // therefore never pruned.
  dag::FixedVectorSet<ResNameId, 8> sinkExternalResources{};

  // Keeps track of all name space, node, resource and auto resolution
  // type names and their IDs ever known to FG in a hierarchical manner.
  // The name <-> id mapping is persistent.
  IdHierarchicalNameMap<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId> knownNames{};
  IdNameMap<ShaderNameId> knownShaders{};

  multiplexing::Mode defaultMultiplexingMode = multiplexing::Mode::FullMultiplex;
  multiplexing::Mode defaultHistoryMultiplexingMode = multiplexing::Mode::FullMultiplex;
};

} // namespace dafg

DAG_DECLARE_RELOCATABLE(dafg::VirtualSubresourceRef);
DAG_DECLARE_RELOCATABLE(dafg::VirtualPassRequirements);
DAG_DECLARE_RELOCATABLE(dafg::Binding);
