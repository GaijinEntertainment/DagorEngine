options indenting = 2
require daBfg


struct ResUid
  nameId : ResNameId
  history : bool


struct VirtualResourceRequestBase
  registry : InternalRegistry?
  resUid : ResUid
  nodeId : NodeNameId


struct VirtualResourceHandle
  registry : InternalRegistry?
  resUid : ResUid

struct VirtualResourceCreationSemiRequest : VirtualResourceRequestBase {}
struct VirtualResourceSemiRequest : VirtualResourceRequestBase {}
struct VirtualResourceRequest : VirtualResourceRequestBase {}

struct VirtualTextureHandle : VirtualResourceHandle {}
struct VirtualBufferHandle : VirtualResourceHandle {}

struct VirtualTextureRequest : VirtualResourceRequest {}
struct VirtualBufferRequest : VirtualResourceRequest {}


struct StateRequest
  registry : InternalRegistry?
  nodeId : NodeNameId

struct VrsRequirements
  rateX : uint
  rateY : uint
  rateTextureResName : string
  vertexCombiner : VariableRateShadingCombiner
  pixelCombiner : VariableRateShadingCombiner

struct VirtualPassRequest
  registry : InternalRegistry?
  nodeId : NodeNameId


variant VirtualAttachmentResource
  resUid : ResUid
  name : string


struct VirtualAttachmentRequest
  resource : VirtualAttachmentResource
  mipLevel : uint
  layer : uint


struct AutoResolution
  name : string
  multiplier : float = 1.0f


variant TextureResolution
  res : tuple<x : uint; y : uint>
  autoRes : AutoResolution


struct Texture2dCreateInfo
  resolution : TextureResolution
  creationFlags : uint
  mipLevels : uint


struct BufferCreateInfo
  elementSize : uint
  elementCount : uint
  flags : uint
  format : uint


struct NamedSlot
  name : string


[generic]
def registerNode(name : string; var declaration_callback : lambda<(var reg : Registry) : lambda<void>>) : NodeHandle
  var registerNodeName = $ <| (var registry : InternalRegistry &)
    let nodeId = registry.knownNodeNames |> addNameId(name)
    var registerNodeId = $ <| (nodeId_ : NodeNameId; var node_data : NodeData &)
      getTracker() |> unregisterNode(nodeId_, node_data.generation)
      getTracker() |> registerNode(nodeId_)
      node_data |> registerNodeDeclaration(declaration_callback)
      return <- register_external_node(nodeId_, node_data.generation)

    return <- invoke(registerNodeId, nodeId, registry.nodes |> get(nodeId))

  return <- invoke(registerNodeName, getTracker() |> getRegistry())


[generic]
def orderMeBefore(var self : Registry; name : string) : Registry
  let nameId = self.registry.knownNodeNames |> getNameId(name)
  if nameId != NodeNameId Invalid
    self.registry.nodes[self.nodeId].followingNodeIds |> insert(nameId)
  else
    let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
    error("Node {selfName} tries to order before not existing node {name}, skipping this order")

  return self


[generic]
def orderMeBefore(var self : Registry; names : array<string>) : Registry
  for name in names
    self |> orderMeBefore(name)

  return self


[generic]
def orderMeAfter(var self : Registry; name : string) : Registry
  let nameId = self.registry.knownNodeNames |> getNameId(name)
  if nameId != NodeNameId Invalid
    self.registry.nodes[self.nodeId].precedingNodeIds |> insert(nameId)
  else
    let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
    error("Node {selfName} tries to order after not existing node {name}, skipping this order")

  return self


[generic]
def orderMeAfter(var self : Registry; names : array<string>) : Registry
  for name in names
    self |> orderMeAfter(name)

  return self


[generic]
def setPriority(var self : Registry; priority : int) : Registry
  self.registry.nodes[self.nodeId].priority = priority
  return self


[generic]
def multiplex(var self : Registry; multiplexing_mode : MultiplexingMode) : Registry
  self.registry.nodes[self.nodeId].multiplexingMode = multiplexing_mode
  return self


[generic]
def executionHas(var self : Registry; side_effect : SideEffect) : Registry
  self.registry.nodes[self.nodeId].sideEffect = side_effect
  return self

