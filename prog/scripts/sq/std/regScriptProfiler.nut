//-file:forbidden-function

let {DBGLEVEL} = require("dagor.system")
let { setInterval, clearTimer } = require("dagor.workcycle")
let console = require("console")
let log = require("log.nut")()
let conprint = log.console_print
let { get_time_msec } = require("dagor.time")
let { hardPersistWatched } = require("globalState.nut")

function registerScriptProfiler(prefix) {
  if (DBGLEVEL > 0) {
    let profiler = require("dagor.profiler")
    let profiler_reset = profiler.reset_values
    let profiler_dump = profiler.dump
    let profiler_get_total_time = profiler.get_total_time
    let isProfileOn = hardPersistWatched($"{prefix}isProfileOn", false)
    let isSpikesProfileOn = hardPersistWatched($"{prefix}isSpikesProfileOn", false)
    let spikesThresholdMs = hardPersistWatched($"{prefix}spikesThresholdMs", 10)

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
        conprint(ret)
        profiler.start()
        return ret
      }
      if (ret=="off") {
        conprint($"time take: {get_time_msec() - st}ms")
        st = 0
        if (fileName == null)
          profiler.stop()
        else {
          profiler.stop_and_save_to_file(fileName)
          ret = $"{ret} (saved to file {fileName})"
        }
      }
      conprint(ret)
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
        conprint("starting spikes profiler with threshold {0}ms".subst(spikesThresholdMs.value))
        clearTimer(profileSpikes)
        profiler_reset()
        profiler.start()
        setInterval(0.005, profileSpikes)
      }
      else{
        conprint("stopping spikes profiler")
        clearTimer(profileSpikes)
      }
    }
    function setSpikesThreshold(val){
      spikesThresholdMs(val.tofloat())
      conprint("set spikes threshold to {0} ms".subst(spikesThresholdMs.value))
    }

    let fileName = $"profile_{prefix}.csv"
    console.register_command(@() toggleProfiler(true), $"{prefix}.profiler.start")
    console.register_command(@() toggleProfiler(false), $"{prefix}.profiler.stop")
    console.register_command(@() toggleProfiler(false, fileName), $"{prefix}.profiler.stopWithFile")
    console.register_command(@() toggleProfiler(null), $"{prefix}.profiler.toggle")
    console.register_command(@() toggleProfiler(null, fileName), $"{prefix}.profiler.toggleWithFile")
    console.register_command(toggleSpikesProfiler, $"{prefix}.profiler.spikes")
    console.register_command(setSpikesThreshold, $"{prefix}.profiler.spikesMsValue")
    console.register_command(@(threshold) profiler.detect_slow_calls(threshold), $"{prefix}.profiler.detect_slow_calls")
  }
}

return registerScriptProfiler
