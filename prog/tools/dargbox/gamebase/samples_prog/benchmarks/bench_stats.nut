// Shared runner for bench_*.ui.nut scenes: warms up, captures a fixed frame
// window with the daRg profiler enabled, then dumps a single BENCH_RESULT
// json line to the debug log and exits. Parsed by prog/tools/dargbox/run_bench.py.
from "%darg/ui_imports.nut" import *

let { exit } = require("dagor.system")
let { get_time_msec } = require("dagor.time")

// monotonic counters of gui_scene.getPerfStats(): report delta over the window
let counterKeys = [
  "updates", "rebuildBatches", "invalidations", "builderEvals",
  "elemsSetupInitial", "elemsSetupRebuild", "elemsSetupRealtime",
  "elemsCreated", "elemsFreed",
  "layoutFixedSizeRoots", "layoutSizeRoots", "layoutFlowRoots",
  "stacksRebuilds"
]

// point-in-time gauges: report the end-of-window value
let gaugeKeys = ["elemCount", "descTableSlots", "storageTableSlots", "watchRefs", "renderListSize", "sqMemUsedKb"]

function toJson(v) {
  let t = type(v)
  if (t == "integer" || t == "float")
    return v.tostring()
  if (t == "string")
    return $"\"{v}\""
  if (t == "table") {
    let s = ["{"]
    local first = true
    foreach (k, val in v) {
      if (!first)
        s.append(",")
      first = false
      s.append($"\"{k}\":", toJson(val))
    }
    return "".join(s.append("}"))
  }
  return "null"
}

// params: { warmup = 120, capture = 600, onFrame = @(frame_idx) ... }
function mkBenchRunner(name, params = {}) {
  let warmup = params?.warmup ?? 120
  let capture = params?.capture ?? 600
  let onFrame = params?.onFrame
  local frame = 0
  local startStats = null
  local startMs = 0

  gui_scene.setUpdateHandler(function(_dt) {
    frame++
    if (onFrame != null)
      onFrame(frame)

    if (frame == warmup) {
      gui_scene?.enableProfiler(true)
      startStats = gui_scene?.getPerfStats()
      startMs = get_time_msec()
    }
    else if (frame == warmup + capture) {
      let endStats = gui_scene?.getPerfStats()
      let result = {
        name = name
        frames = capture
        elapsedMs = get_time_msec() - startMs
      }
      foreach (k in counterKeys)
        if (k in endStats && k in startStats)
          result[k] <- endStats[k] - startStats[k]
      foreach (k in gaugeKeys)
        if (k in endStats)
          result[k] <- endStats[k]
      if ("metrics" in endStats)
        result.metrics <- endStats.metrics

      println("BENCH_RESULT", toJson(result))
      exit(0)
    }
  })
}

return {
  mkBenchRunner
}
