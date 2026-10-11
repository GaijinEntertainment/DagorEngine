// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/fixed_string.h>
#include <EASTL/variant.h>
#include <EASTL/optional.h>

#include <generic/dag_fixedVectorSet.h>
#include <generic/dag_fixedVectorMap.h>
#include <generic/dag_functionRef.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_enhanced_barrier.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_variableRateShading.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_overrideStates.h>

#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/priority.h>
#include <render/daFrameGraph/usage.h>
#include <render/daFrameGraph/stage.h>
#include <render/daFrameGraph/history.h>

#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>
#include <render/daFrameGraph/detail/blob.h>
#include <render/daFrameGraph/detail/projectors.h>
#include <render/daFrameGraph/detail/access.h>

#include <common/autoResolutionData.h>
#include <common/bindingType.h>
#include <common/untrackedResources.h>

#include <memory/dag_framemem.h>
#include <id/idIndexedMapping.h>
#include <id/idSparseIndexedMapping.h>
#include <id/idIndexedFlags.h>
#include <id/idSentinels.h>


// This files contains an intermediate representation of the graph which
// decouples internal FG algorithms user-facing classes.
// The main aim of these structures is to describe how the user graph
// should look, possibly after applying transformations.

namespace dafg::intermediate
{

// Denotes an index inside GraphIR::nodes
enum NodeIndex : uint16_t
{
};

// Denotes an index inside GraphIR::resources
enum ResourceIndex : uint16_t
{
};

// Opaque index denoting the iteration of multiplexing that this entity
// corresponds to. Should only be used for determining the index to
// provide to frontend, not for actual logic.
enum MultiplexingIndex : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<MultiplexingIndex>>(~0u),
};

struct ResourceUsage
{
  Usage type;
  Access access : 2;
  Stage stage : 6;

  bool operator==(const ResourceUsage &other) const = default;
};
static_assert(sizeof(ResourceUsage) <= 4);

struct Request
{
  ResourceIndex resource;
  ResourceUsage usage;
  // Marks that a node wants to use the last frame version of this resource
  bool fromLastFrame;

  bool operator==(const Request &other) const = default;
};

struct Node
{
  // TODO: this approach won't work with subpasses
  priority_t priority{0};

  // Resources that are used by this node
  dag::RelocatableFixedVector<Request, 16> resourceRequests{};

  // Requests that this node technically makes but doesn't use,
  // so we suppress them from the main requests list to avoid pessimizing
  // further graph optimization.
  dag::RelocatableFixedVector<ResourceIndex, 16> supressedRequests{};

  // Nodes that have to be executed before this one
  dag::FixedVectorSet<NodeIndex, 16> predecessors{};

  eastl::optional<NodeNameId> frontendNode{};

  // Multiplexing iteration this node will be executed on
  MultiplexingIndex multiplexingIndex{Invalid};

  bool hasSideEffects = false;

  bool operator==(const Node &other) const = default;
};

using CtorFunc = dag::FunctionRef<void(void *) const>;
using DtorFunc = dag::FunctionRef<void(void *) const>;
using CopyFunc = dag::FunctionRef<void(void *, const void *) const>;

struct BlobDescription
{
  ResourceSubtypeTag typeTag = ResourceSubtypeTag::Invalid;
  size_t size = 0;
  size_t alignment = 0;
  CtorFunc ctor;
  DtorFunc dtor;
  CopyFunc copy;

  bool operator==(const BlobDescription &) const = default;
};

struct DynamicParameter
{
  ResourceIndex resource;

  ResourceSubtypeTag projectedTag;
  detail::TypeErasedProjector projector;

  bool operator==(const DynamicParameter &) const = default;
};

enum class ClearStage
{
  None,       ///< No clear stage, resource is not cleared
  Activation, ///< Resource is cleared on activation
  RenderPass, ///< Resource is cleared in render pass

  MAX_VAL = RenderPass,
};

using EnhancedBarrier = eastl::variant<eastl::monostate, d3d::BufferBarrier, d3d::TextureBarrier>;

struct GpuDescription : ResourceDescription
{
  using ResourceDescription::ResourceDescription;
  GpuDescription(const ResourceDescription &desc) : ResourceDescription{desc} {}

