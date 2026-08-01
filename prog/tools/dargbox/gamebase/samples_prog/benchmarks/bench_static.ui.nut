// Large static screen: baseline per-frame cost of a big element tree with no
// updates at all (layout/render/update overhead floor).
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const COLS = 100
const ROWS = 50 // COLS * ROWS leaf elements

function cell(i) {
  return {
    rendObj = ROBJ_SOLID
    size = flex()
    margin = 1
    color = Color(20 + i % 200, 40, 60)
  }
}

function mkRow(r) {
  let children = []
  for (local c = 0; c < COLS; c++)
    children.append(cell(r * COLS + c))
  return {
    size = flex()
    flow = FLOW_HORIZONTAL
    children
  }
}

let rows = []
for (local r = 0; r < ROWS; r++)
  rows.append(mkRow(r))

mkBenchRunner("bench_static")

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  flow = FLOW_VERTICAL
  children = rows
}
