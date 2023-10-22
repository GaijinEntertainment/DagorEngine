from "%darg/ui_imports.nut" import *

let {makeVertScroll} = require("samples_prog/_basic/components/scrollbar.nut")

let txt = @(text, style = null) {rendObj = ROBJ_TEXT text}.__update(style ?? {})
let textArea = @(text, style = null) {rendObj = ROBJ_TEXTAREA behavior = Behaviors.TextArea text size = [flex(), SIZE_TO_CONTENT]}.__update(style ?? {})
let txtBtn = function(text, handler=null, opts = null) {
  let stateFlags=Watched(0)
  return @() {
    rendObj = ROBJ_BOX
    watch = stateFlags
    padding = [hdpx(5), hdpx(10)]
    children = txt(text)
    onClick = handler
    onElemState = @(s) stateFlags(s)
    borderRadius = hdpx(5)
    behavior = Behaviors.Button
    fillColor = stateFlags.value & S_HOVER ? Color(60,80,80) : Color(40,40,40)
  }.__update(opts ?? {})
}

return {
  txt
  textArea
  txtBtn
  makeVertScroll
}