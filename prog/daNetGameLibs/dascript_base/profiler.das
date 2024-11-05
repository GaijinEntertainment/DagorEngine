module profiler

require ecs
require app
require daProfiler
require debugapi

// USAGE:
// 1. add `require %danetlibs.dascript_base.profiler` line to any dascript file
// 2. open daProfiler and look for functions that starts with "das:"
// NOTE: use hotreload to profile only required module,
//       otherwise will be profiled all scripts loaded after profiler

let
  PROFILER_AGENT_NAME = "profiler_agent"
  debugLogs = false

class ProfilerAgent : DapiDebugAgent

  def override onSimulateContext(var ctx : Context) : void
    if ctx.category.debug_context || ctx.category.debugger_tick || ctx.category.macro_context
      return
    let ctxName = "{ctx.name}"
    ctx |> da_profiler_instrument_all_functions() <| $ [unused_argument(fn)] (fn, sfn)
      let desc = get_tls_description(ctxName, /*line*/0, /*flags*/0u, "das:{sfn.name}", uint(sfn.mangledNameHash))
      if debugLogs
        print("daProfiler: {desc} => {sfn.name}")
      return uint64(desc)


[export, unused_argument(ctx)]
def profiler_agent(ctx : Context)
  install_new_debug_agent(new ProfilerAgent(), PROFILER_AGENT_NAME)


[init, export]
def init()
  if app::get_game_name() != "aot" && !has_debug_agent_context(PROFILER_AGENT_NAME) && !is_in_aot() && !is_in_debug_agent_creation()
    fork_debug_agent_context(@@profiler_agent)
