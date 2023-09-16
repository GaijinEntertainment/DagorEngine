from "%darg/ui_imports.nut" import *

let txt = @(text) { rendObj = ROBJ_TEXT, text }

let extPadding = @(children) {
  rendObj = ROBJ_SOLID
  padding = hdpx(20)
  color = 0xFF666666
  flow = FLOW_VERTICAL
  children = [
    children children children
  ]
}

let intMargin = @(margin) {
  size = [hdpx(150), SIZE_TO_CONTENT]
  rendObj = ROBJ_SOLID
  padding = hdpx(5)
  margin = hdpx(margin)
  color = 0xFF333333
  halign = ALIGN_CENTER
  children = txt($"margin: {margin}")
}

return {
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = {
    flow = FLOW_HORIZONTAL
    gap = hdpx(20)
    children = [
      extPadding(intMargin(0))
      extPadding(intMargin(10))
      extPadding(intMargin(20))
      extPadding(intMargin(30))
    ]
  }
}