options no_aot
module bfg_ecs shared
require daBfg.daBfg public
require DagorSystem public

require daslib/ast_boost
require daslib/templates_boost
require ecs
require DeferToAct

let private LOGERR_GENERATED_CODE = false
let private BFG_ECS_NODE_PASS_ES <- [{auto "track"; "on_event"; "on_appear"; "REQUIRE"; "REQUIRE_NOT"}]

struct private BfgEcsNodeAnnotationArgs
  ok : bool
  needDeclareEs : bool
  fnArguments : array<VariablePtr>
  callArguments : array<ExpressionPtr>

def is_valid_node_handle(handle : NodeHandle&) : bool
  return handle.valid

def is_valid_node_handle(handle_vector : NodeHandleVector&) : bool
  if handle_vector |> length() == 0
    return false
  return handle_vector[0].valid

//! Declare node, that stored in ecs
//!
//! This node will be reloaded automatically on das scripts reload.
//!
//! For every register node function like this:
//!
//! [bfg_ecs_node]
//! def register_sample_node(var sample_node: NodeHandle &)
//!   sample_node <- root() |> registerNode("node_name") <| @(var registry : Registry)
//!     ...
//!
//! Will be automatically generated reload function like this:
//!
//! [defer_to_act]
//! def reload_node`register_sample_node()
//!   query() <| $ [es] (var sample_node: NodeHandle &)
//!     register_sample_node(sample_node)
//!
//! This function will be called in act stage after reloading of script.
//!
//! Also supports:
//! - Additional arguments of function translates to es components
//! - on_appear, on_event, track, REQUIRE and REQUIRE_NOT pass to [es] macro as is
[function_macro(name="bfg_ecs_node")]
class BfgEcsNodeAnnotation : AstFunctionAnnotation

  def declareReloadCallback(func : FunctionPtr; parsed : BfgEcsNodeAnnotationArgs; args : AnnotationArgumentList)
    // To understand declaration syntax, read https://dascript.org/doc/reference/language/reification.html

    var inscope reloadCallback <- qmacro_function("bfg_ecs_reload_node`{func.name}") <| $()
      if !is_dabfg_runtime_initialized()
        logerr("dabfg::startup must be called before loading any das scripts that use daBfg!")
        return

      ecs::query() <| $ [ecs::es] ($a(parsed.fnArguments))
        if bfg_ecs::is_valid_node_handle($i(func.arguments[0].name))
          $c(func.name)($a(parsed.callArguments))

    // Unfortunately, functional macro arguments [es] through $a call
    // (like function calls) is unsupported.
    // We have to add REQUIRE by hacky modifying of ast tree.
    assume reloadCallbackBody = reloadCallback.body as ExprBlock
    assume queryCall = reloadCallbackBody.list[1] as ExprCall
    assume queryBlock = (queryCall.arguments[0] as ExprMakeBlock)._block as ExprBlock
    assume queryEsArguments = queryBlock.annotations[0].arguments
    // Provoke null pointer exception
    assert(queryEsArguments |> length() == 0)

    for arg in args
      if [{auto "REQUIRE"; "REQUIRE_NOT"}] |> has_value("{arg.name}")
        queryEsArguments |> add_annotation_argument("{arg.name}", "{arg.sValue}")

    reloadCallback |> append_annotation("DeferToAct", "defer_to_act")

    if LOGERR_GENERATED_CODE
      logerr(describe(reloadCallback))
    compiling_module() |> add_function(reloadCallback)

  def declareES(func : FunctionPtr; parsed : BfgEcsNodeAnnotationArgs; args : AnnotationArgumentList)

    if !parsed.needDeclareEs
      return

    var inscope bfgES <- qmacro_function("bfg_es`{func.name}") <| $(evt : Event; $a(parsed.fnArguments))
      if !is_dabfg_runtime_initialized()
        logerr("dabfg::startup must be called before loading any das scripts that use daBfg!")
        return

      $c(func.name)($a(parsed.callArguments))

    var inscope esArguments <- [{auto
        [[auto "tag", [[RttiValue tString="render"]] ]]
    }]
    for arg in args
      if BFG_ECS_NODE_PASS_ES |> has_value("{arg.name}")
        if arg.basicType == Type tString
          esArguments |> push([[auto "{arg.name}", [[RttiValue tString="{arg.sValue}"]] ]])
        elif arg.basicType == Type tBool
          esArguments |> push([[auto "{arg.name}", [[RttiValue tBool=arg.bValue]] ]])

    bfgES |> append_annotation("ecs", "es", esArguments)

    if LOGERR_GENERATED_CODE
      logerr(describe(bfgES))
    compiling_module() |> add_function(bfgES)

  def parseArgs(func : FunctionPtr; args : AnnotationArgumentList; var errors : das_string) : BfgEcsNodeAnnotationArgs

    var needDeclareEs = false

    for arg in args
      if !(BFG_ECS_NODE_PASS_ES |> has_value("{arg.name}"))
        errors := "Unexpected parameter '{arg.name}' in [bfg_ecs_node] for {func.name}; expected one of {BFG_ECS_NODE_PASS_ES}"
        return <- [[BfgEcsNodeAnnotationArgs]]

      if !(arg.basicType == Type tString || arg.basicType == Type tBool)
        errors := "Unexpected type '{arg.basicType}' for parameter '{arg.name}' in [bfg_ecs_node] for {func.name}; expected string or bool"
        return <- [[BfgEcsNodeAnnotationArgs]]

      if [{auto "track"; "on_event"; "on_appear"}] |> has_value("{arg.name}")
        needDeclareEs = true

    let persistentHeapArg = compiling_program()._options |> find_arg("persistent_heap")
    if !(persistentHeapArg is tBool && (persistentHeapArg as tBool))
      errors := "The file {func.at.fileInfo.name} contains [bfg_ecs_node] must use persistent heap `options persistent_heap` for correct callback handling!"
      return <- [[BfgEcsNodeAnnotationArgs]]

    if func.arguments |> length() == 0
      errors := "Function {func.name} annotated with [bfg_ecs_node] expects at least one parameter, but none provided."
      return <- [[BfgEcsNodeAnnotationArgs]]

    let firstArgType = describe_typedecl(func.arguments[0]._type, true, true, true)
    if !(firstArgType == "$::NodeHandleVector& -const" || firstArgType == "daBfgCore::NodeHandle& -const")
      errors := "Function {func.name} annotated with [bfg_ecs_node] expects first parameter '{func.arguments[0].name}' with type 'NodeHandle&' or 'NodeHandleVector&', but '{firstArgType}' provided."
      return <- [[BfgEcsNodeAnnotationArgs]]

    var inscope fnArguments : array<VariablePtr>
    var inscope callArguments : array<ExpressionPtr>
    for arg in func.arguments
      if !(arg._type.isConst || arg._type.isRef || arg._type.isPointer)
        errors := "{func.name}: argument 'var {describe(arg)}' is var, but it's change won't affected the actual data. Probably you forgot &"
        return <- [[BfgEcsNodeAnnotationArgs]]
      fnArguments |> emplace_new <| clone_variable(arg)
      callArguments |> emplace_new <| qmacro($i(arg.name))

    return <- [[BfgEcsNodeAnnotationArgs
      ok = true,
      needDeclareEs = needDeclareEs,
      fnArguments <- fnArguments,
      callArguments <- callArguments,
    ]]


  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool

    var inscope parsed <- func |> parseArgs(args, errors)
    if !parsed.ok
      return false

    func |> declareReloadCallback(parsed, args)
    func |> declareES(parsed, args)

    return true