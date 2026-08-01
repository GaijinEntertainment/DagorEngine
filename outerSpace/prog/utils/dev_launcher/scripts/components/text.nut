from "%darg/ui_imports.nut" import *
import "style.nut" as style

let textarea = @(text, params={}) {
    rendObj = ROBJ_TEXTAREA
    color = Color(198, 198, 178)
    textOverflowY = TOVERFLOW_LINE
    text
    behavior = Behaviors.TextArea
  }.__update(params)

let dtext = @(text, params=null) {rendObj = ROBJ_TEXT text}.__update(params ?? {})

let ta = @(val, params = null) textarea(val, {
  size = flex()
  behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
  textOverflowY = TOVERFLOW_CLIP
  preformatted = true
}.__update(params ?? {}))

return {textarea, dtext, ta}