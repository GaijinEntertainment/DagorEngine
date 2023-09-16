from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let function make_samples() {
///utility function to create lots of childs
  let ret = []
  foreach (i in [1 2 3 4 5 6]) {
    ret.append({size = [sh(5),sh(5)] rendObj = ROBJ_SOLID color= Color(100,100,100) halign = ALIGN_CENTER valign = ALIGN_CENTER children = {rendObj = ROBJ_TEXT size=SIZE_TO_CONTENT text=i}})
  }
  return ret
}

let flow_horizontal = {
  flow = FLOW_HORIZONTAL
  size = SIZE_TO_CONTENT
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  padding = sh(1)
  halign = ALIGN_CENTER
  gap = sh(1) //gap between elemts
  children = make_samples()
}

let flow_vertical = {
  flow = FLOW_VERTICAL
  size = SIZE_TO_CONTENT
  halign = ALIGN_CENTER
  gap = {size = [flex(),sh(1.5)] valign=ALIGN_CENTER flow=FLOW_VERTICAL children = {size=[flex(), sh(0.2)] rendObj = ROBJ_SOLID color=Color(40,20,20)}} //gap can be a component!
  children = make_samples()
}


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  cursor = cursors.normal
  halign = ALIGN_LEFT
  valign = ALIGN_CENTER
  flow = FLOW_HORIZONTAL
  size = flex()
  children = [
    {
      size = [sw(40), sh(50)]
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      gap = sh(2)

      children = [
        {rendObj = ROBJ_TEXT text = "horizontal flow, with gap"}
        flow_horizontal
        {size=[0,sh(10)]}
        {rendObj = ROBJ_TEXT text = "vertical flow, with gap"}
        flow_vertical
      ]
    }
  ]
}
