options no_aot
options persistent_heap

require ecs
require app
require daslib/debug
require debugapi
require strings
require daslib/strings_boost
require DagorSystem


def relative_path(file, prefix, replace : string; var res : string&) : bool
  let prefixIdx = file |> find(prefix)
  if prefixIdx < 0
    return false
  res = "{replace}{slice(file, prefixIdx + length(prefix))}"
  return true


def relative_path(file : string) : string
  var res : string
  if relative_path(file, get_game_name(), "..", res)
    return res
  if relative_path(file, "daNetGameLibs", "../../prog/daNetGameLibs", res)
    return res
  return file


class BreakpointsHookAgent : DapiDebugAgent

  queue : table<string; bool>

  def override onBreakpointsReset(file : string; breakpointsNum : int) : void
    if file |> ends_with(".das")
      queue[clone_string(file)] = breakpointsNum > 0

  def override onTick()
    for file, addDebugger in keys(queue), values(queue)
      let unixPath = relative_path(file |> replace("\\", "/"))
      let reloaded = reload_das_debug(unixPath, addDebugger)
      if reloaded
        print("dap: {unixPath} has {addDebugger ? "attached" : "detached"} debugger")
      else
        logerr("dap: unable to reload '{unixPath}'. See log for more information")
    queue |> clear()


[export, unused_argument(ctx)]
def install_breakpoints_hook(ctx : Context)
  install_new_debug_agent(new BreakpointsHookAgent(), "breakpointsHook")


[init, export]
def private auto_start_breakpoints_hook()
  if !is_in_debug_agent_creation() && !has_debug_agent_context("breakpointsHook")
    fork_debug_agent_context(@@install_breakpoints_hook)
