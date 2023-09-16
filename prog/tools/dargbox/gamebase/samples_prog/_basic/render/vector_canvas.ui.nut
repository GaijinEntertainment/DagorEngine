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
  fillColor = Color(122, 1, 0, 0)
  commands = [
    [VECTOR_LINE, 10, 0, 60, 10],          // fromX, fromY, toX, toY
    [VECTOR_WIDTH, 4.2],                   // set current line width = 4.2
    [VECTOR_LINE, 0, 0, 50, 80, 100, 100], // fromX, fromY, toX, toY, toX, toY
    [VECTOR_WIDTH],                        // set default line width
    [VECTOR_COLOR, Color(255, 255, 255)],  // set stroke color = white
    [VECTOR_ELLIPSE, 70, 40, 30, 20],      // centerX, centerY, radiusX, radiusY
    [VECTOR_SECTOR, 3, 42, 15, 15, 0, 55], // centerX, centerY, radiusX, radiusY, angleFrom, angleTo
    [VECTOR_FILL_COLOR, Color(50, 180, 10, 255)], // set fill color to 'green'
    [VECTOR_SECTOR, 0, 40, 15, 15, 55, 0], // centerX, centerY, radiusX, radiusY, angleFrom, angleTo
    [VECTOR_FILL_COLOR, Color(0, 0, 0, 0)],// set transparent fill color
    [VECTOR_RECTANGLE, 80, 10, 20, 8],     // left, top, width, height
    [VECTOR_COLOR],                        // set default color
    [VECTOR_LINE, 50, 100, 50, 100],       // zero-length line will be rendered as dot
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
