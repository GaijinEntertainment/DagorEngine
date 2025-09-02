options indenting = 2
module server_agent_boost shared

require strings

require daslib/ast_boost
require daslib/json

let
  genFnPrefix = "`gen`"


[export, tag_function(send_request_fn), unused_argument(method, params, fn, ctx)]
def send_request(method : string; var params : JsonValue?; var fn : function; var ctx : Context) : void {}


[tag_function(send_request_fn), unused_argument(method, params, fn)]
def send_request(method : string; var params : JsonValue?; var fn : function) : void {}


[tag_function_macro(tag="send_request_fn")]
class SendRequestFn : AstFunctionAnnotation
  [unused_argument(errors)] def override transform(var expr : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr
    var inscope exprClone <- clone_expression(expr)
    var inscope newCall <- unsafe(reinterpret<smart_ptr<ExprCallFunc>> exprClone)
    newCall.name := "{newCall.name}_typed"
    var success = false
    for arg in newCall.arguments
      if arg is ExprAddr
        unsafe
          var exprAddr = reinterpret<ExprAddr?> arg
          if !string(exprAddr.target) |> starts_with(genFnPrefix)
            let newName = "{genFnPrefix}{exprAddr.target}"
            var fn <- compiling_module() |> find_unique_function(newName)
            if fn == null
              compiling_program() |> macro_error(newCall.at, "can't find <{newName}> function, annotate function\n[rpc_handler]\ndef {exprAddr.target}...")
              return default<ExpressionPtr>
            else
              exprAddr.target := newName
              success = true
    if success
      return <- newCall
    return <- default<ExpressionPtr>
