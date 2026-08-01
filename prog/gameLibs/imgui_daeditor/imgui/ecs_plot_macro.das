options no_aot = true
options rtti
module ecs_plot_macro shared private

require ecs.macro_common
require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost

require gameLibs.imgui_daeditor.imgui.ecs_plot_registry_common public

[macro_function]
def private keep_func_and_args(var func)
  func.flags |= FunctionFlags.exports
  for arg in func.arguments
    arg.flags |= VariableFlags.marked_used


[function_macro(name="plot_sample")]
class PlotSampleAnnotation : AstFunctionAnnotation
  [unused_argument(group, progArgs)] def override patch(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool &) : bool
    for ann in func.annotations
      if ann.annotation.name == "plot_sample"
        if !(find_arg(ann.arguments, "patched") is nothing)
          return true

    let typeNameArg = find_arg(args, "type_name")
    if !validate_string_argument(typeNameArg, "type_name", errors)
      return false

    for ann in func.annotations
      if ann.annotation.name == "plot_sample"
        astChanged = true
        ann.arguments |> add_annotation_argument("patched", true)

    keep_func_and_args(func)

    let at = func.at
    var fn = new Function(at = at, atDecl = at, name := "register`{func.name}")
    fn.flags |= FunctionFlags.generated
    fn.flags |= FunctionFlags.privateFunction
    fn.flags |= FunctionFlags.init
    fn.flags |= FunctionFlags.exports
    fn.result = new TypeDecl(baseType = Type.autoinfer, at = at)

    var qblock = quote() <|
      add_plot_sampler(type_name, func)

    var rules : Template
    rules |> replaceVariable("type_name") <| new ExprConstString(at = at, value := "{typeNameArg}")
    rules |> replaceVariable("func")      <| new ExprAddr(at = at, target := "{func.name}")
    apply_template(rules, at, qblock)
    var blk = move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags.isClosure
    fn.body = blk

    compiling_module() |> add_function(fn)
    return true


[function_macro(name="plot_set")]
class PlotSetAnnotation : AstFunctionAnnotation
  [unused_argument(group, progArgs)] def override patch(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool &) : bool
    for ann in func.annotations
      if ann.annotation.name == "plot_set"
        if !(find_arg(ann.arguments, "patched") is nothing)
          return true

    let typeNameArg = find_arg(args, "type_name")
    if !validate_string_argument(typeNameArg, "type_name", errors)
      return false

    for ann in func.annotations
      if ann.annotation.name == "plot_set"
        astChanged = true
        ann.arguments |> add_annotation_argument("patched", true)

    keep_func_and_args(func)

    let at = func.at
    var fn = new Function(at = at, atDecl = at, name := "register`{func.name}")
    fn.flags |= FunctionFlags.generated
    fn.flags |= FunctionFlags.privateFunction
    fn.flags |= FunctionFlags.init
    fn.flags |= FunctionFlags.exports
    fn.result = new TypeDecl(baseType = Type.autoinfer, at = at)

    var qblock = quote() <|
      add_plot_setter(type_name, func)

    var rules : Template
    rules |> replaceVariable("type_name") <| new ExprConstString(at = at, value := "{typeNameArg}")
    rules |> replaceVariable("func")      <| new ExprAddr(at = at, target := "{func.name}")
    apply_template(rules, at, qblock)
    var blk = move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags.isClosure
    fn.body = blk

    compiling_module() |> add_function(fn)
    return true
