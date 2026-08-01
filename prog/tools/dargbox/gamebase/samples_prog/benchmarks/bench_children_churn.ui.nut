// Dynamic list structure: rows inserted/removed every frame, exercising the
// children diff (match_elem_with_new_comp), element alloc/free and stack
// rebuild paths.
from "%darg/ui_imports.nut" import *

let { mkBenchRunner } = require("samples_prog/benchmarks/bench_stats.nut")

const ROWS = 500
const CHURN_PER_FRAME = 10

local nextId = ROWS
let ids = []
for (local i = 0; i < ROWS; i++)
  ids.append(i)
let listState = Watched(ids)

function mkRow(id) {
  return {
    key = id
    rendObj = ROBJ_TEXT
    text = $"item {id}"
    size = [flex(), SIZE_TO_CONTENT]
  }
}

let list = @() {
  watch = listState
  size = flex()
  flow = FLOW_VERTICAL
  clipChildren = true
  children = listState.get().map(mkRow)
}

mkBenchRunner("bench_children_churn", {
  onFrame = function(_frame) {
    listState.mutate(function(v) {
      for (local k = 0; k < CHURN_PER_FRAME; k++) {
        v.remove(0)
        v.append(nextId)
        nextId++
      }
    })
  }
})

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  children = list
}
