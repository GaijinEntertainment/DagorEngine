from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let button = @() watchElemState( @(sf) {
  rendObj = ROBJ_BOX
  size = [sh(20),SIZE_TO_CONTENT]
  padding = sh(2)
  fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
  borderWidth = (sf & S_HOVER) ? 2 : 0
  behavior = [Behaviors.Button]
  shareMouse = true
  halign = ALIGN_CENTER
  children = {rendObj = ROBJ_TEXT text = (sf & S_ACTIVE) ? "pressed": "button" color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }

  function onClick() {
    vlog("clicked")
  }

  function onHover(hover) {
    vlog($"hover: {hover}")
  }

  function onTouchHold() {
    vlog("onTouchHold")
  }

  sound = {
    click  = "ui/button_click"
    hover  = "ui/menu_highlight"
    active = "ui/button_action"
    hold = "ui/button_action"
  }
})


let container = {
  size = [sh(30), sh(80)]
  rendObj = ROBJ_BOX
  borderWidth = hdpx(3)
  borderColor = Color(150,150,150)
  fillColor = Color(40,50,60)

  behavior = Behaviors.Pannable
  shareMouse = true

  children = {
    size = [sh(30), sh(150)]
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    padding = sh(5)
    gap = sh(5)

    children = [
      button()
      button()
      button()
      button()
    ]
  }
}

let contOuter = {
  size = [sh(30), sh(80)]
  clipChildren = true
  children = container
}


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = contOuter
}
