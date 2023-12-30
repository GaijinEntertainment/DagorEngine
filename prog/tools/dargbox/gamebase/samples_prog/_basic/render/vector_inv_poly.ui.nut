from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")
let math = require("math")

let coord = [50, 0, 50, 16, 36, 26, 36,  30,  0, 60,  52, 45,  110, 125, 110, 120]
let cmd = [VECTOR_INVERSE_POLY, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,  0, 0,  0, 0, 0, 0]
local t = 0.0


let vectorCanvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = [hdpx(300), hdpx(300)]
  lineWidth = hdpx(2.5)
  color = Color(50, 200, 255)
  fillColor = Color(122, 1, 0, 0)
  commands = [cmd]

  behavior = Behaviors.RtPropUpdate
  rtAlwaysUpdate = true
  update = function() {
    t += 0.01
    for (local i = 0; i < coord.len(); i += 2) {
      local x = coord[i] - 50;
      local y = coord[i + 1] - 50;
      cmd[i + 1] = (y * math.sin(t) + x * math.cos(t)) + 50;
      cmd[i + 2] = (y * math.cos(t) - x * math.sin(t)) + 50;
    }
    return {}
  }
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
