//-file:forbidden-function

let {DBGLEVEL} = require("dagor.system")
let { setInterval, clearTimer } = require("dagor.workcycle")
let console = require("console")
let log = require("log.nut")()
let { get_time_msec } = require("dagor.time")
let { hardPersistWatched } = require("globalState.nut")

function registerScriptProfiler(prefix, logRes = log.console_print, filePath = null) {
  if (DBGLEVEL <= 0)
    return

  let profiler = require("dagor.profiler")
  let profiler_reset = profiler.reset_values
  let profiler_dump = profiler.dump
  let profiler_get_total_time = profiler.get_total_time
  let isProfileOn = hardPersistWatched($"{prefix}isProfileOn", false)
  let isSpikesProfileOn = hardPersistWatched($"{prefix}isSpikesProfileOn", false)
  let spikesThresholdMs = hardPersistWatched($"{prefix}spikesThresholdMs", 10)
  let frp = require_optional("frp")

  local st = 0
  function toggleProfiler(newVal = null, fileName = null) {
    local ret
    if (newVal == isProfileOn.value)
      ret = "already"
    isProfileOn(newVal ?? !isProfileOn.value)
    if (isProfileOn.value)
      ret = "on"
    else
      ret = "off"
    if (ret=="on"){
      st = get_time_msec()
      logRes(ret)
      profiler.start()
      return ret
    }
    if (ret=="off") {
      let timeTake = get_time_msec() - st
      st = 0
      if (fileName == null)
        profiler.stop()
      else {
        profiler.stop_and_save_to_file(fileName)
        ret = $"{ret} (saved to file {fileName})"
      }
      logRes($"time take: {timeTake}ms")
    }
    logRes(ret)
    return ret
  }
  function profileSpikes(){
    if (profiler_get_total_time() > spikesThresholdMs.value*1000)
      profiler_dump()
    profiler_reset()
  }
  function toggleSpikesProfiler(){
    isSpikesProfileOn(!isSpikesProfileOn.value)
    if (isSpikesProfileOn.value){
      logRes("starting spikes profiler with threshold {0}ms".subst(spikesThresholdMs.value))
      clearTimer(profileSpikes)
      profiler_reset()
      profiler.start()
      setInterval(0.005, profileSpikes)
    }
    else{
      logRes("stopping spikes profiler")
      clearTimer(profileSpikes)
    }
  }
  function setSpikesThreshold(val){
    spikesThresholdMs(val.tofloat())
    logRes("set spikes threshold to {0} ms".subst(spikesThresholdMs.value))
  }

  let fileName = filePath ?? $"../../profile_{prefix}.csv"
  console.register_command(@() toggleProfiler(true), $"{prefix}.profiler.start")
  console.register_command(@() toggleProfiler(false), $"{prefix}.profiler.stop")
  console.register_command(@() toggleProfiler(false, fileName), $"{prefix}.profiler.stopWithFile")
  console.register_command(@() toggleProfiler(null), $"{prefix}.profiler.toggle")
  console.register_command(@() toggleProfiler(null, fileName), $"{prefix}.profiler.toggleWithFile")
  console.register_command(toggleSpikesProfiler, $"{prefix}.profiler.spikes")
  console.register_command(setSpikesThreshold, $"{prefix}.profiler.spikesMsValue")
  console.register_command(@(threshold) profiler.detect_slow_calls(threshold), $"{prefix}.profiler.detect_slow_calls")

  if (frp != null) {
    function frpStats() {
      let stats = frp.gather_graph_stats()
      let names = stats.keys()
      names.sort()
      foreach (n in names) logRes($"{n} = {stats[n]}")
    }

    function recalcAll() {
      frp.recalc_all_computed_values()
      frpStats()
    }

    console.register_command(recalcAll, $"{prefix}.profiler.recalc_all_computed")
    console.register_command(frpStats, $"{prefix}.profiler.gather_frp_stats")
  }
}

return registerScriptProfiler
