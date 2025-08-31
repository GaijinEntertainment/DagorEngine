options no_unused_block_arguments = false

module daFG shared

require daFgCore public
require DagorDriver3D public
require DagorShaders public

require ast
require daslib/ast_boost
require daslib/macro_boost
require daslib/templates_boost


struct NameSpaceRequest
  nameSpaceId : NameSpaceNameId
  nodeId : NodeNameId
  registry : InternalRegistry?

struct Registry : NameSpaceRequest {}

struct NameSpace
  nameSpaceId : NameSpaceNameId

struct ResUid
  nameId : ResNameId
  history : bool

struct VirtualResourceRequestBase
  registry : InternalRegistry?
  resUid : ResUid
  nodeId : NodeNameId

struct VirtualResourceCreationSemiRequest : VirtualResourceRequestBase {}
struct VirtualResourceSemiRequest : VirtualResourceRequestBase {}
struct VirtualResourceRequest : VirtualResourceRequestBase {}
struct VirtualTextureRequest : VirtualResourceRequest {}
struct VirtualBufferRequest : VirtualResourceRequest {}

struct VirtualResourceHandleBase
  provider : ResourceProvider const?
  resUid : ResUid

struct VirtualTextureHandle : VirtualResourceHandleBase {}
struct VirtualBufferHandle : VirtualResourceHandleBase {}



struct StateRequest
  registry : InternalRegistry?
  nodeId : NodeNameId

struct VrsSettings
  rateX : uint
  rateY : uint
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


struct AutoResolutionRequestBase
  autoResTypeId : AutoResTypeNameId
  multiplier : float = 1.0f
  provider : ResourceProvider const?

struct AutoResolutionRequest2 : AutoResolutionRequestBase {}
struct AutoResolutionRequest3 : AutoResolutionRequestBase {}

variant TextureResolution2
  res : tuple<x : uint; y : uint>
  autoRes : AutoResolutionRequest2


struct Texture2dCreateInfo
  resolution : TextureResolution2 = TextureResolution2(uninitialized)
  creationFlags : uint
  mipLevels : uint = 1u


struct BufferCreateInfo
  elementSize : uint
  elementCount : uint
  flags : uint
  format : uint


struct NamedSlot
  name : string

def root() : NameSpace
  if is_in_aot() || is_in_completion()
    return <- NameSpace()

  return NameSpace(nameSpaceId = getRegistry().knownNames |> root)


