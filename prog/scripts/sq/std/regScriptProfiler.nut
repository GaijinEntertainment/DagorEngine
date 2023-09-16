let dagor_sys = require("dagor.system")
let { setInterval, clearTimer } = require("dagor.workcycle")
let {Watched} = require("frp")
let console = require("console")
let log = require("log.nut")()
let conprint = log.console_print

let mkWatched = @(id, val) persist(id, @() Watched(val))

let function registerScriptProfiler(prefix) {
  if (dagor_sys.DBGLEVEL > 0) {
    let profiler = require("dagor.profiler")
    let profiler_reset = profiler.reset_values
    let profiler_dump = profiler.dump
    let profiler_get_total_time = profiler.get_total_time
    let isProfileOn = mkWatched("isProfileOn", false)
    let isSpikesProfileOn = mkWatched("isSpikesProfileOn", false)
    let spikesThresholdMs = mkWatched("spikesThresholdMs", 10)

    let function toggleProfiler(newVal = null, fileName = null) {
      local ret
      if (newVal == isProfileOn.value)
        ret = "already"
      isProfileOn(newVal ?? !isProfileOn.value)
      if (isProfileOn.value)
        ret = "on"
      else
        ret = "off"
      if (ret=="on"){
        conprint(ret)
        profiler.start()
        return ret
      }
      if (ret=="off")
        if (fileName == null)
          profiler.stop()
        else {
          profiler.stop_and_save_to_file(fileName)
          ret = $"{ret} (saved to file {fileName})"
        }
      conprint(ret)
      return ret
    }
    let function profileSpikes(){
      if (profiler_get_total_time() > spikesThresholdMs.value*1000)
        profiler_dump()
      profiler_reset()
    }
    let function toggleSpikesProfiler(){
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
    let function setSpikesThreshold(val){
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
