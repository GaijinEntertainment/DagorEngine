from "%darg/ui_imports.nut" import *
let cursors = require("samples_prog/_cursors.nut")
let { makeVertScroll } = require("samples_prog/_basic/components/scrollbar.nut")

let colors = [0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFF00FFFF, 0xFFFF00FF]
let counts = [3, 5, 22, 4, 17]
let width = hdpx(500)
let headerSize = [width, hdpx(100)]
let optSize = [flex(), hdpx(75)]
let expandedIdx = Watched(-1)


let mkOption = @(idx) {
  size = optSize
  rendObj = ROBJ_SOLID
  color = colors[idx % colors.len()]
}

function mkFoldable(count, idx) {
  let stateFlags = Watched(0)
  let isExpanded = Computed(@() expandedIdx.get() == idx)

  let header = @() {
    watch = stateFlags
    size = headerSize
    rendObj = ROBJ_SOLID
    color = stateFlags.get() & S_HOVER ? 0xFF000000 : 0xFFFFFFFF
    behavior = Behaviors.Button
    onClick = @() expandedIdx.set(isExpanded.get() ? -1 : idx)
    onElemState = @(v) stateFlags.set(v)
    valign = ALIGN_CENTER
    children = {
      rendObj = ROBJ_TEXT
      text = $"Header {idx}"
      color = stateFlags.get() & S_HOVER ? 0xFFFFFFFF : 0xFF000000
    }
  }

  let children = {
    size = FLEX_H
    flow = FLOW_VERTICAL
    padding = hdpx(10)
    gap = hdpx(10)
    rendObj = ROBJ_SOLID
    color = 0xFF202020
    clipChildren = true
    children = array(count).map(@(_, i) mkOption(i))
  }

  return {
    size = FLEX_H
    orientation = O_VERTICAL
    clipChildren = true
    flow = FLOW_VERTICAL
    children = [
      header
      @() {
        key = {}
        watch = isExpanded
        behavior = Behaviors.TransitionSize
        orientation = O_VERTICAL
        speed = 0
        maxTime = 0.3
        targetSize = isExpanded.get() ? FLEX_H : const [flex(), 0]
        size = FLEX_H
        children
      }
    ]
  }
}

let list = {
  size = [width, SIZE_TO_CONTENT]
  flow = FLOW_VERTICAL
  gap = hdpx(20)
  children = counts.map(mkFoldable)
}

return {
  size = flex()
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = {
    size = [width, ph(95)]
    children = makeVertScroll(list)
  }
}