#pragma once

#include <EASTL/span.h>
#include <EASTL/fixed_string.h>
#include <EASTL/variant.h>
#include <EASTL/optional.h>

#include <generic/dag_fixedVectorSet.h>
#include <generic/dag_fixedVectorMap.h>
#include <dag/dag_vector.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_shaderCommon.h>

#include <render/daBfg/externalResources.h>
#include <render/daBfg/multiplexing.h>
#include <render/daBfg/priority.h>
#include <render/daBfg/usage.h>
#include <render/daBfg/stage.h>
#include <render/daBfg/history.h>

#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/detail/resNameId.h>
#include <render/daBfg/detail/blob.h>
#include <render/daBfg/detail/projectors.h>
#include <render/daBfg/detail/access.h>

#include <common/autoResolutionData.h>
#include <common/bindingType.h>

#include <id/idIndexedMapping.h>


// This files contains an intermediate representation of the graph which
// decouples internal FG algorithms user-facing classes.
// The main aim of these structures is to describe how the user graph
// should look, possibly after applying transformations.

namespace dabfg::intermediate
{

// Denotes an index inside GraphIR::nodes
enum NodeIndex : uint32_t
{
};

// Denotes an index inside GraphIR::resources
enum ResourceIndex : uint32_t
{
};

// Opaque index denoting the iteration of multiplexing that this entity
// corresponds to. Should only be used for determining the index to
// provide to frontend, not for actual logic.
enum MultiplexingIndex : uint32_t
{
};

struct ResourceUsage
{
  Access access : 2;
  Usage type : 6;
  Stage stage : 6;
};
static_assert(sizeof(ResourceUsage) == 2);

struct Request
{
  ResourceIndex resource;
  ResourceUsage usage;
  // Marks that a node wants to use the last frame version of this resource
  bool fromLastFrame;
};

struct Node
{
  // TODO: this approach won't work with subpasses
  priority_t priority;

  // Resources that are used by this node
  dag::RelocatableFixedVector<Request, 16> resourceRequests;

  // Nodes that have to be executed before this one
  dag::FixedVectorSet<NodeIndex, 16> predecessors;

  NodeNameId frontendNode;

  // Multiplexing iteration this node will be executed on
  MultiplexingIndex multiplexingIndex;
};

struct ScheduledResource
{
  eastl::variant<ResourceDescription, BlobDescription> description;
  ResourceType resourceType;
  eastl::optional<AutoResolutionData> resolutionType;
  History history;

  bool isGpuResource() const { return eastl::holds_alternative<ResourceDescription>(description); }
  bool isCpuResource() const { return eastl::holds_alternative<BlobDescription>(description); }

  const ResourceDescription &getGpuDescription() const
  {
    G_ASSERT(isGpuResource());
    return eastl::get<ResourceDescription>(description);
  }
  const BlobDescription &getCpuDescription() const
  {
    G_ASSERT(isCpuResource());
    return eastl::get<BlobDescription>(description);
  }
};

struct Resource
{
  eastl::variant<ScheduledResource, ExternalResource> resource;

  // Renaming a resource changes its' ResNameId, but still leaves it
  // the same resource conceptually, so we map all such resources to same
  // IR resource.
  dag::FixedVectorSet<ResNameId, 8> frontendResources;

  // Multiplexing iteration this resource is produced on
  MultiplexingIndex multiplexingIndex;

  bool isExternal() const { return eastl::holds_alternative<ExternalResource>(resource); }
  bool isScheduled() const { return eastl::holds_alternative<ScheduledResource>(resource); }
  bool isExternalTex() const
  {
    return isExternal() && eastl::holds_alternative<ManagedTexView>(eastl::get<ExternalResource>(resource));
  }
  ManagedTexView getExternalTex() const
  {
    G_ASSERT(isExternalTex());
    return eastl::get<ManagedTexView>(eastl::get<ExternalResource>(resource));
  }
  bool isExternalBuf() const
  {
    return isExternal() && eastl::holds_alternative<ManagedBufView>(eastl::get<ExternalResource>(resource));
  }
  ManagedBufView getExternalBuf() const
  {
    G_ASSERT(isExternalBuf());
    return eastl::get<ManagedBufView>(eastl::get<ExternalResource>(resource));
  }
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
  ResourceType getResType() const
  {
    return isScheduled()     ? asScheduled().resourceType
           : isExternalTex() ? ResourceType::Texture
           : isExternalBuf() ? ResourceType::Buffer
                             : ResourceType::Invalid;
  }
};

// Default value here is "unbind everything"
struct Binding
{
  BindingType type = BindingType::Invalid;
  eastl::optional<ResourceIndex> resource = eastl::nullopt;
  bool history = false;

