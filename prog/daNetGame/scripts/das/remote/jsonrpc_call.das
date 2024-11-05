options indenting = 2
module jsonrpc_call shared

require daslib/json public


class RpcCall
  def abstract call(var params : JsonValue?; id : uint64) : void

class LambdaRpcCall : RpcCall
  fn : lambda<(var params : JsonValue?; id : uint64) : void>
  def LambdaRpcCall(var f : lambda<(var params : JsonValue?; id : uint64) : void>)
    fn <- f

  def override call(var params : JsonValue?; id : uint64) : void
    invoke(fn, params, id)


class LambdaRpcNotification : RpcCall
  fn : lambda<(var params : JsonValue?) : void>
  def LambdaRpcNotification(var f : lambda<(var params : JsonValue?) : void>)
    fn <- f

  [unused_argument(id)] def override call(var params : JsonValue ?; id : uint64) : void
    invoke(fn, params)


class FunctionRpcCall : RpcCall
  fn : function<(var params : JsonValue?; id : uint64) : void>
  def FunctionRpcCall(var f : function<(var params : JsonValue?; id : uint64) : void>)
    fn <- f

  def override call(var params : JsonValue?; id : uint64) : void
    invoke(fn, params, id)


class FunctionRpcNotification : RpcCall
  fn : function<(var params : JsonValue?) : void>
  def FunctionRpcNotification(var f : function<(var params : JsonValue?) : void>)
    fn <- f

  [unused_argument(id)] def override call(var params : JsonValue ?; id : uint64) : void
    invoke(fn, params)
