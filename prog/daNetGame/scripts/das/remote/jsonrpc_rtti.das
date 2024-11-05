options indenting = 2
module jsonrpc_rtti shared

require debugapi
require rtti

require remote.jsonrpc_call

class ScopedRpcCall : RpcCall
  ctx : rtti::Context?


class ContextFunctionRpcCall : ScopedRpcCall
  fn : function<(var params : JsonValue?; id : uint64) : void>
  def ContextFunctionRpcCall(var f : function<(var params : JsonValue?; id : uint64) : void>; var context : Context implicit)
    unsafe
      ctx = addr(context)
    fn <- f

  def override call(var params : JsonValue?; id : uint64) : void
    unsafe
      ctx |> invoke_in_context(fn, params, id)


class ContextFunctionRpcNotification : ScopedRpcCall
  fn : function<(var params : JsonValue?) : void>
  def ContextFunctionRpcNotification(var f : function<(var params : JsonValue?) : void>; var context : Context implicit)
    unsafe
      ctx = addr(context)
    fn <- f

  [unused_argument(id)] def override call(var params : JsonValue ?; id : uint64) : void
    unsafe
      ctx |> invoke_in_context(fn, params)
