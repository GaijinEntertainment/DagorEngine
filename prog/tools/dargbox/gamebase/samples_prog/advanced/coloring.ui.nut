from "%darg/ui_imports.nut" import *
let function col(color){
  return {
    rendObj = ROBJ_BOX
    size = 40
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
let fillColor = Color(200,0,200)
let borderColor = Color(255,255,0)
let tintColor = Color(0,0,255,120)
let brightness = 0.8

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
      text("fillColor:") col(fillColor) text(", borderColor: ") col(borderColor) text($", brightness: {brightness}") text(", tintColor to:") col(tintColor)
    ]}
    {rendObj = ROBJ_BOX borderWidth=4 fillColor = fillColor borderColor=borderColor brightness=brightness tint = tintColor size = sh(10)}
    {rendObj = ROBJ_SOLID color = fillColor borderColor=borderColor brightness=brightness tint = tintColor size = sh(10)}
  ]
}
