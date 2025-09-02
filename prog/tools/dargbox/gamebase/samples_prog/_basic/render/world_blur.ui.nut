from "%darg/ui_imports.nut" import *

let backImage = Picture("ui/loading.jpg")

let mkBlur = @(color) {
  rendObj = ROBJ_WORLD_BLUR
  size = static [flex(), 30]
  color
}

let darkBlock = {
  size = FLEX_H
  gap = 20
  flow = FLOW_VERTICAL
  children = [
    mkBlur(0xFF000000)
    mkBlur(0xCC000000)
    mkBlur(0x99000000)
    mkBlur(0x66000000)
    mkBlur(0x33000000)
    mkBlur(0x00000000)
  ]
}

let liteBlock = {
  size = FLEX_H
  gap = 20
  flow = FLOW_VERTICAL
  children = [
    mkBlur(0xFFFFFFFF)
    mkBlur(0xCCFFFFFF)
    mkBlur(0x99FFFFFF)
    mkBlur(0x66FFFFFF)
    mkBlur(0x33FFFFFF)
    mkBlur(0x00FFFFFF)
  ]
}

return {
  rendObj = ROBJ_IMAGE
  size = static [pw(100), ph(100)]
  keepAspect = true
  image = backImage
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  valign = ALIGN_CENTER
  gap = 20
  flow = FLOW_HORIZONTAL
  children = [
    darkBlock
    liteBlock
  ]
}