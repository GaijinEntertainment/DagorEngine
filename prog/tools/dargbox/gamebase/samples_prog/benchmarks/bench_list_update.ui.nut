// Keyed list with per-row observable text: N rows, K rows change per frame
// through the classic watch + rebuild path. This is the primary target of the
// per-field binding work (doc 04 Track A): each text change today re-runs the
// row builder and re-reads all row properties.
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const ROWS = 1000
const UPDATES_PER_FRAME = 20

let states = []
for (local i = 0; i < ROWS; i++)
  states.append(Watched(i))

function mkRow(i) {
  let w = states[i]
  return @() {
    watch = w
    key = i
    rendObj = ROBJ_TEXT
    text = $"row {i}: {w.get()}"
    size = [flex(), SIZE_TO_CONTENT]
  }
}

let rows = []
for (local i = 0; i < ROWS; i++)
  rows.append(mkRow(i))

mkBenchRunner("bench_list_update", {
  onFrame = function(frame) {
    for (local k = 0; k < UPDATES_PER_FRAME; k++) {
      let idx = (frame * UPDATES_PER_FRAME + k) % ROWS
      states[idx].modify(@(v) v + 1)
    }
  }
})

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  flow = FLOW_VERTICAL
  clipChildren = true
  children = rows
}
