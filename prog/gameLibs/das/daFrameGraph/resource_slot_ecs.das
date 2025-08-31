options no_aot
module resource_slot_ecs shared
require daFrameGraph.resource_slot public
require ecs public
require DagorSystem public

require daslib/ast_boost
require daslib/templates_boost
require DeferToAct

let private RES_SLOT_ECS_NODE_PASS_ES <- [ "track", "on_event", "on_appear", "REQUIRE", "REQUIRE_NOT"]

struct private ResSlotEcsAnnotationArgs
  ok : bool
  needDeclareEs : bool
  fnArguments : array<VariablePtr>
  callArguments : array<ExpressionPtr>

def is_valid_access_handle(handle : NodeHandleWithSlotsAccess&) : bool
  return handle.valid

def is_valid_access_handle(handle : NodeHandleWithSlotsAccessVector&) : bool
  if handle |> length() == 0
    return false
  return handle[0].valid

//! Declare resource slot access, that stored in ecs
[function_macro(name="resource_slot_ecs")]
class ResourceSlotEcsAnnotation : AstFunctionAnnotation

  def parseArgs(func : FunctionPtr; args : AnnotationArgumentList; var errors : das_string) : ResSlotEcsAnnotationArgs

    var needDeclareEs = false

    for arg in args
      if !(RES_SLOT_ECS_NODE_PASS_ES |> has_value("{arg.name}"))
        errors := "Unexpected parameter '{arg.name}' in [resource_slot_ecs] for {func.name}; expected one of {RES_SLOT_ECS_NODE_PASS_ES}"
        return <- ResSlotEcsAnnotationArgs()

      if !(arg.basicType == Type.tString || arg.basicType == Type.tBool)
        errors := "Unexpected type '{arg.basicType}' for parameter '{arg.name}' in [resource_slot_ecs] for {func.name}; expected string or bool"
        return <- ResSlotEcsAnnotationArgs()

      if [ "track", "on_event", "on_appear"] |> has_value("{arg.name}")
        needDeclareEs = true

    let persistentHeapArg = compiling_program()._options |> find_arg("persistent_heap")
    if !(persistentHeapArg is tBool && (persistentHeapArg as tBool))
      errors := "The file {func.at.fileInfo.name} contains [resource_slot_ecs] must use persistent heap `options persistent_heap` for correct callback handling!"
      return <- ResSlotEcsAnnotationArgs()

    if func.arguments |> length() == 0
      errors := "Function {func.name} annotated with [resource_slot_ecs] expects at least one parameter, but none provided."
      return <- ResSlotEcsAnnotationArgs()

    let firstArgType = describe_typedecl(func.arguments[0]._type, true, true, true)
    if !(firstArgType == "ResourceSlotCore::NodeHandleWithSlotsAccess& -const" || firstArgType == "$::NodeHandleWithSlotsAccessVector& -const")
      errors := "Function {func.name} annotated with [resource_slot_ecs] expects first parameter '{func.arguments[0].name}' with type 'var {func.arguments[0].name} : NodeHandleWithSlotsAccess&' or 'var {func.arguments[0].name} : NodeHandleWithSlotsAccessVector&', but '{firstArgType}' provided."
      return <- ResSlotEcsAnnotationArgs()

    var inscope fnArguments : array<VariablePtr>
    var inscope callArguments : array<ExpressionPtr>
    for arg in func.arguments
      if !(arg._type.isConst || arg._type.isRef || arg._type.isPointer)
        errors := "{func.name}: argument 'var {describe(arg)}' is var, but it's change won't affected the actual data. Probably you forgot &"
        return <- ResSlotEcsAnnotationArgs()
      fnArguments |> emplace_new <| clone_variable(arg)
      callArguments |> emplace_new <| qmacro($i(arg.name))

    return <- ResSlotEcsAnnotationArgs(
      ok = true,
      needDeclareEs = needDeclareEs,
      fnArguments <- fnArguments,
      callArguments <- callArguments
    )

  def declareReloadCallback(func : FunctionPtr; parsed : ResSlotEcsAnnotationArgs; args : AnnotationArgumentList)
    // To understand declaration syntax, read https://dascript.org/doc/reference/language/reification.html

    var inscope reloadCallback <- qmacro_function("resource_slot_reload`{func.name}") <| $()
      if !is_daframegraph_runtime_initialized()
        logerr("daFG: dafg::startup must be called before loading any das scripts that use daFG!")
        return

      ecs::query() <| $ [ecs::es] ($a(parsed.fnArguments))
        if resource_slot_ecs::is_valid_access_handle($i(func.arguments[0].name))
          $c(func.name)($a(parsed.callArguments))

    assume reloadCallbackBody = reloadCallback.body as ExprBlock
    assume queryCall = reloadCallbackBody.list[1] as ExprCall
    assume queryBlock = (queryCall.arguments[0] as ExprMakeBlock)._block as ExprBlock
    assume queryEsArguments = queryBlock.annotations[0].arguments
    assert(queryEsArguments |> length() == 0)

    for arg in args
      if [ "REQUIRE", "REQUIRE_NOT"] |> has_value("{arg.name}")
        queryEsArguments |> add_annotation_argument("{arg.name}", "{arg.sValue}")

    reloadCallback |> append_annotation("DeferToAct", "defer_to_act")
    compiling_module() |> add_function(reloadCallback)

  def declareES(func : FunctionPtr; parsed : ResSlotEcsAnnotationArgs; args : AnnotationArgumentList)

    if !parsed.needDeclareEs
      return

    var inscope resSlotES <- qmacro_function("resource_slot_es`{func.name}") <| $(evt : Event; $a(parsed.fnArguments))
      if !is_daframegraph_runtime_initialized()
        logerr("daFG: dafg::startup must be called before loading any das scripts that use daFG!")
        return

      $c(func.name)($a(parsed.callArguments))

    var inscope esArguments <- [
        ("tag", RttiValue(tString="render"))
    ]
    for arg in args
      if RES_SLOT_ECS_NODE_PASS_ES |> has_value("{arg.name}")
        if arg.basicType == Type.tString
          esArguments |> push(("{arg.name}", RttiValue(tString = "{arg.sValue}")))
        elif arg.basicType == Type.tBool
          esArguments |> push(("{arg.name}", RttiValue(tBool = arg.bValue)))

    resSlotES |> append_annotation("ecs", "es", esArguments)
    compiling_module() |> add_function(resSlotES)

  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool

    var inscope parsed <- func |> parseArgs(args, errors)
    if !parsed.ok
      return false

    func |> declareReloadCallback(parsed, args)
    func |> declareES(parsed, args)

    return true
