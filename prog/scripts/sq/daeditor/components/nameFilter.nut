from "%darg/ui_imports.nut" import *
let {colors} = require("style.nut")


function nameFilter(watched_text, params) {
  let group = ElemGroup()
  let stateFlags = Watched(0)
  let stateFlagsClear = Watched(0)

  local placeholder = null
  if (params?.placeholder) {
    placeholder = {
      rendObj = ROBJ_TEXT
      text = params.placeholder
      color = Color(160, 160, 160)
    }
  }

  let canClear = function() {
    if (params?.onClear == null)
      return false
    if (stateFlags.get() & S_KB_FOCUS)
      return true
    return watched_text.get().len() > 0
  }

  return @() {
    size = FLEX_H
    watch = [watched_text, stateFlags, stateFlagsClear]

    rendObj = ROBJ_SOLID
    color = colors.ControlBg

    children = {
      size = FLEX_H
      rendObj = ROBJ_FRAME
      color = (stateFlags.get() & S_KB_FOCUS) ? colors.FrameActive : colors.FrameDefault
      group = group
      flow = FLOW_HORIZONTAL

      children = [
        {
          rendObj = ROBJ_TEXT
          size = FLEX_H
          margin = fsh(0.5)

          text = watched_text.get() ?? " "
          behavior = Behaviors.TextInput
          group

          onChange = params?.onChange
          onEscape = params?.onEscape
          onReturn = params?.onReturn
          onElemState = @(sf) stateFlags.set(sf)

          children = watched_text.get().len() ? null : placeholder
        }
        canClear() ? {
          rendObj = ROBJ_TEXT
          size = SIZE_TO_CONTENT
          margin = fsh(0.5)
          text = "   X"
          transform = { scale = [1, 0.75], translate = [hdpx(-2), 0] }
          color = (stateFlagsClear.get() & S_HOVER) ? Color(250, 250, 250) : Color(120, 120, 120)
          behavior = Behaviors.Button
          onElemState = @(sf) stateFlagsClear.set(sf)
          onClick = params?.onClear
        } : null
      ]
    }
  }
}


return nameFilter
