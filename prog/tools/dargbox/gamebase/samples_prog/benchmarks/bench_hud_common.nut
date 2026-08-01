// Shared HUD simulation: a grid of small indicators (value text + colored bar)
// all updating every frame, through a selectable reactivity tier:
//   "watch"  - Watched per indicator, full component rebuild per change
//   "rtprop" - Behaviors.RtPropUpdate polling, in-place SM_REALTIME_UPDATE
// Used by bench_hud_watch.ui.nut / bench_hud_rtprop.ui.nut.
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const INDICATORS = 100
const GRID_COLS = 10

// shared simulated game state, mutated once per frame by the runner
let values = []
for (local i = 0; i < INDICATORS; i++)
  values.append(i * 7 % 100)

function barColor(v) {
  return v < 25 ? Color(200, 40, 40) : Color(40, 200, 40)
}

function mkIndicatorWatch(i) {
  let w = Watched(values[i])
  return {
    watched = w
    comp = @() {
      watch = w
      key = i
      size = [flex(), flex()]
      flow = FLOW_VERTICAL
      children = [
        {
          rendObj = ROBJ_TEXT
          text = $"{w.get()}"
        }
        {
          rendObj = ROBJ_SOLID
          size = [pw(w.get()), sh(1)]
          color = barColor(w.get())
        }
      ]
    }
  }
}

function mkIndicatorRtProp(i) {
  return {
    watched = null
    comp = {
      key = i
      size = [flex(), flex()]
      flow = FLOW_VERTICAL
      children = [
        {
          rendObj = ROBJ_TEXT
          text = $"{values[i]}"
          behavior = Behaviors.RtPropUpdate
          rtAlwaysUpdate = true
          update = @() { text = $"{values[i]}" }
        }
        {
          rendObj = ROBJ_SOLID
          size = [pw(values[i]), sh(1)]
          color = barColor(values[i])
          behavior = Behaviors.RtPropUpdate
          rtAlwaysUpdate = true
          rtRecalcLayout = true
          update = @() {
            size = [pw(values[i]), sh(1)]
            color = barColor(values[i])
          }
        }
      ]
    }
  }
}

function mkHudBench(mode) {
  let indicators = []
  for (local i = 0; i < INDICATORS; i++)
    indicators.append(mode == "watch" ? mkIndicatorWatch(i) : mkIndicatorRtProp(i))

  mkBenchRunner($"bench_hud_{mode}", {
    onFrame = function(_frame) {
      for (local i = 0; i < INDICATORS; i++) {
        values[i] = (values[i] + 1 + i) % 100
        if (indicators[i].watched != null)
          indicators[i].watched.set(values[i])
      }
    }
  })

  let rows = []
  for (local r = 0; r < INDICATORS / GRID_COLS; r++) {
    let cells = []
    for (local c = 0; c < GRID_COLS; c++)
      cells.append(indicators[r * GRID_COLS + c].comp)
    rows.append({
      size = flex()
      flow = FLOW_HORIZONTAL
      gap = 4
      children = cells
    })
  }

  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = Color(10, 10, 10)
    flow = FLOW_VERTICAL
    gap = 4
    padding = 20
    children = rows
  }
}

return {
  mkHudBench
}