[generic]
def create(var self : Registry; name : string; history : History) : VirtualResourceCreationSemiRequest
  let nameId = self.registry.knownResourceNames |> addNameId(name)
  var setHistory = $ <| (var res : ResourceData &)
    res.history = history;

  invoke(setHistory, self.registry.resources |> get(nameId))
  self.registry.nodes[self.nodeId].createdResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = [[ResourceUsage access=Access READ_WRITE]]
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- [[VirtualResourceCreationSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=nameId, history=false]]]]


[generic]
def read(var self : Registry; name : string) : VirtualResourceSemiRequest
  let nameId = self.registry.knownResourceNames |> addNameId(name)
  self.registry.nodes[self.nodeId].readResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = [[ResourceUsage access=Access READ_ONLY]]
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- [[VirtualResourceSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=nameId, history=false]]]]


[generic]
def read(var self : Registry; slot : NamedSlot) : VirtualResourceSemiRequest
  let nameId = self.registry.knownResourceNames |> addNameId(slot.name)
  self.registry.nodes[self.nodeId].readResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.slotRequest = true
    req.usage = [[ResourceUsage access=Access READ_ONLY]]
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- [[VirtualResourceSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=nameId, history=false]]]]


[generic]
def history(var self : Registry; var name : string) : VirtualResourceSemiRequest
  let nameId = self.registry.knownResourceNames |> addNameId(name);
  using() <| $(var req : ResourceRequest#)
    req.usage = [[ResourceUsage access=Access READ_ONLY]]
    self.registry.nodes[self.nodeId].historyResourceReadRequests[nameId] := req

  return <- [[VirtualResourceSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=nameId, history=false]]]]


[generic]
def modify(var self : Registry; name : string) : VirtualResourceSemiRequest
  let nameId = self.registry.knownResourceNames |> addNameId(name)
  self.registry.nodes[self.nodeId].modifiedResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = [[ResourceUsage access=Access READ_WRITE]]
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- [[VirtualResourceSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=nameId, history=false]]]]


[generic]
def modify(var self : Registry; slot : NamedSlot) : VirtualResourceSemiRequest
  let nameId = self.registry.knownResourceNames |> addNameId(slot.name)
  self.registry.nodes[self.nodeId].modifiedResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.slotRequest = true
    req.usage = [[ResourceUsage access=Access READ_WRITE]]
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- [[VirtualResourceSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=nameId, history=false]]]]


