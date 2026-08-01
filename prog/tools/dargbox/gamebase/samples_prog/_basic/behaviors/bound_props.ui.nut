// Behaviors.BoundProps: push-based property binding without rebuilds.
// bindProps = { propName = observable } re-applies the element in place when
// an observable changes; layout-affecting keys (text, size, ...) relayout
// automatically, paint-only keys (color, opacity, ...) do not.
from "%darg/ui_imports.nut" import *
from "dagor.workcycle" import setInterval

let cursors = require("samples_prog/_cursors.nut")

local tick = 0
let boundColor = Watched(Color(200, 80, 80))
let boundOpacity = Watched(1.0)
let boundText = Watched("tick 0")
let boundSize = Watched([100, 100])

setInterval(0.5, function() {
  tick++
  boundColor.set(Color(80 + (tick * 37) % 176, 80 + (tick * 73) % 176, 80))
  boundOpacity.set(0.35 + 0.65 * ((tick % 4) / 3.0))
  boundText.set($"tick {tick}")
  boundSize.set([100 + (tick % 5) * 20, 100])
})

// static tables: no builders, never rebuilt; all updates go through bindings
let colorBox = {
  rendObj = ROBJ_SOLID
  size = [100, 100]
  bindProps = { color = boundColor, opacity = boundOpacity }
}

let sizeBox = {
  rendObj = ROBJ_BOX
  fillColor = Color(60, 100, 60)
  bindProps = { size = boundSize }
}

let label = {
  rendObj = ROBJ_TEXT
  bindProps = { text = boundText }
}

// rebuild survival: this subtree is torn down and rebuilt by watch, the
// bound cell inside must keep showing current values after every rebuild
let subtreeGen = Watched(0)
setInterval(2.0, @() subtreeGen.set(subtreeGen.get() + 1))

function mkRebuiltSubtree() {
  return @() {
    watch = subtreeGen
    flow = FLOW_HORIZONTAL
    gap = 10
    valign = ALIGN_CENTER
    children = [
      { rendObj = ROBJ_TEXT, text = $"rebuild #{subtreeGen.get()}" }
      {
        rendObj = ROBJ_SOLID
        size = [50, 50]
        bindProps = { color = boundColor }
      }
    ]
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30, 40, 50)
  size = flex()
  cursor = cursors.normal
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  gap = 20
  children = [
    label
    colorBox
    sizeBox
    mkRebuiltSubtree()
  ]
}
