// BoundProps vs classic watch comparison: a grid of solid cells whose
// colors all change every frame.
//   "watch"   - each cell is a builder with watch = obs (full rebuild path)
//   "binding" - each cell is a STATIC table with bindProps = { color = obs }:
//               no builder eval, no diff, in-place re-apply,
//               paint-only so no layout work

from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const CELLS = 300
const GRID_COLS = 25

function colorOf(v) {
  return Color(v % 256, (v * 3) % 256, (v * 7) % 256)
}

function mkFieldBench(mode) {
  let states = []
  for (local i = 0; i < CELLS; i++)
    states.append(Watched(colorOf(i)))

  function cellWatch(i) {
    let w = states[i]
    return @() {
      watch = w
      rendObj = ROBJ_SOLID
      size = flex()
      margin = 1
      color = w.get()
    }
  }

  function cellBinding(i) {
    let w = states[i]
    return {
      rendObj = ROBJ_SOLID
      size = flex()
      margin = 1
      bindProps = { color = w }
    }
  }

  mkBenchRunner(mode == "watch" ? "bench_field_watch" : "bench_bound_props", {
    onFrame = function(frame) {
      for (local i = 0; i < CELLS; i++)
        states[i].set(colorOf(frame + i))
    }
  })

  let rows = []
  for (local r = 0; r < CELLS / GRID_COLS; r++) {
    let cells = []
    for (local c = 0; c < GRID_COLS; c++) {
      let idx = r * GRID_COLS + c
      cells.append(mode == "watch" ? cellWatch(idx) : cellBinding(idx))
    }
    rows.append({
      size = flex()
      flow = FLOW_HORIZONTAL
      children = cells
    })
  }

  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = Color(10, 10, 10)
    flow = FLOW_VERTICAL
    padding = 20
    children = rows
  }
}

return {
  mkFieldBench
}
