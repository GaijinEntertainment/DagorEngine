// daFrameGraph.das - Pure daslang module providing daFrameGraph API types and helpers
// This module provides type definitions and documentation for the daFrameGraph API
// without requiring the C++ runtime. For actual execution, run within Dagor Engine.

options gen2
options persistent_heap
options gc
options multiple_contexts
options rtti

module dafg;

// ============================================================
// Core Enums (matching daFrameGraph C++ API)
// ============================================================

enum Access : int {
    Read = 0
    Write = 1
    ReadWrite = 2
    ShaderResource = 3
    UnorderedAccess = 4
}

enum Usage : int {
    None = 0
    Color = 1
    Depth = 2
    ShaderResource = 3
    UnorderedAccess = 4
    RenderTarget = 5
    DepthStencil = 6
    CopySrc = 7
    CopyDst = 8
    ResolveSrc = 9
    ResolveDst = 10
    Present = 11
    ShadingRate = 12
    VRS = 13
}

enum Stage : int {
    None = 0
    Vertex = 1
    Hull = 2
    Domain = 3
    Geometry = 4
    Pixel = 5
    Compute = 6
    RayTracing = 7
    AllGraphics = 8
    All = 9
}

enum ResourceType : int {
    Unknown = 0
    Texture1D = 1
    Texture2D = 2
    Texture3D = 3
    TextureCube = 4
    Texture1DArray = 5
    Texture2DArray = 6
    TextureCubeArray = 7
    Buffer = 8
    Blob = 9
}

enum BindingType : int {
    Invalid = 0
    SRV = 1
    UAV = 2
    RTV = 3
    DSV = 4
    Sampler = 5
    CBV = 6
}

enum SideEffects : int {
    None = 0
    ModifiesState = 1
    ModifiesResources = 2
    ModifiesGlobals = 4
    All = 7
}

enum History : int {
    None = 0
    Single = 1
    Double = 2
    Triple = 3
}

enum MultiplexingMode : int {
    Disabled = 0
    Enabled = 1
    Explicit = 2
}

enum AutoResTypeNameId : int {
    BackBuffer = 0
    DepthBuffer = 1
    MotionVectors = 2
}

enum RefinedBlockNameId : int {
    Global = 0
    PerObject = 1
    PerMaterial = 2
    PerFrame = 3
}

enum NodeNameId : int {
    Invalid = 0xFFFFFFFF
}

enum ResNameId : int {
    Invalid = 0xFFFFFFFF
}

enum NameSpaceNameId : int {
    Root = 0
}

// ============================================================
// Core Structs
// ============================================================

struct IPoint2 {
    x : int
    y : int
}

struct IPoint3 {
    x : int
    y : int
    z : int
}

struct ResourceRequest {
    resNameId : ResNameId
    access : Access
    usage : Usage
    stage : Stage
    history : History
    multiplexing : MultiplexingMode
}

struct ResourceData {
    nameId : ResNameId
    resType : ResourceType
    requests : array<ResourceRequest>
}

struct CreatedResourceData {
    resourceData : ResourceData
    handle : uint64
}

struct VirtualPassRequirements {
    nameId : NodeNameId
    colorAttachments : array<ResNameId>
    depthAttachment : ResNameId
    sideEffects : SideEffects
}

struct NodeData {
    nameId : NodeNameId
    requirements : VirtualPassRequirements
    source : string
}

struct InternalRegistry {
    ptr : uint64
}

struct ResourceProvider {
    ptr : uint64
}

struct ResourceUsage {
    resNameId : ResNameId
    access : Access
    stage : Stage
}

struct AutoResolutionData {
    autoResType : AutoResTypeNameId
    multiplier : float
    overrideX : int
    overrideY : int
    hasOverride : bool
}

struct StateRequest {
    registry : InternalRegistry
    nodeId : NodeNameId
    hasRegistry : bool
}

