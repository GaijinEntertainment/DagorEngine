from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

require("daRg").gui_scene.setConfigProps({kbCursorControl= true})


function item(...) {
  let expanded = Watched(false)

  let button = watchElemState(@(sf) {
    size = static [flex(), sh(10)]
    rendObj = ROBJ_FRAME
    borderWidth = 2
    borderColor = Color(200,50,50)
    fillColor = (sf & S_ACTIVE) ? Color(200,200,50)
              : (sf & S_HOVER) ? Color(150, 200, 50)
              : Color(50,80,30)
    onHover = @(on) expanded.set(on)
    behavior = Behaviors.Button
  })

  let extra = {
    rendObj = ROBJ_TEXT
    hplace = ALIGN_CENTER
    text = "Extra object"
    margin = static [sh(3), 0]
  }


  return @() {
    size = FLEX_H
    behavior = Behaviors.TransitionSize
    speed = sh(50)
    orientation = O_VERTICAL
    watch = expanded
    flow = FLOW_VERTICAL

    children = [
      button
      expanded.get() ? extra : null
    ]
  }
}


let items = array(5).apply(item)

let container = {
  size = static [sh(40), sh(80)]
  rendObj = ROBJ_FRAME
  borderWidth = sh(1)
  fillColor = Color(80, 80, 80)
  borderColor = Color(200,200,200)
  children = items
  flow = FLOW_VERTICAL
  clipChildren = true
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = container
}
