from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")
//let math = require("math")

let cmd = [VECTOR_POLY, 50, 0, 50, 16, 36, 26, 34,  49,  0, 60,  52, 56,  110, 125, 110, 120]

let vectorCanvasFakeAA2x = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = hdpx(300)
  lineWidth = hdpx(2.5)
  color = Color(0, 0, 0, 0)
  commands = [
    [VECTOR_TM_OFFSET, 0.6, 0.4],
    [VECTOR_FILL_COLOR, Color(122, 125, 100, 127)],
    cmd,
    [VECTOR_TM_OFFSET], // reset offset
    cmd,
  ]
}

let vectorCanvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = hdpx(300)
  lineWidth = hdpx(2.5)
  color = Color(0, 0, 0, 0)
  commands = [
    [VECTOR_FILL_COLOR, Color(122 * 3 / 2, 125 * 3 / 2, 100 * 3 / 2, 127 * 3 / 2)],
    cmd,
  ]
}



function basicsRoot() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30, 40, 50)
    cursor = cursors.normal
    size = flex()
    padding = 50
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        flow = FLOW_VERTICAL
        size = flex()
        gap = 40
        children = [
          vectorCanvasFakeAA2x
          vectorCanvas
        ]
      }
    ]
  }
}

return basicsRoot
