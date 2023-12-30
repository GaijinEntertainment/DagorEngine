from "%darg/ui_imports.nut" import *
from "math" import max

let cursors = require("samples_prog/_cursors.nut")

let height = hdpx(100)
let cProgressWidth = height/5
let cProgresWidthFract = cProgressWidth/height/2*100
let vectorCanvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = [height, height]
  lineWidth = max(hdpx(1), height/80.0)
  color = Color(50, 200, 255)
  fillColor = Color(0, 0, 0, 50)
  commands = [
    [VECTOR_COLOR, Color(85, 85, 85, 100)],
    [VECTOR_ELLIPSE, 50, 50, 100, 100],
    [VECTOR_FILL_COLOR, 0],
    [VECTOR_ELLIPSE, 50, 50, 100-cProgresWidthFract*2, 100-cProgresWidthFract*2],
    [VECTOR_WIDTH, cProgressWidth],
    [VECTOR_COLOR, Color(50, 50, 255)],
    [VECTOR_SECTOR, 50, 50, 100-cProgresWidthFract, 100-cProgresWidthFract, 0.89, 0.2],
  ]
}



function basicsRoot() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30, 40, 50)
    cursor = cursors.normal
    size = flex()
    padding = hdpx(50)
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        flow = FLOW_VERTICAL
        size = flex()
        gap = hdpx(40)
        children = [
          vectorCanvas
        ]
      }
    ]
  }
}

return basicsRoot