struct ShaderBlockLayersInfo {
    layers : array<string>
}

struct VrsStateRequirements {
    rateX : uint
    rateY : uint
}

struct VirtualSubresourceRef {
    resource : ResNameId
    mipLevel : uint
    arraySlice : uint
}

struct Binding {
    bindingType : BindingType
    resource : ResNameId
    subresource : VirtualSubresourceRef
    hasSubresource : bool
}

// ============================================================
// Handle types (opaque)
// ============================================================

struct NodeHandle {
    id : uint64
}

struct TextureView {
    handle : uint64
}

struct BufferView {
    handle : uint64
}

struct BlobView {
    handle : uint64
}

// ============================================================
// Registry API (documentation helpers)
// ============================================================

def get_registry() : InternalRegistry {
    var r : InternalRegistry
    r.ptr = 0
    return r
}

def register_node(registry: InternalRegistry, name: string, source: string) : NodeHandle {
    var h : NodeHandle
    h.id = 0
    return h
}

def create_texture(registry: InternalRegistry, name: string, width: int, height: int, format: int) : CreatedResourceData {
    var d : CreatedResourceData
    d.handle = 0
    return <- d
}

def create_buffer(registry: InternalRegistry, name: string, size: int, flags: int) : CreatedResourceData {
    var d : CreatedResourceData
    d.handle = 0
    return <- d
}

def set_resolution(registry: InternalRegistry, resName: string, width: int, height: int) : bool => false

def set_dynamic_resolution(registry: InternalRegistry, resName: string, multiplier: float) : bool => false

def fill_slot(ns: NameSpaceNameId, slot: string, resNs: NameSpaceNameId, resName: string) : bool => false

def update_externally_consumed(registry: InternalRegistry, resName: string) : bool => false

def run_frame_graph(registry: InternalRegistry) : bool => false

def invalidate_history(registry: InternalRegistry) : bool => false

def set_multiplexing(registry: InternalRegistry, mode: MultiplexingMode) : bool => false

def set_multiplexing_extents(registry: InternalRegistry, width: int, height: int) : bool => false

def get_resource_info(registry: InternalRegistry, resName: string) : ResourceData {
    var d : ResourceData
    return <- d
}

def dump_frame_graph(registry: InternalRegistry) : string => "Frame graph dump not available (requires running engine)"

def visualize(registry: InternalRegistry, outputPath: string) : bool => false

def mark_external_validation(registry: InternalRegistry, resName: string) : bool => false

def reset_shadervar() { }

// ============================================================
// Namespace helpers
// ============================================================

struct NameSpace {
    nameSpaceId : NameSpaceNameId
}

def operator/(self : NameSpace; name: string) : NameSpace {
    var ns : NameSpace
    ns.nameSpaceId = NameSpaceNameId.Root
    return ns
}

def register_node(self : NameSpace; name: string, source: string) : NodeHandle {
    var h : NodeHandle
    h.id = 0
    return h
}

def root_namespace() : NameSpace {
    var ns : NameSpace
    ns.nameSpaceId = NameSpaceNameId.Root
    return ns
}

// ============================================================
// Documentation helpers
// ============================================================

def get_api_version() : string => "daFrameGraph API v1.0 (daslang MCP wrapper)"

def get_node_types() : array<string> => [
    "RenderPass",
    "ComputePass", 
    "CopyPass",
    "ResolvePass",
    "PresentPass",
    "RayTracingPass"
]

def get_resource_types() : array<string> => [
    "Texture2D",
    "Texture3D",
    "TextureCube",
    "Buffer",
    "Blob"
]

def get_access_modes() : array<string> => [
    "Read",
    "Write", 
    "ReadWrite",
    "ShaderResource",
    "UnorderedAccess"
]

def get_usage_types() : array<string> => [
    "Color",
    "Depth",
    "ShaderResource",
    "UnorderedAccess",
    "RenderTarget",
    "DepthStencil"
]