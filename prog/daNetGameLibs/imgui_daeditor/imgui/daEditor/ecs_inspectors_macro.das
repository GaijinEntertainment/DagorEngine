
options no_aot = true
options rtti
module ecs_inspectors_macro shared private
require ecs.macro_common
require ast

require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost

require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_common public
require debugapi public

[macro_function]
def keep_args(var func)
  for arg in func.arguments
    arg.flags |= VariableFlags.marked_used

[macro_function]
def keep_func_and_args(var func)
  func.flags |= FunctionFlags.exports
  keep_args(func)


[function_macro(name="ecs_inspector")]
class EcsInspectorAnnotation : AstFunctionAnnotation
  [unused_argument(group, progArgs)] def override patch(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool &) : bool
    for ann in func.annotations
      if ann.annotation.name == "ecs_inspector"
        if !(find_arg(ann.arguments, "patched") is nothing)
          return true

    let typeNameArg = find_arg(args, "type_name")
    if !validate_string_argument(typeNameArg, "type_name", errors)
      return false

    let fieldName = find_arg(args, "field_name")
    if !(fieldName is nothing) && !validate_string_argument(fieldName, "field_name", errors)
      return false

    let tagArg = find_arg(args, "tag")
    if !(tagArg is nothing) && !validate_string_argument(tagArg, "tag", errors)
      return false

    keep_func_and_args(func)

    for ann in func.annotations
      if ann.annotation.name == "ecs_inspector"
        astChanged = true
        ann.arguments |> add_annotation_argument("patched", true)

    let at = func.at
    var inscope fn <- new Function(at = at, atDecl = at, name := "register`{func.name}")
    fn.flags |= FunctionFlags.generated
    fn.flags |= FunctionFlags.privateFunction
    fn.flags |= FunctionFlags.init
    fn.flags |= FunctionFlags.exports
    fn.result |> move_new() <| new TypeDecl(baseType = Type.autoinfer, at = at)

    var inscope qblock <- quote() <|
      unsafe
        lock_debug_agent() <| $()
          start_ecs_inspectors_agent()
          if not_in_ecs_inspectors_context() && !is_in_aot()
            add_inspector(type_name, field_name, tag, func, this_context())

    var inscope rules : Template
    rules |> replaceVariable("type_name") <| new ExprConstString(at = at, value := "{typeNameArg}")
    rules |> replaceVariable("field_name") <| new ExprConstString(at = at, value := "{fieldName}")
    rules |> replaceVariable("tag") <| new ExprConstString(at = at, value := "{tagArg}")
    rules |> replaceVariable("func") <| new ExprAddr(at = at, target := "{func.name}")
    apply_template(rules, at, qblock)
    delete rules
    var inscope blk <- move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags.isClosure
    fn.body |> move() <| blk

    compiling_module() |> add_function(fn)
    return true



[function_macro(name="ecs_editor")]
class EcsEditorAnnotation : AstFunctionAnnotation
  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    var inscope tags : array<string>
    for arg in args
      if string(arg.name) == "tags"
        if arg.basicType != Type.tString
          errors := "tags should be string value"
          return false
        tags |> push("{arg.sValue}")

    let prefixArg = args |> find_arg("prefix")
    if !(prefixArg is nothing) && !validate_bool_argument(prefixArg, "prefix", errors)
      return false
    let prefix = (prefixArg is nothing) || (prefixArg as tBool)

    keep_func_and_args(func)

    let at = func.at
    var inscope fn <- new Function(at = at, atDecl = at, name := "register`{func.name}")
    fn.flags |= FunctionFlags.generated
    fn.flags |= FunctionFlags.privateFunction
    fn.flags |= FunctionFlags.init
    fn.flags |= FunctionFlags.exports
    fn.result |> move_new() <| new TypeDecl(baseType = Type.autoinfer, at = at)

    var inscope qblock <- quote() <|
      unsafe
        lock_debug_agent() <| $()
          start_ecs_inspectors_agent()
          if not_in_ecs_inspectors_context() && !is_in_aot()
            var tempTags <- tags
            add_editor(prefix, tempTags, func, this_context())
            delete tempTags

    var inscope rules : Template
    rules |> replaceVariable("prefix") <| new ExprConstBool(at = at, value = prefix)
    var inscope tagsArr <- new ExprMakeArray(at = at, makeType <- typeinfo ast_typedecl(type<string>))
    for tag in tags
      if true
        var inscope expr <- new ExprConstString(at = at, value := tag)
        tagsArr.values |> emplace <| expr
    var inscope tagsArrMove <- new ExprCall(at = at, name := "to_array_move")
    tagsArrMove.arguments |> emplace <| tagsArr

    rules |> replaceVariable("tags") <| add_ptr_ref(tagsArrMove)
    rules |> replaceVariable("func") <| new ExprAddr(at = at, target := "{func.name}")
    apply_template(rules, at, qblock)
    delete rules
    var inscope blk <- move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags.isClosure
    fn.body |> move <| blk

    compiling_module() |> add_function(fn)
    return true



[function_macro(name="ecs_unselected_inspector")]
class EcsUnselectedInspectorAnnotation : AstFunctionAnnotation
  [unused_argument(group, args, errors)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    keep_func_and_args(func)

    let at = func.at
    var inscope fn <- new Function(at = at, atDecl = at, name := "register`{func.name}")
    fn.flags |= FunctionFlags.generated
    fn.flags |= FunctionFlags.privateFunction
    fn.flags |= FunctionFlags.init
    fn.flags |= FunctionFlags.exports
    fn.result |> move_new() <| new TypeDecl(baseType = Type.autoinfer, at = at)

    var inscope qblock <- quote() <|
      unsafe
        lock_debug_agent() <| $()
          start_ecs_inspectors_agent()
          if not_in_ecs_inspectors_context() && !is_in_aot()
            add_unselected_inspector(func, this_context())

    var inscope rules : Template
    rules |> replaceVariable("func") <| new ExprAddr(at = at, target := "{func.name}")
    apply_template(rules, at, qblock)
    delete rules
    var inscope blk <- move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags.isClosure
    fn.body |> move <| blk

    compiling_module() |> add_function(fn)
    return true