def operator /(var self : NameSpace; child_name : string) : NameSpace
  if is_in_aot() || is_in_completion()
    return <- NameSpace()

  return NameSpace(nameSpaceId = getRegistry().knownNames |> addNameId`NameSpaceNameId(self.nameSpaceId, child_name))


def fillSlot(var self : NameSpace; slot : NamedSlot; res_name_space : NameSpace; res_name : string)
  fill_slot(self.nameSpaceId, slot.name, res_name_space.nameSpaceId, res_name)


def private get(var resources : idIndexedMapping`ResNameId`ResourceData&; res_id : ResNameId; cb : block<(var value : ResourceData) : void>) : void
  cb |> invoke(resources |> get(res_id))


def private get(var nodes : idIndexedMapping`NodeNameId`NodeData&; node_id : NodeNameId; cb : block<(var value : NodeData) : NodeHandle>) : NodeHandle
  return <- cb |> invoke(nodes |> get(node_id))


def registerNode(var self : NameSpace; name : string; var declaration_callback : lambda<(var reg : Registry) : lambda<void>>) : NodeHandle
  if is_in_aot() || is_in_completion()
    return <- NodeHandle()

  var registry = getRegistry()
  let nodeId = registry.knownNames |> addNameId`NodeNameId(self.nameSpaceId, name)
  return <- registry.nodes |> get(nodeId) <| $(var node_data : NodeData&)
    getTracker() |> unregisterNode(nodeId, node_data.generation)
    getTracker() |> registerNode(nodeId)
    node_data |> registerNodeDeclaration(self.nameSpaceId) <| @  capture(<- declaration_callback, = registry) ()
      return <- declaration_callback |> invoke() <| Registry(
        nameSpaceId = self.nameSpaceId,
        nodeId = nodeId,
        registry = registry
      )
    return <- register_external_node(nodeId, node_data.generation)


def root(var self : Registry) : NameSpaceRequest
  return NameSpaceRequest(nameSpaceId = self.registry.knownNames |> root, nodeId = self.nodeId, registry = self.registry)


def orderMeBefore(var self : Registry; name : string) : Registry
  let nodeNsId = self.registry.knownNames |> getParent(self.nodeId)
  let nameId = self.registry.knownNames |> getNameId`NodeNameId(nodeNsId, name)
  if nameId != NodeNameId.Invalid
    self.registry.nodes[self.nodeId].followingNodeIds |> insert(nameId)
  else
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Node {selfName} tries to order before not existing node {name}, skipping this order")

  return self


def orderMeBefore(var self : Registry; names : array<string>) : Registry
  for name in names
    self |> orderMeBefore(name)

  return self


def orderMeAfter(var self : Registry; name : string) : Registry
  let nodeNsId = self.registry.knownNames |> getParent(self.nodeId)
  let nameId = self.registry.knownNames |> getNameId`NodeNameId(nodeNsId, name)
  if nameId != NodeNameId.Invalid
    self.registry.nodes[self.nodeId].precedingNodeIds |> insert(nameId)
  else
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Node {selfName} tries to order after not existing node {name}, skipping this order")

  return self


def orderMeAfter(var self : Registry; names : array<string>) : Registry
  for name in names
    self |> orderMeAfter(name)

  return self


def setPriority(var self : Registry; priority : int) : Registry
  self.registry.nodes[self.nodeId].priority = priority
  return self


def multiplex(var self : Registry; multiplexing_mode : MultiplexingMode) : Registry
  assume multiplexMode = self.registry.nodes[self.nodeId].multiplexingMode as some
  multiplexMode = multiplexing_mode
  return self


def executionHas(var self : Registry; side_effect : SideEffect) : Registry
  self.registry.nodes[self.nodeId].sideEffect = side_effect
  return self


def create(var self : Registry; name : string; history : History) : VirtualResourceCreationSemiRequest
  let nodeNsId = self.registry.knownNames |> getParent(self.nodeId)
  let nameId = self.registry.knownNames |> addNameId`ResNameId(nodeNsId, name)
  self.registry.resources |> get(nameId) <| $(var res : ResourceData&)
    res.history = history

  self.registry.nodes[self.nodeId].createdResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = ResourceUsage(access = Access.READ_WRITE)
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- VirtualResourceCreationSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = nameId, history = false))


def operator /(var self : NameSpaceRequest; child_name : string) : NameSpaceRequest
  return NameSpaceRequest(
    nameSpaceId = self.registry.knownNames |> addNameId`NameSpaceNameId(self.nameSpaceId, child_name),
    nodeId = self.nodeId,
    registry = self.registry)


def getResolution`2(var self : NameSpaceRequest; type_name : string; multiplier : float = 1.f) : AutoResolutionRequest2
  return AutoResolutionRequest2(
    autoResTypeId = self.registry.knownNames |> addNameId`AutoResTypeNameId(self.nameSpaceId, type_name),
    multiplier = multiplier,
    provider = self.registry |> getProvider())

def getResolution`3(var self : NameSpaceRequest; type_name : string; multiplier : float = 1.f) : AutoResolutionRequest3
  return AutoResolutionRequest3(
    autoResTypeId = self.registry.knownNames |> addNameId`AutoResTypeNameId(self.nameSpaceId, type_name),
    multiplier = multiplier,
    provider = self.registry |> getProvider())

def read(var self : NameSpaceRequest; name : string) : VirtualResourceSemiRequest
  let nameId = self.registry.knownNames |> addNameId`ResNameId(self.nameSpaceId, name)
  self.registry.nodes[self.nodeId].readResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = ResourceUsage(access = Access.READ_ONLY)
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- VirtualResourceSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = nameId, history = false))


def read(var self : NameSpaceRequest; slot : NamedSlot) : VirtualResourceSemiRequest
  let nameId = self.registry.knownNames |> addNameId`ResNameId(self.nameSpaceId, slot.name)
  self.registry.nodes[self.nodeId].readResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.slotRequest = true
    req.usage = ResourceUsage(access = Access.READ_ONLY)
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- VirtualResourceSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = nameId, history = false))


