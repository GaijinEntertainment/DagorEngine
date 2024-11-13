options indenting = 2
module jsonrpc_boost shared
options strict_smart_pointers

require ast
require strings
require debugapi public

require daslib/ast_boost
require daslib/templates_boost

require remote.server_agent public
require remote.server_agent_boost public
require remote.jsonrpc_handler public
require remote.jsonrpc_args public


[macro_function]
def private keep_args(var func)
  for arg in func.arguments
    arg.flags |= VariableFlags marked_used


[macro_function]
def private keep_func_and_args(var func)
  func.flags |= FunctionFlags exports
  keep_args(func)


[macro_function]
def private add_call_reg_func(at; call_fn_name; call_fn_alias)
  let fn_name = "register_{call_fn_name}_{call_fn_alias}"
  var inscope fn <- new [[Function() at = at, atDecl = at, name := fn_name]]
  fn.flags |= FunctionFlags generated
  fn.flags |= FunctionFlags privateFunction
  fn.flags |= FunctionFlags init
  fn.result |> move_new() <| new [[TypeDecl() baseType=Type autoinfer, at=at]]

  var inscope qblock <- quote() <|
    unsafe
      lock_debug_agent() <| $()
        start_agent()
        if not_in_web_context()
          add_call(func_name, func, this_context())

  var inscope rules : Template
  rules |> replaceVariable("func_name") <| new [[ExprConstString() at = at, value := call_fn_alias]]
  rules |> replaceVariable("func") <| new [[ExprAddr() at = at, target := call_fn_name, funcType <- typeinfo(ast_typedecl type<function<(var params : JsonValue?; id : uint64) : void>>)]]
  apply_template(rules, at, qblock)
  delete rules
  unsafe
    var blk <- move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags isClosure
    fn.body <- blk

  compiling_module() |> add_function(fn)


[macro_function]
def private add_notification_reg_func(at; call_fn_name; call_fn_alias)
  let fn_name = "register_{call_fn_name}_{call_fn_alias}"
  var inscope fn <- new [[Function() at = at, atDecl = at, name := fn_name]]
  fn.flags |= FunctionFlags generated
  fn.flags |= FunctionFlags privateFunction
  fn.flags |= FunctionFlags init
  fn.result |> move_new() <| new [[TypeDecl() baseType=Type autoinfer, at=at]]

  var inscope qblock <- quote() <|
    unsafe
      lock_debug_agent() <| $()
        start_agent()
        if not_in_web_context()
          add_notification(func_name, func, this_context())

  var inscope rules : Template
  rules |> replaceVariable("func_name") <| new [[ExprConstString() at = at, value := call_fn_alias]]
  rules |> replaceVariable("func") <| new [[ExprAddr() at = at, target := call_fn_name, funcType <- typeinfo(ast_typedecl type<function<(var params : JsonValue?) : void>>)]]
  apply_template(rules, at, qblock)
  delete rules
  unsafe
    var blk <- move_unquote_block(qblock)
    blk.blockFlags ^= ExprBlockFlags isClosure
    fn.body <- blk

  compiling_module() |> add_function(fn)


