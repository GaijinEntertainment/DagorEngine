// Bound text on a content-sized column: exercises the layout-affecting
// binding path (text is not paint-only, so every apply relayouts from this
// element's size roots).
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const LABELS = 100

let states = []
for (local i = 0; i < LABELS; i++)
  states.append(Watched($"label {i}"))

mkBenchRunner("bench_bound_text", {
  onFrame = function(frame) {
    for (local i = 0; i < LABELS; i++)
      states[i].set($"label {i} frame {(frame + i) % 97}")
  }
})

let labels = []
for (local i = 0; i < LABELS; i++) {
  labels.append({
    rendObj = ROBJ_TEXT
    bindProps = { text = states[i] }
  })
}

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  padding = 20
  flow = FLOW_VERTICAL
  children = [{
    size = SIZE_TO_CONTENT
    flow = FLOW_VERTICAL
    children = labels
  }]
}