def historyFor(var self : NameSpaceRequest; var name : string) : VirtualResourceSemiRequest
  let nameId = self.registry.knownNames |> addNameId`ResNameId(self.nameSpaceId, name)
  using() <| $(var req : ResourceRequest#)
    req.usage = ResourceUsage(access = Access.READ_ONLY)
    self.registry.nodes[self.nodeId].historyResourceReadRequests[nameId] := req

  return <- VirtualResourceSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = nameId, history = true))


def modify(var self : NameSpaceRequest; name : string) : VirtualResourceSemiRequest
  let nameId = self.registry.knownNames |> addNameId`ResNameId(self.nameSpaceId, name)
  self.registry.nodes[self.nodeId].modifiedResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.usage = ResourceUsage(access = Access.READ_WRITE)
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- VirtualResourceSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = nameId, history = false))


def modify(var self : NameSpaceRequest; slot : NamedSlot) : VirtualResourceSemiRequest
  let nameId = self.registry.knownNames |> addNameId`ResNameId(self.nameSpaceId, slot.name)
  self.registry.nodes[self.nodeId].modifiedResources |> insert(nameId)
  using() <| $(var req : ResourceRequest#)
    req.slotRequest = true
    req.usage = ResourceUsage(access = Access.READ_WRITE)
    self.registry.nodes[self.nodeId].resourceRequests[nameId] := req

  return <- VirtualResourceSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = nameId, history = false))


def rename(var self : NameSpaceRequest; from : string; to : string; history : History) : VirtualResourceSemiRequest
  let fromResId = self.registry.knownNames |> addNameId`ResNameId(self.nameSpaceId, from)
  let nodeNsId = self.registry.knownNames |> getParent(self.nodeId)
  let toResId = self.registry.knownNames |> addNameId`ResNameId(nodeNsId, to)

  self.registry.nodes[self.nodeId].renamedResources[toResId] := fromResId
  using() <| $(var req : ResourceRequest#)
    req.usage = ResourceUsage(access = Access.READ_WRITE)
    self.registry.nodes[self.nodeId].resourceRequests[fromResId] := req
  self.registry.resources |> get(toResId) <| $(var res : ResourceData&)
    res.history = history

  return <- VirtualResourceSemiRequest(registry = self.registry, nodeId = self.nodeId, resUid = ResUid(nameId = fromResId, history = false))


def texture(var self : VirtualResourceCreationSemiRequest; info : Texture2dCreateInfo) : VirtualTextureRequest
  self.registry.resources |> get(self.resUid.nameId) <| $(var res : ResourceData&)
    using() <| $(var desc : TextureResourceDescription#)
      desc.mipLevels = info.mipLevels
      desc.cFlags = info.creationFlags
      if info.resolution is autoRes
        res |> setResolution(AutoResolutionData(
          id = info.resolution as autoRes.autoResTypeId,
          multiplier = info.resolution as autoRes.multiplier))
      else
        desc.width = info.resolution as res.x
        desc.height = info.resolution as res.y
      res |> setDescription(desc)

    res.resType = ResourceType.Texture

  return <- VirtualTextureRequest(registry = self.registry, nodeId = self.nodeId, resUid = self.resUid)


def texture(var self : VirtualResourceSemiRequest) : VirtualTextureRequest
  return <- VirtualTextureRequest(registry = self.registry, nodeId = self.nodeId, resUid = self.resUid)


def buffer(var self : VirtualResourceCreationSemiRequest; info : BufferCreateInfo) : VirtualBufferRequest
  self.registry.resources |> get(self.resUid.nameId) <| $(var res : ResourceData&)
    using() <| $(var desc : BufferResourceDescription#)
      desc.elementCount = info.elementCount
      desc.elementSizeInBytes = info.elementSize
      desc.viewFormat = info.format
      desc.cFlags = info.flags
      desc.activation = ResourceActivationAction.DISCARD_AS_UAV
      res |> setDescription(desc)

    res.resType = ResourceType.Buffer

  return <- VirtualBufferRequest(registry = self.registry, nodeId = self.nodeId, resUid = self.resUid)


def buffer(var self : VirtualResourceSemiRequest) : VirtualBufferRequest
  return <- VirtualBufferRequest(registry = self.registry, nodeId = self.nodeId, resUid = self.resUid)


[macro_function]
def isDasNative(var t : TypeDeclPtr) : bool
  if t.isHandle
    return false

  if length(t.dim) > 0
    return true

  // These are interop types by default, see blobBindings.cpp
  return (
    t.baseType != Type.tBool &&
    t.baseType != Type.tInt8 &&
    t.baseType != Type.tUInt8 &&
    t.baseType != Type.tInt16 &&
    t.baseType != Type.tUInt16 &&
    t.baseType != Type.tInt &&
    t.baseType != Type.tUInt &&
    t.baseType != Type.tInt64 &&
    t.baseType != Type.tUInt64 &&
    t.baseType != Type.tFloat &&
    t.baseType != Type.tDouble
  )


[macro_function]
def useDasCtorDtor(var t : TypeDeclPtr) : bool
  // For Pods, we can use memset to 0 on the C++ side
  return !t.isPod && isDasNative(t)

[macro_function]
def private tryAddBlobSpecializations(var blobType : TypeDeclPtr; hasDefValue : bool) : void
  let mangledBlobTypeName = get_mangled_name(blobType)
  let blobTypeHash = hash(mangledBlobTypeName)

  // We add 2 new types per blob type: blob request and blob handle
  var inscope vrr_type <- typeinfo ast_typedecl(type<VirtualResourceRequest>)
  var vrrBase = vrr_type.structType
  var inscope blobRequestStruct <- clone_structure(vrrBase)
  blobRequestStruct.name := "VirtualBlobRequest`{blobTypeHash}"
  blobRequestStruct.parent := vrrBase
  blobRequestStruct.flags |= StructureFlags.generated

  var inscope vrhb_type <- typeinfo ast_typedecl(type<VirtualResourceHandleBase>)
  var vrhBase = vrhb_type.structType
  var inscope blobHandleStruct <- clone_structure(vrhBase)
  blobHandleStruct.name := "VirtualBlobHandle`{blobTypeHash}"
  blobHandleStruct.parent := vrhBase
  blobHandleStruct.flags |= StructureFlags.generated

  var inscope blobHandleFn <- qmacro_function("handle") <| \
    $(self : $t(blobRequestStruct)) : $t(blobHandleStruct) {
      return <- struct<$t(blobHandleStruct)>(provider = self.registry |> getProvider(), resUid = self.resUid);
    }
  compiling_module() |> add_function(blobHandleFn)

  if !blobType.isRef
    var inscope blobGetFn <- qmacro_function("get") <| \
      $(self : $t(blobHandleStruct); cb : block<(var value : $t(blobType)&) : void>) : void {
        let blobView = self.provider |> getBlobView(self.resUid.nameId, self.resUid.history);
        invoke(cb) <| deref(unsafe(reinterpret<$t(blobType)?> blobView |> getData($v(mangledBlobTypeName))));
      }
    compiling_module() |> add_function(blobGetFn)
  else
    var inscope blobGetFn <- qmacro_function("get") <| \
      $(self : $t(blobHandleStruct); cb : block<(var value : $t(blobType)) : void>) : void {
        let blobView = self.provider |> getBlobView(self.resUid.nameId, self.resUid.history);
        invoke(cb) <| deref(unsafe(reinterpret<$t(blobType)?> blobView |> getData($v(mangledBlobTypeName))));
      }
    compiling_module() |> add_function(blobGetFn)

  if useDasCtorDtor(blobType)
    var inscope exprReinterpret <- qmacro(unsafe(reinterpret<$t(blobType)?> self))
    // NOTE: some project use the "please error out if some optimizations
    // were not possible due to aliasing restrictions" mode, so we need to
    // explicitly say that no aliasing is happening here, hence the `assumeNoAlias`
    var inscope exprDeref <- new ExprPtr2Ref(subexpr <- exprReinterpret, assumeNoAlias = true)
    var inscope blobCtorFn <- qmacro_function("ctor`{blobTypeHash}") <| \
      $(var self : void?) : void {
        $e(exprDeref) <- struct<$t(blobType)>();
      }
    compiling_module() |> add_function(blobCtorFn)

    var inscope blobDtorFn <- qmacro_function("dtor`{blobTypeHash}") <| \
      $(var self : void?) : void {
        delete deref(unsafe(reinterpret<$t(blobType)?> self));
      }
    compiling_module() |> add_function(blobDtorFn)

    var inscope exprCastFrom <- qmacro(unsafe(reinterpret<$t(blobType) const?> from))
    var inscope exprFrom <- new ExprPtr2Ref(subexpr <- exprCastFrom, assumeNoAlias = true)
    var inscope blobCopyFn <- qmacro_function("copy`{blobTypeHash}") <| \
      $(var self : void?; from : void const?) : void {
        $e(clone_expression(exprDeref)) = $e(exprFrom);
      }
    compiling_module() |> add_function(blobCopyFn)

  if hasDefValue
    var inscope exprReinterpret <- qmacro(unsafe(reinterpret<$t(blobType)?> self))
    // NOTE: some project use the "please error out if some optimizations
    // were not possible due to aliasing restrictions" mode, so we need to
    // explicitly say that no aliasing is happening here, hence the `assumeNoAlias`
    var inscope exprDeref <- new ExprPtr2Ref(subexpr <- exprReinterpret, assumeNoAlias = true)
    var inscope blobDefCtorFn <- qmacro_function("ctor_def`{blobTypeHash}") <| \
      $(var self : void?; def_value : $t(blobType)) : void {
        $e(exprDeref) <- def_value;
      }
    compiling_module() |> add_function(blobDefCtorFn)

  compiling_module() |> add_structure(blobRequestStruct)
  compiling_module() |> add_structure(blobHandleStruct)

