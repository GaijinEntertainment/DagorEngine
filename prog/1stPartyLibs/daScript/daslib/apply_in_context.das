options indenting = 2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers

module apply_in_context shared private

require ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/defer
require daslib/strings_boost

require rtti public
require debugapi public

[function_macro(name="apply_in_context")]
class AppendCondAnnotation : AstFunctionAnnotation
  //! [apply_in_context] function annotation.
  //! Function is modified, so that it is called in the debug agent context, specified in the annotation.
  //! If specified context is not insalled, panic is called.
  //!
  //! For example::
  //!  [apply_in_context(opengl_cache)]
  //!  def public cache_font(name:string implicit) : Font?
  //!      ...
  //!  ...
  //!  let font = cache_font("Arial") // call invoked in the "opengl_cache" debug agent context
  def override patch ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string; var astChanged:bool& ) : bool

    for ann in func.annotations
      if ann.annotation.name=="apply_in_context"
        if !(find_arg(ann.arguments,"patched") is nothing)
          return true

    if args |> length != 1
      errors := "expecting one argument"
      return false

    var contextName = ""
    for argv in args
      let val = get_annotation_argument_value(argv)
      if val is tBool
        contextName = "{argv.name}"
      elif val is tString
        contextName = val as tString
      else
        errors := "invalid argument type {argv.name}"
        return false

    for arg in func.arguments
      if is_temp_type(arg._type,true) && !(arg._type.flags.temporary || arg._type.flags._implicit)
        errors := "argument {arg.name} needs to be temporary or implicit, i.e. {describe(arg._type)}# or {describe(arg._type)} implicit"
        return false

    for ann in func.annotations
      if ann.annotation.name=="apply_in_context"
        astChanged = true
        ann.arguments |> add_annotation_argument("patched",true)

    let resName = "__res__"
    let ctxCloneFnName = "CONTEXT_CLONE`{func.name}"
    let ctxFnName = "CONTEXT`{func.name}"
    var inscope ctxFnArguments : array<VariablePtr>
    var inscope ctxFnBlockList : array<ExpressionPtr>
    var inscope callInCtxArguments : array<ExpressionPtr>
    for arg in func.arguments
      ctxFnArguments |> emplace_new <| clone_variable(arg)
      callInCtxArguments |> emplace_new <| qmacro($i(arg.name))
    if !func.result.isVoid
      ctxFnArguments |> emplace_new <| qmacro_variable(resName,type<$t(func.result)?>)
      ctxFnBlockList |> emplace_new <| qmacro_expr(${*$i(resName) = $c(ctxCloneFnName)($a(callInCtxArguments));})
    else
      ctxFnBlockList |> emplace_new <| qmacro($c(ctxCloneFnName)($a(callInCtxArguments)))
    var inscope ctxFn <- qmacro_function(ctxFnName) <| $ ( $a(ctxFnArguments) ) : void
      unsafe
        verify(has_debug_agent_context($v(contextName)),"debug agent is not installed")
        verify(addr(get_debug_agent_context($v(contextName))) == addr(this_context()),"agent context mismatch")
        $b(ctxFnBlockList)
    ctxFn.moreFlags |= MoreFunctionFlags pinvoke
    compiling_module() |> add_function(ctxFn)

    var inscope fn <- clone_function(func)
    fn.name := ctxCloneFnName
    fn.flags |= FunctionFlags generated | FunctionFlags privateFunction | FunctionFlags exports
    func.body := null
    var inscope funcBlocklist : array<ExpressionPtr>
    if !func.result.isVoid
      funcBlocklist |> emplace_new <| qmacro_expr(${let $i(resName) : $t(func.result);})
    var inscope pinvokeArguments : array<ExpressionPtr>
    for arg in func.arguments
      if arg._type.isRef && !arg._type.isRefType
        pinvokeArguments |> emplace_new <| qmacro(unsafe(addr($i(arg.name))))
      else
        pinvokeArguments |> emplace_new <| qmacro($i(arg.name))
    if !func.result.isVoid
      pinvokeArguments |> emplace_new <| qmacro(unsafe(addr($i(resName))))
    funcBlocklist |> emplace_new <| qmacro(unsafe(invoke_in_context(get_debug_agent_context($v(contextName)),@@$c(ctxFnName),$a(pinvokeArguments))))
    if !func.result.isVoid
      funcBlocklist |> emplace_new() <| qmacro_expr(${return $i(resName);})
    unsafe
      func.body <- qmacro_block() <|
        unsafe
          verify(has_debug_agent_context($v(contextName)),"debug agent is not installed")
          verify(addr(get_debug_agent_context($v(contextName))) != addr(this_context()),"agent context mismatch")
          $b(funcBlocklist)
    compiling_module() |> add_function(fn)
    return true

