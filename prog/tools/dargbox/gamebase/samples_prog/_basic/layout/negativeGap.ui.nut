from "%darg/ui_imports.nut" import *

const FIRST = 0x0
const MIDDLE = 0x1
const LAST = 0x2

let width = sh(30)
let fixedHeight = hdpx(150)
let gap = hdpx(10)
let radius = 2 * gap
let bgColor = 0xFF000080
let amount = 6

let function block(height, order, margin = null) {
  let stateFlags = Watched(0)
  let borderRadius = order == LAST ? [0, 0, radius, radius]
    : order == FIRST ? [radius, radius, 0, 0]
    : 0
  return @() {
    watch = stateFlags
    size = [flex(), height]
    margin = margin
    rendObj = ROBJ_BOX
    borderRadius = borderRadius
    borderWidth = gap
    fillColor = 0xFF808000
    borderColor = stateFlags.value & S_HOVER ? 0xFFFFFFFF : 0x00000001

    onElemState = @(sf) stateFlags(sf)
    behavior = Behaviors.Button
  }
}

let header = @(text) { rendObj = ROBJ_TEXT, text = text, hplace = ALIGN_CENTER }

let getOrder = @(idx, total) idx == 0 ? FIRST
  : idx >= total - 1 ? LAST
  : MIDDLE

let mkList = @(headerText, listHeight, blockHeight, margin = null) {
  size = [width, listHeight]
  flow = FLOW_VERTICAL
  margin = gap
  gap = gap

  children = [
    header(headerText)
    {
      size = [width, listHeight]
      flow = FLOW_VERTICAL
      gap = -gap
      rendObj = ROBJ_BOX
      borderRadius = radius
      borderWidth = 0
      fillColor = bgColor
      children = array(amount).map(@(_, i) block(blockHeight, getOrder(i, amount), margin))
    }
  ]
}

return {
  size = flex()
  flow = FLOW_HORIZONTAL
  children = [
    mkList("Only gap, flex height", flex(), flex())
    mkList("Only gap, fixed height", SIZE_TO_CONTENT, fixedHeight)
    mkList("Gap & margin, flex height", flex(), flex(), [gap * 2, 0])
    mkList("Gap & margin, fixed height", SIZE_TO_CONTENT, fixedHeight, [gap * 2, 0])
  ]
}