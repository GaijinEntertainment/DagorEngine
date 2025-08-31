options no_aot
module fg_ecs shared
require daFrameGraph.daFG public
require DagorSystem public

require daslib/ast_boost
require daslib/templates_boost
require ecs
require DeferToAct

let private LOGERR_GENERATED_CODE = false
let private FG_ECS_NODE_PASS_ES <- [ "track", "on_event", "on_appear", "REQUIRE", "REQUIRE_NOT"]

struct private FgEcsNodeAnnotationArgs
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
//! [fg_ecs_node]
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
[function_macro(name="fg_ecs_node")]
class FgEcsNodeAnnotation : AstFunctionAnnotation

  def declareReloadCallback(func : FunctionPtr; parsed : FgEcsNodeAnnotationArgs; args : AnnotationArgumentList)
    // To understand declaration syntax, read https://dascript.org/doc/reference/language/reification.html

    var inscope reloadCallback <- qmacro_function("fg_ecs_reload_node`{func.name}") <| $()
      if !is_daframegraph_runtime_initialized()
        logerr("daFG: dafg::startup must be called before loading any das scripts that use daFG!")
        return

      ecs::query() <| $ [ecs::es] ($a(parsed.fnArguments))
        if fg_ecs::is_valid_node_handle($i(func.arguments[0].name))
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
      if [ "REQUIRE", "REQUIRE_NOT"] |> has_value("{arg.name}")
        queryEsArguments |> add_annotation_argument("{arg.name}", "{arg.sValue}")

    reloadCallback |> append_annotation("DeferToAct", "defer_to_act")

    if LOGERR_GENERATED_CODE
      logerr(describe(reloadCallback))
    compiling_module() |> add_function(reloadCallback)

  def declareES(func : FunctionPtr; parsed : FgEcsNodeAnnotationArgs; args : AnnotationArgumentList)

    if !parsed.needDeclareEs
      return

    var inscope fgES <- qmacro_function("fg_es`{func.name}") <| $(evt : Event; $a(parsed.fnArguments))
      if !is_daframegraph_runtime_initialized()
        logerr("daFG: dafg::startup must be called before loading any das scripts that use daFrameGraph!")
        return

      $c(func.name)($a(parsed.callArguments))

    var inscope esArguments <- [
        ("tag", RttiValue(tString="render"))
    ]
    for arg in args
      if FG_ECS_NODE_PASS_ES |> has_value("{arg.name}")
        if arg.basicType == Type.tString
          esArguments |> push(("{arg.name}", RttiValue(tString = "{arg.sValue}")))
        elif arg.basicType == Type.tBool
          esArguments |> push(("{arg.name}", RttiValue(tBool = arg.bValue)))

    fgES |> append_annotation("ecs", "es", esArguments)

    if LOGERR_GENERATED_CODE
      logerr(describe(fgES))
    compiling_module() |> add_function(fgES)

  def parseArgs(func : FunctionPtr; args : AnnotationArgumentList; var errors : das_string) : FgEcsNodeAnnotationArgs

    var needDeclareEs = false

    for arg in args
      if !(FG_ECS_NODE_PASS_ES |> has_value("{arg.name}"))
        errors := "Unexpected parameter '{arg.name}' in [fg_ecs_node] for {func.name}; expected one of {FG_ECS_NODE_PASS_ES}"
        return <- FgEcsNodeAnnotationArgs()

      if !(arg.basicType == Type.tString || arg.basicType == Type.tBool)
        errors := "Unexpected type '{arg.basicType}' for parameter '{arg.name}' in [fg_ecs_node] for {func.name}; expected string or bool"
        return <- FgEcsNodeAnnotationArgs()

      if [ "track", "on_event", "on_appear"] |> has_value("{arg.name}")
        needDeclareEs = true

    let persistentHeapArg = compiling_program()._options |> find_arg("persistent_heap")
    if !(persistentHeapArg is tBool && (persistentHeapArg as tBool))
      errors := "The file {func.at.fileInfo.name} contains [fg_ecs_node] must use persistent heap `options persistent_heap` for correct callback handling!"
      return <- FgEcsNodeAnnotationArgs()

    if func.arguments |> length() == 0
      errors := "Function {func.name} annotated with [fg_ecs_node] expects at least one parameter, but none provided."
      return <- FgEcsNodeAnnotationArgs()

    let firstArgType = describe_typedecl(func.arguments[0]._type, true, true, true)
    if !(firstArgType == "$::NodeHandleVector& -const" || firstArgType == "daFgCore::NodeHandle& -const")
      errors := "Function {func.name} annotated with [fg_ecs_node] expects first parameter '{func.arguments[0].name}' with type 'NodeHandle&' or 'NodeHandleVector&', but '{firstArgType}' provided."
      return <- FgEcsNodeAnnotationArgs()

    var inscope fnArguments : array<VariablePtr>
    var inscope callArguments : array<ExpressionPtr>
    for arg in func.arguments
      if !(arg._type.isConst || arg._type.isRef || arg._type.isPointer)
        errors := "{func.name}: argument 'var {describe(arg)}' is var, but it's change won't affected the actual data. Probably you forgot &"
        return <- FgEcsNodeAnnotationArgs()
      fnArguments |> emplace_new <| clone_variable(arg)
      callArguments |> emplace_new <| qmacro($i(arg.name))

    return <- FgEcsNodeAnnotationArgs(
      ok = true,
      needDeclareEs = needDeclareEs,
      fnArguments <- fnArguments,
      callArguments <- callArguments
    )


  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool

    var inscope parsed <- func |> parseArgs(args, errors)
    if !parsed.ok
      return false

    func |> declareReloadCallback(parsed, args)
    func |> declareES(parsed, args)

    return true