  bool operator==(const GpuDescription &other) const
  {
    return ResourceDescription::operator==(other) && asBasicRes.activation == other.asBasicRes.activation;
  }
};

struct ScheduledResource
{
  eastl::variant<GpuDescription, BlobDescription> description;
  ResourceType resourceType;
  eastl::optional<AutoResolutionData> resolutionType;
  ClearStage clearStage;
  eastl::variant<ResourceClearValue, DynamicParameter> clearValue;
  ResourceClearFlags clearFlags;
  History history;
  bool autoMipCount;

  bool operator==(const ScheduledResource &) const = default;

  bool isGpuResource() const { return eastl::holds_alternative<GpuDescription>(description); }
  bool isCpuResource() const { return eastl::holds_alternative<BlobDescription>(description); }

  bool isUntracked() const
  {
    if (!isGpuResource())
      return false;
    const uint32_t cFlags = getGpuDescription().asBasicRes.cFlags;
    if (resourceType == ResourceType::Buffer)
      return (cFlags & SBCF_NO_STATE_TRACKING) != 0;
    if (resourceType == ResourceType::Texture)
      return (cFlags & TEXCF_NO_STATE_TRACKING) != 0;
    return false;
  }

  ResourceDescription &getGpuDescription()
  {
    G_ASSERT(isGpuResource());
    return eastl::get<GpuDescription>(description);
  }
  const ResourceDescription &getGpuDescription() const
  {
    G_ASSERT(isGpuResource());
    return eastl::get<GpuDescription>(description);
  }
  const BlobDescription &getCpuDescription() const
  {
    G_ASSERT(isCpuResource());
    return eastl::get<BlobDescription>(description);
  }
};

// TODO: this should be a driver-level thing, containing size and other
// similar stuff, but we don't need it right now, so this is good enough
struct BufferInfo
{
  int flags;

  bool operator==(const BufferInfo &) const = default;
};

struct ExternalResource
{
  eastl::variant<TextureInfo, BufferInfo> info;

  bool operator==(const ExternalResource &other) const
  {
    if (const auto *tex = eastl::get_if<TextureInfo>(&info))
    {
      const auto *otherTex = eastl::get_if<TextureInfo>(&other.info);
      return otherTex && tex->w == otherTex->w && tex->h == otherTex->h && tex->d == otherTex->d && tex->a == otherTex->a &&
             tex->mipLevels == otherTex->mipLevels && tex->type == otherTex->type && tex->isCommitted == otherTex->isCommitted &&
             tex->cflg == otherTex->cflg;
    }
    const auto *buf = eastl::get_if<BufferInfo>(&info);
    const auto *otherBuf = eastl::get_if<BufferInfo>(&other.info);
    return buf && otherBuf && *buf == *otherBuf;
  }
};

// Backbuffers are not available on the main thread and their acquisition
// is deferred to the driver thread, so we handle them separately.
// Currently, this is can only be the backbuffer.
struct DriverDeferredTexture
{
  bool operator==(const DriverDeferredTexture &) const = default;
};

using ConcreteResource = eastl::variant<eastl::monostate, ScheduledResource, ExternalResource, DriverDeferredTexture>;

struct Resource
{
  ConcreteResource resource{};

  // Renaming a resource changes its' ResNameId, but still leaves it
  // the same resource conceptually, so we map all such resources to same
  // IR resource.
  // Note that the ResNameIds are listed according to the renaming chain.
  dag::RelocatableFixedVector<ResNameId, 8> frontendResources{};

  // Multiplexing iteration this resource is produced on
  MultiplexingIndex multiplexingIndex{Invalid};

  bool debugRelated = false;

  bool operator==(const Resource &) const = default;

