from "%darg/ui_imports.nut" import *

let button = require ("fs_node.nut")

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
//  cursor = cursors.normal
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    button("button.nut", "scripts/basics/")
  ]
}
