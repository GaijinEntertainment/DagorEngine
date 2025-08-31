from "%darg/ui_imports.nut" import *

// Activate XMB navigation using keyboard
require("daRg").gui_scene.setConfigProps({kbCursorControl= true})

let cursors = require("samples_prog/_cursors.nut")

let order = Watched(array(5).map(@(_val, idx) idx))

let info = {
  rendObj = ROBJ_TEXTAREA
  behavior = Behaviors.TextArea
  text = @"Use arrows to navigate.
    Press Space to reorder XMB nodes.
    Their traverse order will not be recalculated."
  size = static [pw(80), SIZE_TO_CONTENT]
  halign = ALIGN_CENTER
}

function node(id) {
  return {
    key = id
    rendObj = ROBJ_SOLID
    color = Color(100,180,50)
    size = sh(10)
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = {
      rendObj = ROBJ_TEXT
      text = id
      fontSize = sh(5)
    }
    xmbNode = XmbNode()
    behavior = Behaviors.Button
  }
}

let xmb = @() {
  watch = order
  xmbNode = XmbContainer()
  flow = FLOW_HORIZONTAL
  gap = sh(5)
  children = order.get().map(@(i) node(i))
  hotkeys = [
    ["Space", @() order.mutate(@(v) v.reverse())]
  ]
}


let root = {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(30, 40, 50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(5)
  children = [
    xmb
    info
  ]
}

return root
