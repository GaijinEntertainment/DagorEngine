// Worst-case layout propagation: deep SIZE_TO_CONTENT nesting where a leaf
// text change re-flows the whole chain up to the size root each frame.
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const COLUMNS = 20
const DEPTH = 12

let leafStates = []
for (local i = 0; i < COLUMNS; i++)
  leafStates.append(Watched(0))

function mkChain(col, depth) { //-w226
  if (depth == 0) {
    let w = leafStates[col]
    return @() {
      watch = w
      rendObj = ROBJ_TEXT
      text = $"v{w.get() % 1000}"
    }
  }
  return {
    size = SIZE_TO_CONTENT
    padding = 1
    rendObj = ROBJ_FRAME
    borderWidth = 1
    children = mkChain(col, depth - 1)
  }
}

let columns = []
for (local i = 0; i < COLUMNS; i++)
  columns.append(mkChain(i, DEPTH))

mkBenchRunner("bench_deep_flex", {
  onFrame = function(frame) {
    // one column per frame, round robin
    leafStates[frame % COLUMNS].modify(@(v) v + 1)
  }
})

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  flow = FLOW_HORIZONTAL
  gap = 4
  padding = 20
  children = columns
}
