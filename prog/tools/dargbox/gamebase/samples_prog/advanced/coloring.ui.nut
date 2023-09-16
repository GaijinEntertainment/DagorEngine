from "%darg/ui_imports.nut" import *

let function col(color){
  return {
    rendObj = ROBJ_BOX
    size = [40,40]
    fillColor = color
    borderColor= Color(100,100,100)
  }
}

let function text(s){
  return {
    rendObj = ROBJ_TEXT
    text=s
  }
}
let fillColor = Color(0,0,0)
let borderColor = Color(55,55,0)
let tintColor = Color(0,0,255,1)
let saturation = 1.0
let brightness = 1.0

return {
  rendObj = ROBJ_SOLID
  color = Color(30, 30, 30)
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = 10
  children = [
    {flow=FLOW_HORIZONTAL valign = ALIGN_CENTER children = [
      text("fillColor:") col(fillColor) text(", borderColor: ") col(borderColor) text($", saturation: {saturation}") text($", brightness: {brightness}") text(", tintColor to:") col(tintColor)
    ]}
    {rendObj = ROBJ_BOX fillColor = fillColor borderColor=borderColor saturation = saturation brightness= brightness tint = tintColor size = [sh(10),sh(10)]}
  ]
}

