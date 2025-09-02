from "%darg/ui_imports.nut" import *

return {
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = 10
  padding = 10
  opacity = 0.2
  children = [
    { rendObj = ROBJ_SOLID, size = 200, color = 0xFFFF00 }
    { rendObj = ROBJ_SOLID, size = 200, color = 0xFF00FF }
    { rendObj = ROBJ_SOLID, size = 200, color = 0x00FFFF }
  ]
}