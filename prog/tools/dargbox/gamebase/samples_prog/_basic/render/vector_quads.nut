from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let vectorCanvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = [hdpx(300), hdpx(300)]
  lineWidth = hdpx(2.5)
  color = Color(50, 200, 255)
  fillColor = Color(255, 120, 100, 0)
  commands = [
    [VECTOR_COLOR, Color(255, 255, 255)],  // set stroke color = white
    [VECTOR_QUADS,
      10, 10, Color(255, 255, 255), 10, 90, Color(255, 255, 255), 90, 90, Color(255, 255, 255), 90, 10, Color(255, 255, 255), // first quad
      20, 20, Color(255, 255, 255), 20, 80, Color(255, 0, 255), 80, 80, Color(255, 0, 255), 80, 20, Color(255, 255, 255)],    // second quad
  ]
}



let function basicsRoot() {
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
          vectorCanvas
        ]
      }
    ]
  }
}

return basicsRoot
