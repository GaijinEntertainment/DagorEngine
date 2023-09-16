from "%darg/ui_imports.nut" import *

let wbp = @(color = null, fillColor = null) {
  // so, fillColor is not needed
  rendObj = ROBJ_WORLD_BLUR_PANEL
  size = [flex(), SIZE_TO_CONTENT]
  minWidth = SIZE_TO_CONTENT
  halign = ALIGN_CENTER
  padding = sh(1)
  children = {
    rendObj = ROBJ_TEXT
    text = $"c:{color} fc:{fillColor}"
  }
}.__update(color != null ? {color} : {}, fillColor != null ? {fillColor} : {})

let img = {
  rendObj = ROBJ_IMAGE
  size = [ph(100), ph(100)]
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  keepAspect = true
  image = Picture("ui/ca_cup1.png")
}

let wpbs = {
  flow = FLOW_VERTICAL
  gap = sh(1)
  children = [
    wbp()
    wbp(0x77FF0000)
    wbp(null, 0x7700FF00)
    wbp(0x7700FF00, 0x770000FF)
  ]
}

return {
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  children = [
    img
    wpbs
  ]
}