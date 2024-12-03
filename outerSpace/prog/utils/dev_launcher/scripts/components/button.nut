from "%darg/ui_imports.nut" import *
import "style.nut" as style

let {setTooltip} = require("cursors.nut")

let mkButton = kwarg(function(onClick, text=null, txtIcon=null, fontSize = style.NORMAL_FONT_SIZE, tooltip=null, hotkeys = null, iconComp=null, override=null) {
  let stateFlags = Watched(0)
  let sizeToContent = "size" not in override
  let group = ElemGroup()
  let icon = iconComp ?? (txtIcon == null ? null : @() {
    rendObj = ROBJ_TEXT
    text = txtIcon
    watch = stateFlags
    font = Fonts?.fontawesome
    fontSize
    color = stateFlags.get() & S_HOVER ? style.BTN_TXT_HOVER : style.BTN_TXT_NORMAL
  })
  return function(){
    return {
      onHover = tooltip !=null ? @(on) setTooltip(on ? tooltip : null) : null
      padding = text !=null ? [fontSize/5, fontSize/2] : fontSize/3
      rendObj = ROBJ_BOX, behavior = Behaviors.Button, watch = stateFlags, onElemState = @(s) stateFlags.set(s)
      onClick, hotkeys, tooltip
      fillColor = stateFlags.get() & S_HOVER ? style.BTN_BG_HOVER : style.BTN_BG_NORMAL
      borderWidth = hdpx(1)
      borderColor = style.BTN_BD_NORMAL
      minWidth = style.NORMAL_FONT_SIZE
      minHeight= style.NORMAL_FONT_SIZE
      flow = FLOW_HORIZONTAL
      valign = ALIGN_CENTER
      group
      gap = fontSize/2
      clipChildren = true
      children = [icon, text == null ? null : {
        size = sizeToContent ? null : [flex(),SIZE_TO_CONTENT]
        behavior = sizeToContent ? null : Behaviors.Marquee
        scrollOnHover=true
        group
        children = {
          text
          fontSize
          rendObj = ROBJ_TEXT
          color = stateFlags.get() & S_HOVER ? style.BTN_TXT_HOVER : style.BTN_TXT_NORMAL
        }

      }]
    }.__update(override ?? {})
  }
})

return {mkButton}