[generic]
def rename(var self : Registry; from : string; to : string; history : History) : VirtualResourceSemiRequest
  let fromResId = self.registry.knownResourceNames |> addNameId(from)
  let toResId = self.registry.knownResourceNames |> addNameId(to)

  self.registry.nodes[self.nodeId].renamedResources[toResId] := fromResId
  using() <| $(var req : ResourceRequest#)
    req.usage = [[ResourceUsage access=Access READ_WRITE]]
    self.registry.nodes[self.nodeId].resourceRequests[fromResId] := req
  var setHistory = $ <| (var res : ResourceData &)
    res.history = history

  invoke(setHistory, self.registry.resources |> get(toResId))

  return <- [[VirtualResourceSemiRequest registry=self.registry, nodeId=self.nodeId, resUid=[[ResUid nameId=fromResId, history=false]]]]


[generic]
def texture(var self : VirtualResourceCreationSemiRequest; info : Texture2dCreateInfo) : VirtualTextureRequest
  var setTexture = $ <| (var self_ : VirtualResourceCreationSemiRequest; info_ : Texture2dCreateInfo; var res : ResourceData &)
    using() <| $(var desc : TextureResourceDescription#)
      desc.mipLevels = info_.mipLevels
      desc.cFlags = info_.creationFlags
      if info_.resolution is autoRes
        res |> setTextureInfo(desc)
        res |> setResolution([[AutoResolutionData id=self_.registry.knownAutoResolutionTypeNames |> getNameId(info_.resolution as autoRes.name), multiplier=info_.resolution as autoRes.multiplier]])
      else
        desc.width = info_.resolution as res.x
        desc.height = info_.resolution as res.y
        res |> setTextureInfo(desc)

    res.resType = ResourceType Texture

  invoke(setTexture, self, info, self.registry.resources |> get(self.resUid.nameId))
  return <- [[VirtualTextureRequest registry=self.registry, nodeId=self.nodeId, resUid=self.resUid]]


[generic]
def texture(var self : VirtualResourceSemiRequest) : VirtualTextureRequest
  return <- [[VirtualTextureRequest registry=self.registry, nodeId=self.nodeId, resUid=self.resUid]]


[generic]
def buffer(var self : VirtualResourceCreationSemiRequest; info : BufferCreateInfo) : VirtualBufferRequest
  var setBuffer = $ <| (info_ : BufferCreateInfo; var res : ResourceData &)
    using() <| $(var desc : BufferResourceDescription#)
      desc.elementCount = info_.elementCount
      desc.elementSizeInBytes = info_.elementSize
      desc.viewFormat = info_.format
      desc.cFlags = info_.flags
      desc.activation = ResourceActivationAction DISCARD_AS_UAV
      res |> setDescription(desc)

    res.resType = ResourceType Buffer

  invoke(setBuffer, info, self.registry.resources |> get(self.resUid.nameId))
  return <- [[VirtualBufferRequest registry=self.registry, nodeId=self.nodeId, resUid=self.resUid]]


[generic]
def buffer(var self : VirtualResourceSemiRequest) : VirtualBufferRequest
  return <- [[VirtualBufferRequest registry=self.registry, nodeId=self.nodeId, resUid=self.resUid]]


[generic]
def blob(var self : VirtualResourceSemiRequest) : VirtualResourceRequest
  assert(false, "Blobs are not implemented in daScript API")
  var setBlob = $ <| (var res : ResourceData &)
    res.resType = ResourceType Blob

  invoke(setBlob, self.registry.resources |> get(self.resUid.nameId))
  return <- [[VirtualResourceRequest registry=self.registry, nodeId=self.nodeId, resUid=self.resUid]]


[generic]
def thisRequest(var self : VirtualResourceRequest) : ResourceRequest
  return (self.resUid.history ?
    self.registry.nodes[self.nodeId].historyResourceReadRequests[self.resUid.nameId] :
    self.registry.nodes[self.nodeId].resourceRequests[self.resUid.nameId])


[generic]
def useAs(var self : auto(TT); usageType : Usage) : TT
  thisRequest(self).usage.usageType = usageType
  return self


[generic]
def atStage(var self : auto(TT); stage : Stage) : TT
  thisRequest(self).usage.stage = stage
  return self


[generic]
def bindToShaderVar(var self : auto(TT); name : string = "") : TT
  var svName = name
  if (name == "")
    svName = self.registry.knownResourceNames |> getName(self.resUid.nameId)

  let svId = get_shader_variable_id(svName)
  let setBinding = $ <| (var self_ : TT; sv_id : int; name_ : string; var bindings : BindingsMap &)
    if !(bindings |> key_exists(sv_id))
      using() <| $(var info : Binding#)
        info.bindType = BindingType ShaderVar
        info.resource = self_.resUid.nameId
        info.history = self_.resUid.history
        bindings[sv_id] := info
    else
      let selfName = self_.registry.knownNodeNames |> getName(self_.nodeId)
      error("Encountered duplicate shader var {name_} binding in {selfName} frame graph node! Ignoring one of the them!")

  invoke(setBinding, self, svId, name) <| self.registry.nodes[self.nodeId].bindings
  thisRequest(self).usage.usageType = Usage SHADER_RESOURCE
  return self


[generic]
def handle(self : VirtualTextureRequest) : VirtualTextureHandle
  return <- [[VirtualTextureHandle registry=self.registry, resUid=self.resUid]]


[generic]
def handle(self : VirtualBufferRequest) : VirtualBufferHandle
  return <- [[VirtualBufferHandle registry=self.registry, resUid=self.resUid]]


[generic]
def view(handle : VirtualTextureHandle) : ManagedTexView
  return handle.registry |> getResourceProvider |> getTexView(handle.resUid.nameId, handle.resUid.history)


[generic]
def view(handle : VirtualBufferHandle) : ManagedBufView
  return handle.registry |> getResourceProvider |> getBufView(handle.resUid.nameId, handle.resUid.history)


[generic]
def requestState(var self : Registry) : StateRequest
  if self.registry.nodes[self.nodeId].stateRequirements is some
    let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
    error("Global state requested twice on {selfName} frame graph node! Ignoring one of the requests!")

  self.registry.nodes[self.nodeId].stateRequirements |> emplace_some

  return [[StateRequest registry=self.registry, nodeId=self.nodeId]]


[generic]
def private setBlock(var self : StateRequest; name : string; layer : string; var value : int&) : StateRequest
  if value != -1
    let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
    error("Block requested to be set to layer {layer} twice within {selfName} frame graph node! Ignoring one of the requests!")

  value = getBlockId(name)
  return self


[generic]
def setFrameBlock(var self : StateRequest; name : string) : StateRequest
  return self |> setBlock(name, "'FRAME'", self.registry.nodes[self.nodeId].shaderBlockLayers.frameLayer)


[generic]
def setSceneBlock(var self : StateRequest; name : string) : StateRequest
  return self |> setBlock(name, "'SCENE'", self.registry.nodes[self.nodeId].shaderBlockLayers.sceneLayer)


[generic]
def setObjectBlock(var self : StateRequest; name : string) : StateRequest
  return self |> setBlock(name, "'OBJECT'", self.registry.nodes[self.nodeId].shaderBlockLayers.objectLayer)


[generic]
def allowWireFrame(var self : StateRequest) : StateRequest
  assume maybeStateReqs = self.registry.nodes[self.nodeId].stateRequirements
  verify(maybeStateReqs is some)
  (maybeStateReqs as some).supportsWireframe = true
  return self


[generic]
def allowVrs(var self : StateRequest; vrs : VrsRequirements) : StateRequest
  assume maybeStateReqs = self.registry.nodes[self.nodeId].stateRequirements
  verify(maybeStateReqs is some)

  let rateTexId = self.registry.knownResourceNames |> addNameId(vrs.rateTextureResName)
  self.registry.nodes[self.nodeId].readResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = [[ResourceUsage access=Access READ_ONLY, usage=Usage VRS_RATE_TEXTURE, stage=ALL_GRAPHICS]]
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  let setVrsState = $ <| (var vrsState : VrsStateRequirements)
    vrsState.rateX = vrs.rateX
    vrsState.rateY = vrs.rateY
    vrsState.rateTextureResId = rateTexId
    vrsState.vertexCombiner = vrs.vertexCombiner
    vrsState.pixelCombiner = vrs.pixelCombiner

  let passthrough = $ <| (var stateReqs : NodeStateRequirements)
    stateReqs.vrsState |> emplace_some
    setVrsState |> invoke(stateReqs.vrsState as some)

  passthrough |> invoke(maybeStateReqs as some)
  return self


[generic]
def enableOverride(var self : StateRequest; das_override : OverrideRenderState)
  assume maybeStateReqs = self.registry.nodes[self.nodeId].stateRequirements
  verify(maybeStateReqs is some)

  let setVrsState = $ <| (var final_override : OverrideState)
    final_override.bits = uint(das_override.bits)
    final_override.zFunc = uint8(das_override.zFunc)
    final_override.forcedSampleCount = das_override.forcedSampleCount
    final_override.blendOp = uint8(das_override.blendOp)
    final_override.blendOpA = uint8(das_override.blendOpA)
    final_override.sblend = uint8(das_override.sblend)
    final_override.dblend = uint8(das_override.dblend)
    final_override.sblenda = uint8(das_override.sblenda)
    final_override.dblenda = uint8(das_override.dblenda)
    final_override.colorWr = uint(das_override.colorWr)

  let passthrough = $ <| (var stateReqs : NodeStateRequirements)
    stateReqs.pipelineStateOverride |> emplace_some
    setVrsState |> invoke(stateReqs.pipelineStateOverride as some)

  passthrough |> invoke(maybeStateReqs as some)
  return self


[generic]
def requestRenderPass(var self : Registry) : VirtualPassRequest
  if self.registry.nodes[self.nodeId].renderingRequirements is some
    let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
    error("Encountered two renderpass requests in {selfName} framegraph node! Ignoring one of the requests!")

  self.registry.nodes[self.nodeId].renderingRequirements |> emplace_some

  return [[VirtualPassRequest registry=self.registry, nodeId=self.nodeId]]


[generic]
def private processAttachment(var self : VirtualPassRequest; attachment : VirtualAttachmentRequest; access_override : Access) : ResUid
  var uid : ResUid
  if attachment.resource is resUid
    uid = attachment.resource as resUid
  else
    let resId = self.registry.knownResourceNames |> addNameId(attachment.resource as name)
    if (access_override == Access READ_ONLY)
      self.registry.nodes[self.nodeId].readResources |> insert(resId)
    else
      self.registry.nodes[self.nodeId].modifiedResources |> insert(resId)

    uid.nameId = resId
    uid.history = false

  let setReq = $ <| (access_override_ : Access; var req : ResourceRequest &)
    req.usage.access = access_override_
    req.usage.stage = Stage POST_RASTER
    req.usage.usageType = Usage COLOR_ATTACHMENT

  invoke(setReq, access_override, self.registry.nodes[self.nodeId].resourceRequests[uid.nameId])
  return uid


[generic]
def color(var self : VirtualPassRequest; requests : VirtualTextureRequest[]) : VirtualPassRequest
  let attachments <- [{for req in requests; [[VirtualAttachmentRequest resource=[[VirtualAttachmentResource resUid=req.resUid]]]]}]
  return self |> color(attachments)


[generic]
def color(var self : VirtualPassRequest; names : string[]) : VirtualPassRequest
  let attachments <- [{for name in names; [[VirtualAttachmentRequest resource=[[VirtualAttachmentResource name=name]]]]}]
  return self |> color(attachments)


[generic]
def color(var self : VirtualPassRequest; attachments : array<VirtualAttachmentRequest>) : VirtualPassRequest
  verify(self.registry.nodes[self.nodeId].renderingRequirements is some)

  let setReqs = $ <| (var reqs : VirtualPassRequirements)
    if reqs.colorAttachments |> length != 0
      let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
      error("Encountered duplicate color attachment calls on the same pass request in {selfName} frame graph node! Ignoring one of them!")
      reqs.colorAttachments |> clear()

    for attachment in attachments
      let uid = self |> processAttachment(attachment, Access READ_WRITE)
      verify(!uid.history)
      reqs.colorAttachments |> push([[VirtualSubresourceRef nameId=uid.nameId, mipLevel=attachment.mipLevel, layer=attachment.layer]])

  setReqs |> invoke(self.registry.nodes[self.nodeId].renderingRequirements as some)

  return self


[generic]
def private depth(var self : VirtualPassRequest; name : string; usage_access : Access; read_only : bool) : VirtualPassRequest
  let attachment <- [[VirtualAttachmentRequest resource=[[VirtualAttachmentResource name=name]]]]
  return self |> depth(attachment, usage_access, read_only)

[generic]
def private depth(var self : VirtualPassRequest; attachment : VirtualAttachmentRequest; usage_access : Access; read_only : bool) : VirtualPassRequest
  verify(self.registry.nodes[self.nodeId].renderingRequirements is some)

  let setReqs = $ <| (var reqs : VirtualPassRequirements)
    if reqs.depthAttachment.nameId != ResNameId Invalid
      let selfName = self.registry.knownNodeNames |> getName(self.nodeId)
      error("Encountered duplicate depth attachment calls on the same pass request in {selfName} frame graph node! Ignoring one of them!")

    let uid = self |> processAttachment(attachment, usage_access)
    verify(!uid.history)
    reqs.depthAttachment = [[VirtualSubresourceRef nameId=uid.nameId, mipLevel=attachment.mipLevel, layer=attachment.layer]]
    reqs.depthReadOnly = read_only

  setReqs |> invoke(self.registry.nodes[self.nodeId].renderingRequirements as some)

  return self


[generic]
def depthRw(var self : VirtualPassRequest; attachment : auto) : VirtualPassRequest
  return self |> depth(attachment, Access READ_WRITE, false)


[generic]
def depthRo(var self : VirtualPassRequest; attachment : auto) : VirtualPassRequest
  return self |> depth(attachment, Access READ_ONLY, true);


[generic]
def fill_slot(slot : NamedSlot; name : string)
  fill_slot(slot.name, name)