  bool isExternal() const { return eastl::holds_alternative<ExternalResource>(resource); }
  bool isScheduled() const { return eastl::holds_alternative<ScheduledResource>(resource); }
  bool isDriverDeferredTexture() const { return eastl::holds_alternative<DriverDeferredTexture>(resource); }
  ScheduledResource &asScheduled()
  {
    G_ASSERT(isScheduled());
    return eastl::get<ScheduledResource>(resource);
  }
  const ScheduledResource &asScheduled() const
  {
    G_ASSERT(isScheduled());
    return eastl::get<ScheduledResource>(resource);
  }
  const ExternalResource &asExternal() const
  {
    G_ASSERT(isExternal());
    return eastl::get<ExternalResource>(resource);
  }
  ResourceType getResType() const
  {
    // Paranoid checks in case we modify the structure but forget to update this
    static_assert(eastl::variant_size_v<decltype(resource)> == 4);
    static_assert(eastl::variant_size_v<decltype(asExternal().info)> == 2);
    static_assert(eastl::variant_size_v<decltype(asScheduled().description)> == 2);

    return eastl::visit(
      [](auto &&res) {
        using T = eastl::decay_t<decltype(res)>;
        if constexpr (eastl::is_same_v<T, ScheduledResource>)
          return res.resourceType;
        else if constexpr (eastl::is_same_v<T, ExternalResource>)
          return eastl::holds_alternative<TextureInfo>(res.info) ? ResourceType::Texture : ResourceType::Buffer;
        else if constexpr (eastl::is_same_v<T, DriverDeferredTexture>)
          return ResourceType::Texture;
        else /* if T == eastl::monostate, don't forget to update it for new types! */
          return ResourceType::Invalid;
      },
      eastl::move(resource));
  }
  bool isUntrackedBuffer() const
  {
    if (!untracked_resources_supported())
      return false;
    if (getResType() != ResourceType::Buffer)
      return false;
    if (isScheduled())
      return asScheduled().isUntracked();
    if (isExternal())
      return (eastl::get<BufferInfo>(asExternal().info).flags & SBCF_NO_STATE_TRACKING) != 0;
    return false;
  }
  bool isUntrackedTexture() const
  {
    if (!untracked_resources_supported())
      return false;
    if (getResType() != ResourceType::Texture)
      return false;
    if (isScheduled())
      return asScheduled().isUntracked();
    if (isExternal())
      return (eastl::get<TextureInfo>(asExternal().info).cflg & TEXCF_NO_STATE_TRACKING) != 0;
    return false;
  }
  bool isUntracked() const { return isUntrackedBuffer() || isUntrackedTexture(); }
};

// Default value here is "unbind everything"
struct Binding
{
  BindingType type = BindingType::Invalid;
  eastl::optional<ResourceIndex> resource = eastl::nullopt;
  bool history = false;

  // do we need to reset it to default value after node is executed
  bool reset = false;

  // is this binding created with .optional()
  bool optional = false;

  // projected type of "what"
  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Invalid;
  // projector to get the value
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

struct VertexSource
{
  eastl::optional<ResourceIndex> buffer;
  uint32_t stride;
};

struct IndexSource
{
  eastl::optional<ResourceIndex> buffer;
};

using BindingsMap = dag::FixedVectorMap<int, Binding, 8>;

struct VrsState
{
  uint32_t rateX = 1;
  uint32_t rateY = 1;
  VariableRateShadingCombiner vertexCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
  VariableRateShadingCombiner pixelCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;

  friend bool operator==(const VrsState &first, const VrsState &second) = default;
};

struct SubresourceRef
{
  ResourceIndex resource;
  uint32_t mipLevel;
  uint32_t layer;

  friend bool operator==(const SubresourceRef &first, const SubresourceRef &second) = default;
};

// No subresource ref means we want an empty binding for that attachment
struct RenderPass
{
  dag::RelocatableFixedVector<eastl::optional<SubresourceRef>, 8> colorAttachments;
  eastl::optional<SubresourceRef> depthAttachment;
  bool depthReadOnly = true;
  dag::FixedVectorMap<ResourceIndex, ResourceIndex, 2> resolves;
  bool isLegacyPass = false;
  eastl::optional<SubresourceRef> vrsRateAttachment;

