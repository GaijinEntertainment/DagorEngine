from "%darg/ui_imports.nut" import *
import "%sqstd/ecs.nut" as ecs

let iconWidget = require_optional("%ui/components/icon3d.nut")
let math = require("math")

function mkIconView(eid){
  if (ecs.obsolete_dbg_get_comp_val(eid, "animchar__res") != null && ecs.obsolete_dbg_get_comp_val(eid, "item__iconYaw") != null) { // it has icon in it most likely!
    let itemParams = Watched(null)
    let iconParams = {width=math.min(hdpx(256), fsh(40)), height=math.min(hdpx(256), fsh(40))}
    function updateItemParams(){
      let iconOffs = ecs.obsolete_dbg_get_comp_val(eid, "item__iconOffset")
      let itemTbl = {
        iconName = ecs.obsolete_dbg_get_comp_val(eid, "animchar__res")
        iconYaw = ecs.obsolete_dbg_get_comp_val(eid, "item__iconYaw")
        iconPitch = ecs.obsolete_dbg_get_comp_val(eid, "item__iconPitch")
        iconRoll = ecs.obsolete_dbg_get_comp_val(eid, "item__iconRoll")
        iconScale = ecs.obsolete_dbg_get_comp_val(eid, "item__iconScale")
        iconSunZenith = ecs.obsolete_dbg_get_comp_val(eid, "item__iconSunZenith")
        iconSunAzimuth = ecs.obsolete_dbg_get_comp_val(eid, "item__iconSunAzimuth")
        iconOffsX = iconOffs?.x
        iconOffsY = iconOffs?.y
      }
      itemParams(itemTbl)
    }
    updateItemParams()
    gui_scene.setInterval(1, updateItemParams)
    return @(){
      watch = itemParams
      children = iconWidget(itemParams.value, iconParams)
      hplace = ALIGN_CENTER
    }
  }
  return null
}
return mkIconView
