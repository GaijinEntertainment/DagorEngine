options indenting = 2
options no_global_variables = false
options persistent_heap
options gc
module server_agent shared

require app
require strings
require debugapi

require daslib/apply_in_context

require remote.jsonrpc_rtti
require remote.jsonrpc_handler
require remote.server

let
  AGENT_NAME = "web_agent"
  initialStrLimit = uint64(2 * 1024 * 1024)

var
  serverDebugLogs = false
  verbose = false
  strLimit = initialStrLimit

// TODO: rewrite using invoke_debug_agent_function
var private [[ignore_heap_usage]] server : WebServer?
var private [[ignore_heap_usage]] jsonRpc : JsonRpcHandler?


def private bytes_hr(value : uint64)
  if value > uint64(1024 * 1024)
    return "{float(value) / (1024f * 1024f)}mb ({int(value)})"
  if value > uint64(1024)
    return "{float(value) / 1024f}kb ({int(value)})"
  return "{int(value)} b"


def private start() : bool
  server = new WebServer()
  server.debugLogs = serverDebugLogs
  server->make_server_adapter()
  jsonRpc = new JsonRpcHandler()
  jsonRpc->listen(server)
  jsonRpc->listenUnhandled(server)
  let success = start_server(server)
  if !success
    unsafe
      delete jsonRpc
  return success


def private stop()
  unsafe
    delete jsonRpc
    delete server


class WebDebugAgent : DapiDebugAgent
  [unused_argument(agent)] def override onInstall(agent : DebugAgent?) : void
    if server == null
      start()
      if verbose
        print("start server on agent {agent} ctx: {this_context()}\n")

  [unused_argument(agent)] def override onUninstall(agent : DebugAgent?) : void
    if agent == thisAgent && server != null
      // server->restart()
      stop()
      if verbose
        print("stop server on agent {agent} ctx: {this_context()}\n")

  def override onDestroyContext(ctx : Context)
    if verbose
      print("onDestroyContext `{ctx}`\n")
    if server == null
      return
    var toRemoveCalls : array<string>
    for name, call in keys(jsonRpc.calls), values(jsonRpc.calls)
      if empty(name)
        stackwalk()
        print("[E] call name is empty")
        debug(call)
        continue
      unsafe
        var scoped = reinterpret<ScopedRpcCall?> (call)
        let currentScope & = *(scoped.ctx)
        if addr(currentScope) == addr(ctx)
          toRemoveCalls |> push(name)

    for it in toRemoveCalls
      if verbose
        print("delete call `{it}`\n")
      jsonRpc.calls |> erase(it)

    var toRemoveNotifications : array<string>
    for name, call in keys(jsonRpc.notifications), values(jsonRpc.notifications)
      if empty(name)
        stackwalk()
        print("[E] notification name is empty")
        debug(call)
        continue
      unsafe
        var scoped = reinterpret<ScopedRpcCall?> (call)
        let currentScope & = *(scoped.ctx)
        if addr(currentScope) == addr(ctx)
          toRemoveNotifications |> push(name)

    for it in toRemoveNotifications
      if verbose
        print("delete notification `{it}`\n")
      jsonRpc.notifications |> erase(it)

    unsafe
      let localCtx : Context const& = this_context()
      if addr(localCtx) == addr(ctx)
        stop()

  def override onTick() : void
    if server != null
      server->tick()

    var alloc = string_heap_bytes_allocated()
    if alloc > strLimit
      let strReport = "STRING HEAP COLLECT: {bytes_hr(alloc)} -> "
      let heapReport = "HEAP COLLECT: {bytes_hr(heap_bytes_allocated())} -> "
      unsafe
        heap_collect(true)
      alloc = string_heap_bytes_allocated()
      if verbose
        print("{strReport}{bytes_hr(alloc)} limit : {bytes_hr(strLimit)}\n{heapReport}{bytes_hr(heap_bytes_allocated())}")
      if alloc > strLimit
        strLimit *= uint64(2)
        if verbose
          print("UP string heap limit: {bytes_hr(strLimit)}\n")
      elif strLimit > initialStrLimit && alloc < strLimit / uint64(2)
        strLimit = strLimit / uint64(2)
        if verbose
          print("DOWN string heap limit: {bytes_hr(strLimit)}\n")


def not_in_web_context()
  unsafe
    if has_debug_agent_context(AGENT_NAME) && addr(get_debug_agent_context(AGENT_NAME))  != addr(this_context())
      return true
  return false


[apply_in_context(web_agent)]
def add_call(name : string implicit; var fn : function<(var params : JsonValue?; id : uint64) : void>; var ctx : Context implicit) : bool
  if verbose
    print("add call `{name}`\n")
  jsonRpc->addCall(clone_string(name), new ContextFunctionRpcCall(fn, ctx))
  return true


[apply_in_context(web_agent)]
def add_notification(name : string implicit; var fn : function<(var params : JsonValue?) : void>; var ctx : Context implicit) : bool
  if verbose
    print("add notification `{name}`\n")
  jsonRpc->addNotification(clone_string(name), new ContextFunctionRpcNotification(fn, ctx))
  return true


[apply_in_context(web_agent)]
def add_error_handler(var fn : function<(var params : JsonValue?; id : uint64) : void>; var ctx : Context implicit) : bool
  if verbose
    print("add error handler\n")
  jsonRpc->addErrorHandler(new ContextFunctionRpcCall(fn, ctx))
  return true


[apply_in_context(web_agent)]
def send_response(var data : JsonValue? implicit; id : uint64) : bool
  jsonRpc->sendResponseStr(server, write_json(data), id)
  unsafe
    if addr(get_debug_agent_context(AGENT_NAME)) != addr(this_context())
      delete data
  return true


[apply_in_context(web_agent)]
def send_request_typed(method : string implicit; var data : JsonValue? implicit; var fn : function<(var params : JsonValue?; id : uint64) : void>; var ctx : Context implicit) : bool
  jsonRpc->sendRequestStr(server, clone_string(method), write_json(data), new ContextFunctionRpcCall(fn, ctx))
  unsafe
    if addr(get_debug_agent_context(AGENT_NAME)) != addr(this_context())
      delete data
  return true


def send_request_typed(method : string; var params : JsonValue?; var fn : function<(var params : JsonValue?; id : uint64) : void>) : bool
  return send_request_typed(method, params, fn, this_context())


[apply_in_context(web_agent)]
def send_notification(method : string implicit; var data : JsonValue? implicit) : bool
  jsonRpc->sendNotificationStr(server, clone_string(method), write_json(data))
  unsafe
    if addr(get_debug_agent_context(AGENT_NAME)) != addr(this_context())
      delete data
  return true


[apply_in_context(web_agent)]
def send_error(error : string implicit; code : double; data : string implicit; id : uint64) : bool
  jsonRpc->sendError(server, clone_string(error), code, empty(data) ? null : JV(data), id)
  return true


[export]
def has_web_agent() : bool
  return has_debug_agent_context(AGENT_NAME)


[export, unused_argument(ctx)]
def private start_debug_agent(ctx : Context)
  install_new_debug_agent(new WebDebugAgent(), AGENT_NAME)


[export]
def start_agent() : bool
  unsafe
    lock_debug_agent() <| $()
      if app::get_game_name() != "aot" && !has_web_agent() && !is_in_aot() && !is_in_debug_agent_creation()
        fork_debug_agent_context(@@start_debug_agent)
      elif verbose
        print("debug agent already exists\n")
  return true


[export]
def tick_agent()
  tick_debug_agent(AGENT_NAME)
