from "%darg/ui_imports.nut" import *

let dmModule = require("dmModule.nut")

let engine = dmModule(Watched(3), Watched(1))
let transmission = dmModule(Watched(2), Watched(1))
let torpedo = dmModule(Watched(2), Watched(0))
let artillery = dmModule(Watched(2), Watched(2))
let steeringGears = dmModule(Watched(2), Watched(2))

let damageModules = {
  rendObj = ROBJ_FRAME
  borderWidth = 1
  size = SIZE_TO_CONTENT
  flow = FLOW_VERTICAL
  valign = ALIGN_TOP
  children = [
    engine
    steeringGears
    transmission
    torpedo
    artillery
  ]
}

return {
  padding = 20
  children = damageModules
}
