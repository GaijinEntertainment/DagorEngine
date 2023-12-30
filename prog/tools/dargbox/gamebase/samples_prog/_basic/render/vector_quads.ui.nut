from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let vectorCanvasQuadList = {
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
      // list of quad vertices, 4 vertices per quad: x, y, color
      10, 10, Color(255, 255, 255), 10, 90, Color(255, 255, 255), 90, 90, Color(255, 255, 255), 90, 10, Color(255, 255, 255), // first quad
      20, 20, Color(255, 255, 255), 20, 80, Color(255, 0, 255), 80, 80, Color(255, 0, 255), 80, 20, Color(255, 255, 255)],    // second quad
  ]
}

let vectorCanvasVerticesIndices = {
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
      // all vertices
      [10, 10, Color(255, 255, 255), // 0
       10, 90, Color(255, 255, 255), // 1
       90, 90, Color(255, 255, 255), // 2
       90, 10, Color(255, 255, 255), // 3
       20, 20, Color(255, 255, 255), // 4
       20, 80, Color(255, 0, 255),   // 5
       80, 80, Color(255, 0, 255),   // 6
       80, 20, Color(255, 255, 255), // 7
      ],
      // indices
      [
        0, 1, 2, 3, // first quad
        4, 5, 6, 7, // second quad
      ]
    ]
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
          vectorCanvasQuadList,
          vectorCanvasVerticesIndices,
        ]
      }
    ]
  }
}

return basicsRoot
