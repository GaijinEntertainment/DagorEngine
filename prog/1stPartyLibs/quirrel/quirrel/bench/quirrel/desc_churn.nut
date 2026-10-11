// component-description churn: build and discard small string-keyed tables
// with nested arrays, the shape of a daRg builder re-evaluation
from "profile.nut" import vmBench

const N = 100000

function mkChild(i) {
  return {
    rendObj = 2
    size = [i % 100, 20]
    color = 4278190080 + (i % 65536)
    text = "child"
  }
}

function mkDesc(i) {
  return {
    rendObj = 1
    size = [100, 50]
    color = 0xFF102030
    text = "component"
    padding = 4
    margin = 2
    flow = 1
    halign = 0
    valign = 1
    gap = 3
    opacity = 0.5
    zOrder = i % 10
    clipChildren = true
    behavior = false
    hotkeys = false
    children = [mkChild(i), mkChild(i + 1), mkChild(i + 2)]
  }
}

function churn() {
  local sum = 0
  for (local i = 0; i < N; i++) {
    let d = mkDesc(i)
    sum += d.zOrder + d.children.len()
  }
  return sum
}

vmBench("desc_churn", churn)