  // projected type of "what"
  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Invalid;
  // projector to get the value
  detail::TypeErasedProjector projector = +[](void *data) { return data; };
};

using BindingsMap = dag::FixedVectorMap<int, Binding, 8>;

struct VrsState
{
  uint32_t rateX;
  uint32_t rateY;
  ResourceIndex rateTexture;
  VariableRateShadingCombiner vertexCombiner;
  VariableRateShadingCombiner pixelCombiner;
};

struct SubresourceRef
{
  ResourceIndex resource;
  uint32_t mipLevel;
  uint32_t layer;
};

// No subresource ref means we want an empty binding for that attachment
struct RenderPass
{
  dag::RelocatableFixedVector<eastl::optional<SubresourceRef>, 8> colorAttachments;
  eastl::optional<SubresourceRef> depthAttachment;
  bool depthReadOnly = true;
};

struct ShaderBlockBindings
{
  int objectLayer = -1;
  int frameLayer = -1;
  int sceneLayer = -1;
};

struct RequiredNodeState
{
  bool wire = false;
  eastl::optional<VrsState> vrs;
  // NOTE: we can be within a render pass even with no attachments bound
  eastl::optional<RenderPass> pass;
  eastl::optional<shaders::OverrideState> shaderOverrides;
  BindingsMap bindings;
  ShaderBlockBindings shaderBlockLayers;
};

class Mapping;

using DebugNodeName = eastl::fixed_string<char, 64>;
// Resource names are longer due to renaming chains
using DebugResourceName = eastl::fixed_string<char, 128>;

struct Graph
{
  IdIndexedMapping<ResourceIndex, Resource> resources;
  IdIndexedMapping<NodeIndex, Node> nodes;
  // DoD suggests these should not be inside Node/Resource
  IdIndexedMapping<NodeIndex, RequiredNodeState> nodeStates;
  IdIndexedMapping<NodeIndex, DebugNodeName> nodeNames;
  IdIndexedMapping<ResourceIndex, DebugResourceName> resourceNames;

  // Choses a subgraph by selecting some nodes and specifying their order
  // Argument must map "old -> new" index and not contain any gaps
  void choseSubgraph(eastl::span<const intermediate::NodeIndex> old_to_new_index_mapping);

  // Calculates the original resources/nodes -> IR resources/nodes mapping
  Mapping calculateMapping();

  // Logerrs in debug build if state is inconsistent
  // only useful for finding bugs in IR generation
  void validate() const;
};


// These support structures can be used to determine which IR entity
// a frontend resource or a node got mapped to (and whether they got mapped at all)

inline constexpr ResourceIndex RESOURCE_NOT_MAPPED{~0u};
inline constexpr NodeIndex NODE_NOT_MAPPED{~0u};

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

  ResourceIndex &mapRes(ResNameId id, MultiplexingIndex multiIdx);
  ResourceIndex mapRes(ResNameId id, MultiplexingIndex multiIdx) const;
  NodeIndex &mapNode(NodeNameId id, MultiplexingIndex multiIdx);
  NodeIndex mapNode(NodeNameId id, MultiplexingIndex multiIdx) const;

private:
  uint32_t mappedNodeNameIdCount_{0};
  uint32_t mappedResNameIdCount_{0};

  dag::Vector<NodeIndex> nodeNameIdMapping_{};
  dag::Vector<ResourceIndex> resNameIdMapping_{};
};

} // namespace dabfg::intermediate

DAG_DECLARE_RELOCATABLE(dabfg::intermediate::Binding);
DAG_DECLARE_RELOCATABLE(dabfg::intermediate::Request);
DAG_DECLARE_RELOCATABLE(eastl::optional<dabfg::intermediate::SubresourceRef>);