  friend bool operator==(const RenderPass &first, const RenderPass &second) = default;
};

struct ShaderBlockBindings
{
  int objectLayer = -1;
  int frameLayer = -1;
  int sceneLayer = -1;
};

inline constexpr uint32_t MAX_VERTEX_STREAMS = 4;

struct RequiredNodeState
{
  bool asyncPipelines = false;
  bool wire = false;
  VrsState vrs;
  // NOTE: we can be within a render pass even with no attachments bound
  eastl::optional<RenderPass> pass;
  eastl::optional<shaders::OverrideState> shaderOverrides;
  eastl::array<eastl::optional<VertexSource>, MAX_VERTEX_STREAMS> vertexSources;
  eastl::optional<IndexSource> indexSource;
  BindingsMap bindings;
  ShaderBlockBindings shaderBlockLayers;
};

class Mapping;

using DebugNodeName = eastl::fixed_string<char, 64>;
// Resource names are longer due to renaming chains
using DebugResourceName = eastl::fixed_string<char, 128>;

struct Graph
{
  IdSparseIndexedMapping<ResourceIndex, Resource> resources;
  IdSparseIndexedMapping<NodeIndex, Node> nodes;
  IdSparseIndexedMapping<NodeIndex, RequiredNodeState> nodeStates;
  IdSparseIndexedMapping<NodeIndex, DebugNodeName> nodeNames;
  IdSparseIndexedMapping<ResourceIndex, DebugResourceName> resourceNames;

  void clear()
  {
    resources.clear();
    nodes.clear();
    nodeStates.clear();
    nodeNames.clear();
    resourceNames.clear();
  }

  void pruneResources();

  // Calculates the original resources/nodes -> IR resources/nodes mapping
  Mapping calculateMapping() const;

  // Logerrs in debug build if state is inconsistent
  // only useful for finding bugs in IR generation
  void validate() const;
};

IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> apply_node_remap(Graph &graph, const Graph &source_graph,
  const IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> &source_to_desired,
  const IdIndexedMapping<NodeIndex, NodeIndex> &source_to_prev_dst,
  const IdIndexedFlags<NodeIndex, framemem_allocator> &source_nodes_changed,
  IdIndexedFlags<NodeIndex, framemem_allocator> &out_nodes_changed);


// These support structures can be used to determine which IR entity
// a frontend resource or a node got mapped to (and whether they got mapped at all)

inline constexpr ResourceIndex RESOURCE_NOT_MAPPED = MINUS_ONE_SENTINEL_FOR<ResourceIndex>;
inline constexpr NodeIndex NODE_NOT_MAPPED = MINUS_ONE_SENTINEL_FOR<NodeIndex>;

using HistoryPairing = IdIndexedMapping<ResourceIndex, ResourceIndex, framemem_allocator>;

inline bool does_pairing_keep_indices(const HistoryPairing &pairing)
{
  for (auto [idx, oldIdx] : pairing.enumerate())
    if (oldIdx != idx && oldIdx != RESOURCE_NOT_MAPPED)
      return false;
  return true;
}

class Mapping final
{
public:
  Mapping() = default;
  Mapping(uint32_t node_count, uint32_t res_count, uint32_t multiplexing_extent);

  bool wasResMapped(ResNameId id, MultiplexingIndex multiIdx) const;
  bool wasNodeMapped(NodeNameId id, MultiplexingIndex multiIdx) const;

  uint32_t multiplexingExtent() const;
  uint32_t mappedNodeNameIdCount() const { return mappedNodeNameIdCount_; }
  uint32_t mappedResNameIdCount() const { return mappedResNameIdCount_; }
  uint32_t mappedNodeIndexCount() const { return nodeNameIdMapping_.size(); }
  uint32_t mappedResourceIndexCount() const { return resNameIdMapping_.size(); }

  ResourceIndex &mapRes(ResNameId id, MultiplexingIndex multiIdx);
  ResourceIndex mapRes(ResNameId id, MultiplexingIndex multiIdx) const;
  NodeIndex &mapNode(NodeNameId id, MultiplexingIndex multiIdx);
  NodeIndex mapNode(NodeNameId id, MultiplexingIndex multiIdx) const;

private:
  uint32_t mappedNodeNameIdCount_{0};
  uint32_t mappedResNameIdCount_{0};
  uint32_t multiplexingExtent_{0};

  dag::Vector<NodeIndex> nodeNameIdMapping_{};
  dag::Vector<ResourceIndex> resNameIdMapping_{};
};

} // namespace dafg::intermediate

DAG_DECLARE_RELOCATABLE(dafg::intermediate::Binding);
DAG_DECLARE_RELOCATABLE(dafg::intermediate::Request);
DAG_DECLARE_RELOCATABLE(eastl::optional<dafg::intermediate::SubresourceRef>);
