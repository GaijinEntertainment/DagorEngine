from "%darg/ui_imports.nut" import *
from "math" import clamp

let cursors = require("samples_prog/_cursors.nut")

let rumbleVals = {
  loSpeed = Watched(0)
  hiSpeed = Watched(0)
}


function slider(rumble_field) {
  let minVal = 0.0
  let maxVal = 1.0

  let knob = static {
    size  = [hdpx(10), flex()]
    rendObj = ROBJ_SOLID
    pos = [-hdpx(10),0]
  }

  function onChange(val) {
    vlog($"rumble_field ->{val}")
    rumbleVals[rumble_field].set(val)
//    get_rumble()[rumble_field] = val
  }

  return function() {
    let fValue = rumbleVals[rumble_field].get()

    return {
      rendObj = ROBJ_SOLID
      valign = ALIGN_CENTER
      behavior = Behaviors.Slider

      watch = rumbleVals[rumble_field]
      fValue = fValue

      knob = knob
      min = minVal
      max = maxVal
      unit = 0.02
      orientation = O_HORIZONTAL
      size = static [flex(), sh(5)]
      color = Color(0, 10, 20)
      flow = FLOW_HORIZONTAL

      children = [
        {size=[flex(fValue-minVal), flex()] color = Color(clamp((fValue-minVal)/(maxVal-minVal), 0.0, 1.0)*160, 0, 0) rendObj = ROBJ_SOLID}
        knob
        {size=[flex(maxVal-fValue), flex()]}
      ]

      onChange
    }
  }
}
return {
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  rendObj = ROBJ_SOLID
  color = Color(20,30,50)

  cursor = cursors.normal

  flow = FLOW_VERTICAL
  gap = sh(5)

  children = [
    slider("loSpeed")
    slider("hiSpeed")
  ]
}
