from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let DURATION = 1

let function makeAnim(params) {
  return [
    { prop=AnimProp.rotate, from=0, to=90, duration=DURATION, play=true, easing=InOutCubic, loop=true }.__merge(params)
  ]
}

function makeRow(animations) {
  return {
    flow = FLOW_HORIZONTAL
    gap = sh(5)
    children = array(5).map(@(_, n) {
      size = [sh(10), sh(10)]
      rendObj = ROBJ_SOLID
      color = Color(220, 180, 120)
      transform = {}
      animations
    })
  }
}


let anims= [
  makeAnim({})
  makeAnim({delay=DURATION})
  makeAnim({delay=DURATION, loopPause=2*DURATION})
]

let content = {
  flow = FLOW_VERTICAL
  gap = sh(5)
  children = anims.map(@(a) makeRow(a))
}



return {
  rendObj = ROBJ_SOLID
  size = flex()
  color = Color(30,40,50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  children = content
}
