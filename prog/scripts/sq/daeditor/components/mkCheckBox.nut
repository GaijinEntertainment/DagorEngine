from "%darg/ui_imports.nut" import *
let { colors } = require("style.nut")

function mkCheckBox(value, onClick) {
  let group = ElemGroup()
  let stateFlags = Watched(0)
  let hoverFlag = Computed(@() stateFlags.get() & S_HOVER)

  return function () {
    local mark = null
    if (value.get()) {
      mark = @(){
        rendObj = ROBJ_SOLID
        watch = hoverFlag
        color = (hoverFlag.get() != 0) ? colors.Hover : colors.Interactive
        group
        size = [pw(50), ph(50)]
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
      }
    }

    return {
      flow = FLOW_HORIZONTAL
      halign = ALIGN_LEFT
      valign = ALIGN_CENTER

      watch = [value]

      children = [
        {
          size = [fontH(80), fontH(80)]
          rendObj = ROBJ_SOLID
          color = colors.ControlBg

          behavior = Behaviors.Button
          group

          children = mark

          onElemState = @(sf) stateFlags.set(sf)

          onClick
        }
      ]
    }
  }
}

return mkCheckBox