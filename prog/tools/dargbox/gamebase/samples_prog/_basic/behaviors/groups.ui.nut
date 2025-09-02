from "%darg/ui_imports.nut" import *

let combined = ElemGroup()

let mkButton = function(color, group = null) {
  let sf = Watched(0)
  return @() {
    group
    watch = sf
    rendObj = ROBJ_SOLID
    color = sf.get() & S_HOVER ? 0xFFFFFF : color
    padding = 20
    children = {
      rendObj = ROBJ_TEXT
      text = color
      color = sf.get() & S_HOVER ? 0xFF000000 : 0xFFFFFF
    }
    behavior = Behaviors.Button
    onElemState = @(flags) sf.set(flags)
  }
}

return {
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  flow = FLOW_HORIZONTAL
  gap = 10
  children = [
    mkButton(0xFFCC99)
    mkButton(0xFF99CC)
    mkButton(0xCCFF99)
    mkButton(0x99FFCC, combined)
    mkButton(0xCC99FF, combined)
    mkButton(0x99CCFF, combined)
  ]
}