[macro_function]
def private transform_args(var func; is_call; var errors : das_string) : bool
  var inscope fn <- clone_function(func)
  fn.annotations |> clear()
  fn.name := "{genFnPrefix}{fn.name}"
  fn.flags |= FunctionFlags generated
  let at = fn.at
  var blk : ExprBlock?
  unsafe
    blk = reinterpret<ExprBlock?> fn.body

  let paramsName = "__params__"
  let argsNum = length(fn.arguments)
  let processArgsNum = argsNum - (is_call ? 1 : 0)
  for i in range(processArgsNum)
    if true
      var inscope argi <- new [[ExprCall() at = at, name := "get_arg"]]
      argi.arguments |> emplace_new(new [[ExprVar() at = at, name := paramsName]])
      argi.arguments |> emplace_new(new [[ExprConstInt() at = at, value = i]])

      assume baseArg = fn.arguments[i]
      assume baseArgType = baseArg._type
      if baseArgType.baseType == Type tInt
        var inscope toValue <- new [[ExprCall() at = at, name := "as_int"]]
        toValue.arguments |> emplace(argi)
        argi |> move() <| toValue
      elif baseArgType.baseType == Type tUInt
        var inscope toValue <- new [[ExprCall() at = at, name := "as_uint"]]
        toValue.arguments |> emplace(argi)
        argi |> move() <| toValue
      elif baseArgType.baseType == Type tFloat
        var inscope toValue <- new [[ExprCall() at = at, name := "as_float"]]
        toValue.arguments |> emplace(argi)
        argi |> move() <| toValue
      elif baseArgType.baseType == Type tDouble
        var inscope toValue <- new [[ExprCall() at = at, name := "as_double"]]
        toValue.arguments |> emplace(argi)
        argi |> move() <| toValue
      elif baseArgType.baseType == Type tBool
        var inscope toValue <- new [[ExprCall() at = at, name := "as_bool"]]
        toValue.arguments |> emplace(argi)
        argi |> move() <| toValue
      elif baseArgType.baseType == Type tString
        var inscope toValue <- new [[ExprCall() at = at, name := "as_string"]]
        toValue.arguments |> emplace(argi)
        argi |> move() <| toValue
      elif (baseArgType.baseType == Type tPointer && baseArgType.firstType != null && baseArgType.firstType.baseType == Type tStructure
            && string(baseArgType.firstType.structType.name) == "JsonValue" && string(baseArgType.firstType.structType._module.name) == "json")
        pass // JsonValue
      else
        errors := "{baseArg.name}: {describe(baseArgType)} - unsupported argument type"
        return false

      var inscope vlet <- new [[ExprLet() at = at, atInit = at]]
      vlet.variables |> emplace_new() <| new [[Variable() at = at,
              name := baseArg.name,
              _type := baseArgType,
              init <- argi
          ]]
      blk.list |> emplace(vlet, i)

  for _ in range(processArgsNum) // cleanup all arguments
    fn.arguments |> erase(0)

  var inscope paramsArg <- new [[Variable()
    at = at,
    name := paramsName,
    _type <- typeinfo(ast_typedecl type<json::JsonValue? -const>)
  ]]
  fn.arguments |> emplace(paramsArg, 0)

  keep_func_and_args(fn)
  compiling_module() |> add_function(fn)
  return true


[macro_function]
def private validate_name_annotation(name_arg; var errors : das_string) : bool
  if name_arg is nothing
    errors := "name is not defined"
    return false
  if !(name_arg is tString)
    errors := "name should be string value"
    return false
  if empty(name_arg as tString)
    errors := "name string is empty"
    return false
  return true


[function_macro(name="rpc_call_raw")]
class RpcCallRawAnnotation : AstFunctionAnnotation
  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    let nameArg = find_arg(args, "name")
    if !validate_name_annotation(nameArg, errors)
      return false

    keep_func_and_args(func)
    add_call_reg_func(func.at, func.name, nameArg as tString)
    return true


[function_macro(name="rpc_notification_raw")]
class RpcNotificationRawAnnotation : AstFunctionAnnotation
  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    let nameArg = find_arg(args, "name")
    if !validate_name_annotation(nameArg, errors)
      return false

    keep_func_and_args(func)
    add_notification_reg_func(func.at, func.name, nameArg as tString)
    return true


[macro_function]
def private validate_req_id_arg(arguments; var errors : das_string)
  let argsNum = length(arguments)
  if argsNum < 1
    errors := "at least one argument with request id is required (req_id: uint64)"
    return false
  if arguments[argsNum - 1]._type.baseType != Type tUInt64
    errors := "the last argument should be of uint64 type (req_id: uint64)"
    return false
  return true


[function_macro(name="rpc_call")]
class RpcCallAnnotation : AstFunctionAnnotation
  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    let nameArg = find_arg(args, "name")
    if !validate_name_annotation(nameArg, errors)
      return false
    if !validate_req_id_arg(func.arguments, errors)
      return false
    if !transform_args(func, /*is_call*/true, errors)
      return false
    keep_args(func)
    add_call_reg_func(func.at, "{genFnPrefix}{func.name}", nameArg as tString)
    return true


[function_macro(name="rpc_notification")]
class RpcNotificationAnnotation : AstFunctionAnnotation
  [unused_argument(group)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    let nameArg = find_arg(args, "name")
    if !validate_name_annotation(nameArg, errors)
      return false
    if !transform_args(func, /*is_call*/false, errors)
      return false
    keep_args(func)
    add_notification_reg_func(func.at, "{genFnPrefix}{func.name}", nameArg as tString)
    return true


[function_macro(name="rpc_handler")]
class RpcHandlerAnnotation : AstFunctionAnnotation
  [unused_argument(group, args)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    if !validate_req_id_arg(func.arguments, errors)
      return false
    if !transform_args(func, /*is_call*/true, errors)
      return false
    keep_args(func)
    return true
