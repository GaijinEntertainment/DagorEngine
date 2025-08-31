from "dagor.math" import CatmullRomSplineBuilder2D, Point2
from "%darg/ui_imports.nut" import *
let cursors = require("samples_prog/_cursors.nut")

let isClosed = false
let tension = 0.0
let keyPoints = [
  0, 0,
  50, 52,
  10, 84,
  0, 47,
  100, 90,
  100, 0,
]

local spline = CatmullRomSplineBuilder2D()
spline.build(keyPoints, isClosed, tension)

local dashes = []
let numPoints = 60
for (local f = 0.0; f < numPoints; f += 2.0) {
  local t0 = f / numPoints
  local t1 = (f + 1) / numPoints
  local p0 = spline.getMonotonicPoint(t0)
  local p1 = spline.getMonotonicPoint(t1)

  dashes.append([VECTOR_LINE, p0.x, p0.y, p1.x, p1.y])
}


let vectorCanvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = hdpx(300)
  lineWidth = hdpx(7.0)
  color = Color(50, 200, 255)
  fillColor = Color(122, 1, 0, 0)
  commands = dashes
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
          vectorCanvas
        ]
      }
    ]
  }
}

return basicsRoot
