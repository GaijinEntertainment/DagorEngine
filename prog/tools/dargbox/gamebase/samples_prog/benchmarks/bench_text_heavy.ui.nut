// Text layout stress: many wrapping textareas, a slice of them changing text
// each frame (re-measure + re-flow of wrapped text).
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const AREAS = 200
const UPDATES_PER_FRAME = 10
const GRID_COLS = 10

let baseText = "The quick brown fox jumps over the lazy dog and keeps running through the long wrapped line of user interface text"

let states = []
for (local i = 0; i < AREAS; i++)
  states.append(Watched(0))

function mkArea(i) {
  let w = states[i]
  return @() {
    watch = w
    key = i
    size = [flex(), SIZE_TO_CONTENT]
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    text = $"{baseText} #{w.get()}"
  }
}

let cols = []
for (local c = 0; c < GRID_COLS; c++) {
  let cells = []
  for (local r = 0; r < AREAS / GRID_COLS; r++)
    cells.append(mkArea(c * (AREAS / GRID_COLS) + r))
  cols.append({
    size = flex()
    flow = FLOW_VERTICAL
    gap = 2
    children = cells
  })
}

mkBenchRunner("bench_text_heavy", {
  onFrame = function(frame) {
    for (local k = 0; k < UPDATES_PER_FRAME; k++) {
      let idx = (frame * UPDATES_PER_FRAME + k) % AREAS
      states[idx].modify(@(v) v + 1)
    }
  }
})

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  flow = FLOW_HORIZONTAL
  gap = 8
  padding = 20
  children = cols
}