// when we do registry |> create("kek") |> blob(type<int>), we need to
// generate a bunch of stuff and inject it into the script to be used by
// the following functions like handle, get, etc.
[call_macro(name="blob")]
class BlobMacro : AstCallMacro
  [unused_argument(mod)] def override visit(prog : ProgramPtr; mod : Module ?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr
    macro_verify(expr.arguments |> length == 2, prog, expr.at,
      "Function-like macro `blob` accepts two arguments: request and \
       blob type, e.g. registry |> read(\"sea_level\") |> blob(type<float>)")

    assume request = expr.arguments[0]
    var inscope creationReqTd <- typeinfo ast_typedecl(type<VirtualResourceCreationSemiRequest>)
    let isCreationReq = is_same_type(request._type, creationReqTd, RefMatters.no, ConstMatters.no, TemporaryMatters.no)
    var inscope regularReqTd <- typeinfo ast_typedecl(type<VirtualResourceSemiRequest>)
    let isRegularReq = is_same_type(request._type, regularReqTd, RefMatters.no, ConstMatters.no, TemporaryMatters.no)

    macro_verify(isCreationReq || isRegularReq, prog, expr.at,
                 "First argument of function-like macro `blob` must be a resource request, \
                  e.g. registry |> read(\"sea_level\") or registry |> create(\"sea_level\", History No)")

    let hasValue = !(expr.arguments[1] is ExprTypeDecl)
    var inscope blobType := expr.arguments[1]._type
    if hasValue
      blobType.flags &= ~TypeDeclFlags.constant
      blobType.flags &= ~TypeDeclFlags.ref
      blobType.flags &= ~TypeDeclFlags.explicitRef
    assume arg1 = expr.arguments[1]

    macro_verify(blobType != null, prog, expr.at,
                 "Unknown blob type! Perhaps you forgot to require the module that contains it? Got `{describe(expr.arguments[1])}`")
    macro_verify(!blobType.isVoid, prog, expr.at,
                 "Blob type must not be void! Got '{describe(blobType)}'")
    macro_verify(!blobType.isConst, prog, expr.at,
                 "Blob type must not be a constant! Got {describe(blobType)}")
    // hasValue -> !useDasCtorDtor(blobType)
    macro_verify(!hasValue || !useDasCtorDtor(blobType), prog, expr.at,
                 "Non-pod types are not supported yet! Got {describe(blobType)}")

    tryAddBlobSpecializations(blobType, hasValue)

    let mangledBlobTypeName = get_mangled_name(blobType)
    let blobTypeHash = hash(mangledBlobTypeName)

    var requestStruct = compiling_program() |> find_unique_structure("VirtualBlobRequest`{blobTypeHash}")

    macro_verify(requestStruct != null, prog, expr.at, "Failed to retreive the request structure back from the module!")

    if isCreationReq
      return <- qmacro_expr <| ${
          $(var self : VirtualResourceCreationSemiRequest) {
            $(var res : ResourceData&) {

              if $v(useDasCtorDtor(blobType)) {
                res |> setDescription($v(mangledBlobTypeName), $v(blobType.sizeOf), $v(blobType.alignOf),
                  getTypeInfo(type<$t(blobType)>), @@$c("ctor`{blobTypeHash}"), @@$c("dtor`{blobTypeHash}"), @@$c("copy`{blobTypeHash}"));
              } else {
                let nullFunc = default<function<(var arg0 : void?) : void>>;
                let nullCopyFunc = default<function<(var arg0 : void?; arg1 : void const?) : void>>;
                res |> setDescription($v(mangledBlobTypeName), $v(blobType.sizeOf), $v(blobType.alignOf),
                  getTypeInfo(type<$t(blobType)>), nullFunc, nullFunc, nullCopyFunc);
              }

              static_if $v(hasValue) {
                let ctor <- @(var self1 : void?){ ($c("ctor_def`{blobTypeHash}")(self1, $e(arg1))); };
                res |> overrideCtor(getTypeInfo(type<$t(blobType)>), ctor);
              }

              res.resType = ResourceType.Blob;
            } |> invoke(self.registry.resources |> get(self.resUid.nameId));
            self |> modifyRequest() <| $(var request : ResourceRequest&) {
              request |> markWithTag($v(mangledBlobTypeName));
            }
            return <- struct<$t(requestStruct)>(registry = self.registry, nodeId = self.nodeId, resUid = self.resUid);
          } |> invoke($e(request));
        }
    else
      return <- qmacro_expr <| ${
          $(var self : VirtualResourceSemiRequest) {
            self |> modifyRequest() <| $(var request : ResourceRequest&) {
              request |> markWithTag($v(mangledBlobTypeName));
            }
            return <- struct<$t(requestStruct)>(registry = self.registry, nodeId = self.nodeId, resUid = self.resUid);
          } |> invoke($e(request));
        }


def modifyRequest(var self; modifier : block<(var request : daFgCore::ResourceRequest) : void>)
  invoke(modifier, self.resUid.history ?
    self.registry.nodes[self.nodeId].historyResourceReadRequests[self.resUid.nameId] :
    self.registry.nodes[self.nodeId].resourceRequests[self.resUid.nameId])


def optional(var self : auto(TT)) : TT
  let setOptional = $ <| (var request : ResourceRequest &)
    request.optional = true
  self |> modifyRequest(setOptional)
  return self


def useAs(var self : auto(TT); usageType : Usage) : TT
  let setUsage = $ <| (var request : ResourceRequest &)
    request.usage.usageType = usageType
  self |> modifyRequest(setUsage)
  return self


def atStage(var self : auto(TT); stage : Stage) : TT
  let setStage = $ <| (var request : ResourceRequest &)
    request.usage.stage = stage
  self |> modifyRequest(setStage)
  return self


def bindToShaderVar(var self : auto(TT); name : string = "") : TT
  var svName = name
  if (name == "")
    svName = self.registry.knownNames |> getShortName(self.resUid.nameId)

  let svId = DagorShaders::get_shader_variable_id(svName)
  assume bindings = self.registry.nodes[self.nodeId].bindings
  if !(bindings |> key_exists(svId))
    using() <| $(var info : Binding#)
      info.bindType = BindingType.ShaderVar
      info.resource = self.resUid.nameId
      info.history = self.resUid.history
      bindings[svId] := info
      self |> modifyRequest() <| $(var request : ResourceRequest&)
        request |> useRequestTagForBinding(bindings[svId])
  else
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Encountered duplicate shader var {name} binding in {selfName} frame graph node! Ignoring one of the them!")

  useAs(self, Usage.SHADER_RESOURCE)
  return self


def handle(self : VirtualTextureRequest) : VirtualTextureHandle
  return <- VirtualTextureHandle(provider = self.registry |> getProvider(), resUid = self.resUid)


def handle(self : VirtualBufferRequest) : VirtualBufferHandle
  return <- VirtualBufferHandle(provider = self.registry |> getProvider(), resUid = self.resUid)


def view(self : VirtualTextureHandle) : ManagedTexView
  return self.provider |> getTexView(self.resUid.nameId, self.resUid.history)


def view(self : VirtualBufferHandle) : ManagedBufView
  return self.provider |> getBufView(self.resUid.nameId, self.resUid.history)


def get(self : AutoResolutionRequest2) : int2
  let baseRes = self.provider |> getResolution`2(self.autoResTypeId)
  return int2(float2(baseRes) * self.multiplier)

def get(self : AutoResolutionRequest3) : int3
  let baseRes = self.provider |> getResolution`3(self.autoResTypeId)
  return int3(float3(baseRes) * self.multiplier)

def requestState(var self : Registry) : StateRequest
  if self.registry.nodes[self.nodeId].stateRequirements is some
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Global state requested twice on {selfName} frame graph node! Ignoring one of the requests!")

  self.registry.nodes[self.nodeId].stateRequirements |> emplace_some

  return StateRequest(registry = self.registry, nodeId = self.nodeId)


def private setBlock(var self : StateRequest; name : string; layer : string; var value : int&) : StateRequest
  if value != -1
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Block requested to be set to layer {layer} twice within {selfName} frame graph node! Ignoring one of the requests!")

  value = getBlockId(name)
  return self


def setFrameBlock(var self : StateRequest; name : string) : StateRequest
  return self |> setBlock(name, "'FRAME'", self.registry.nodes[self.nodeId].shaderBlockLayers.frameLayer)


def setSceneBlock(var self : StateRequest; name : string) : StateRequest
  return self |> setBlock(name, "'SCENE'", self.registry.nodes[self.nodeId].shaderBlockLayers.sceneLayer)


def setObjectBlock(var self : StateRequest; name : string) : StateRequest
  return self |> setBlock(name, "'OBJECT'", self.registry.nodes[self.nodeId].shaderBlockLayers.objectLayer)


def allowWireFrame(var self : StateRequest) : StateRequest
  assume stateReqs = self.registry.nodes[self.nodeId].stateRequirements as some
  stateReqs.supportsWireframe = true
  return self


def vrs(var self : StateRequest; vrs : VrsSettings) : StateRequest
  assume stateReqs = self.registry.nodes[self.nodeId].stateRequirements as some
  assume vrsState = stateReqs.vrsState

  vrsState.rateX = vrs.rateX
  vrsState.rateY = vrs.rateY
  vrsState.vertexCombiner = vrs.vertexCombiner
  vrsState.pixelCombiner = vrs.pixelCombiner

  return self


def enableOverride(var self : StateRequest; das_override : OverrideRenderState)
  assume stateReqs = self.registry.nodes[self.nodeId].stateRequirements as some
  stateReqs.pipelineStateOverride |> emplace_some
  assume final_override = stateReqs.pipelineStateOverride as some

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

  return self


def requestRenderPass(var self : Registry) : VirtualPassRequest
  if self.registry.nodes[self.nodeId].renderingRequirements is some
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Encountered two renderpass requests in {selfName} framegraph node! Ignoring one of the requests!")

  self.registry.nodes[self.nodeId].renderingRequirements |> emplace_some

  return VirtualPassRequest(registry = self.registry, nodeId = self.nodeId)


def private processAttachment(var self : VirtualPassRequest; attachment : VirtualAttachmentRequest; access_override : Access) : ResUid
  var uid : ResUid
  if attachment.resource is resUid
    uid = attachment.resource as resUid
  else
    let nodeNsId = self.registry.knownNames |> getParent(self.nodeId)
    let resId = self.registry.knownNames |> addNameId`ResNameId(nodeNsId, attachment.resource as name)
    if (access_override == Access.READ_ONLY)
      self.registry.nodes[self.nodeId].readResources |> insert(resId)
    else
      self.registry.nodes[self.nodeId].modifiedResources |> insert(resId)

    uid.nameId = resId
    uid.history = false

  return uid


def color(var self : VirtualPassRequest; requests : VirtualTextureRequest[]) : VirtualPassRequest
  let attachments <- [for req in requests; VirtualAttachmentRequest(resource=VirtualAttachmentResource(resUid=req.resUid))]
  return self |> color(attachments)


def color(var self : VirtualPassRequest; names : string[]) : VirtualPassRequest
  let attachments <- [for name in names; VirtualAttachmentRequest(resource=VirtualAttachmentResource(name=name))]
  return self |> color(attachments)


def color(var self : VirtualPassRequest; attachments : array<VirtualAttachmentRequest>) : VirtualPassRequest
  assume reqs = self.registry.nodes[self.nodeId].renderingRequirements as some

  if reqs.colorAttachments |> length != 0
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Encountered duplicate color attachment calls on the same pass request in {selfName} frame graph node! Ignoring one of them!")
    reqs.colorAttachments |> clear()

  for attachment in attachments
    let uid = self |> processAttachment(attachment, Access.READ_WRITE)
    verify(!uid.history)
    self.registry.nodes[self.nodeId].resourceRequests[uid.nameId].usage = ResourceUsage(access = Access.READ_WRITE, stage = Stage.POST_RASTER, usageType = Usage.COLOR_ATTACHMENT)
    reqs.colorAttachments |> push(VirtualSubresourceRef(nameId = uid.nameId, mipLevel = attachment.mipLevel, layer = attachment.layer))

  return self


def private depth(var self : VirtualPassRequest; name : string; usage_access : Access; read_only : bool) : VirtualPassRequest
  let attachment <- VirtualAttachmentRequest(resource = VirtualAttachmentResource(name = name))
  return self |> depth(attachment, usage_access, read_only)


def private depth(var self : VirtualPassRequest; attachment : VirtualAttachmentRequest; usage_access : Access; read_only : bool) : VirtualPassRequest
  assume reqs = self.registry.nodes[self.nodeId].renderingRequirements as some

  if reqs.depthAttachment.nameId != ResNameId.Invalid
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Encountered duplicate depth attachment calls on the same pass request in {selfName} frame graph node! Ignoring one of them!")

  let uid = self |> processAttachment(attachment, usage_access)
  verify(!uid.history)
  self.registry.nodes[self.nodeId].resourceRequests[uid.nameId].usage = ResourceUsage(access = read_only ? Access.READ_ONLY : Access.READ_WRITE,
    stage = Stage.POST_RASTER,
    usageType = read_only ? Usage.DEPTH_ATTACHMENT_AND_SHADER_RESOURCE : Usage.DEPTH_ATTACHMENT)
  reqs.depthAttachment = VirtualSubresourceRef(nameId = uid.nameId, mipLevel = attachment.mipLevel, layer = attachment.layer)
  reqs.depthReadOnly = read_only

  return self


def depthRw(var self : VirtualPassRequest; attachment : auto) : VirtualPassRequest
  return self |> depth(attachment, Access.READ_WRITE, false)


def depthRo(var self : VirtualPassRequest; attachment : auto) : VirtualPassRequest
  return self |> depth(attachment, Access.READ_ONLY, true)

def vrsRate(var self : VirtualPassRequest; name : string[]) : VirtualPassRequest
  let attachment <- VirtualAttachmentRequest(resource = VirtualAttachmentResource(name = name))
  return self |> color(attachments)

def vrsRate(var self : VirtualPassRequest; attachment : VirtualAttachmentRequest) : VirtualPassRequest
  assume reqs = self.registry.nodes[self.nodeId].renderingRequirements as some

  if reqs.vrsRateAttachment.nameId != ResNameId.Invalid
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Encountered duplicate VRS rate attachment calls on the same pass request in {selfName} frame graph node! Ignoring one of them!")

  let uid = self |> processAttachment(attachment, Access.READ_ONLY)
  verify(!uid.history)
  self.registry.nodes[self.nodeId].resourceRequests[uid.nameId].usage = ResourceUsage(access = Access.READ_ONLY,
    stage = Stage.POST_RASTER,
    usageType = Usage.VRS_RATE_TEXTURE)
  reqs.vrsRateAttachment = VirtualSubresourceRef(nameId = uid.nameId, mipLevel = attachment.mipLevel, layer = attachment.layer)

  if attachment.mipLevel != 0u || attachment.layer != 0u
    let selfName = self.registry.knownNames |> getName(self.nodeId)
    error("Warning: attempted to specify a mip level or a layer for a VRS rate texture in node {selfName}. \
          This is currently supported by NONE of our drivers and will be ignored.")

  return self
