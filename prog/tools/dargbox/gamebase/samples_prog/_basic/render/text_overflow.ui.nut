from "%darg/ui_imports.nut" import *
from "math" import max

/*
   Horizontal overflow handling modes:
    - TOVERFLOW_CLIP (clip at coord, default)
    - TOVERFLOW_CHAR (truncate by symbol)
    - TOVERFLOW_WORD (truncate by word)

   Vertical overflow handling modes:
    - TOVERFLOW_CLIP (clip at coord default)
    - TOVERFLOW_LINE (truncate by line)
*/

let cursors = require("samples_prog/_cursors.nut")

let textFrameState = Watched({
  size   = static [hdpx(107), hdpx(30)]
  pos    = [0, 0]
})

function dText(text, params=null) {
  return {
    rendObj = ROBJ_TEXT
    color = Color(198,198,128)
    text
  }.__update(params ?? {})
}


function dTextFrame(params={}) {
  return @() {
    rendObj = ROBJ_FRAME
    color = Color(255,0,0)
    size = textFrameState.get().size
    pos = textFrameState.get().pos
    children = dText("Съешь ещё этих мягких французских булок, да выпей же чаю").__update(params, {size=flex() textOverflowX=null})
    behavior = Behaviors.MoveResize
    moveResizeCursors = cursors.moveResizeCursors
    watch = textFrameState

    function onMoveResize(dx, dy, dw, dh) {
      let w = textFrameState.get()
      w.pos = [w.pos[0]+dx, w.pos[1]+dy]
      w.size = [max(5, w.size[0]+dw), max(20, w.size[1]+dh)]
      return w
    }
  }
}

let white = Color(255,255,255)
return {
  rendObj = ROBJ_SOLID
  color = Color(80,80,80)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  cursor = cursors.normal
  size = flex()
  children = {
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    size = static [sw(80),sh(80)]
    gap = 10
    children = [
      dText("This is auto-clipped overflowed text")
      {rendObj = ROBJ_FRAME size = [100,30].map(hdpx) children = {size =flex() rendObj = ROBJ_TEXT text="This is long-long-long text"}}
      dText("This is nonlcipped overflowed text (text element has size by content and parent is not clipping children)")
      {rendObj = ROBJ_FRAME  size = [100,30].map(hdpx) children = {  rendObj = ROBJ_TEXT text="This is long-long-long text"}}
      dText("This is clipped overflowed text")
      {rendObj = ROBJ_FRAME size = [100,30].map(hdpx) clipChildren = true children = {rendObj = ROBJ_TEXT text="This is long-long-long text"}}
      dText("This is ellipsis overflowed text, split by chars.")
      {rendObj = ROBJ_FRAME size = [210,30].map(hdpx) children = {rendObj = ROBJ_TEXT text="Съешь ещё этих мягких французских булок, да" ellipsis = true textOverflowX = TOVERFLOW_CHAR size = flex() }}
      dText("This is ellipsis overflowed (d)text, split by words.")
      {rendObj = ROBJ_FRAME size = [210,30].map(hdpx) children = {rendObj = ROBJ_TEXT text="Съешь ещё этих мягких французских булок, да" ellipsis = true textOverflowX = TOVERFLOW_WORD size = flex()}}
      dText("This is resizable overflowed text, split by chars")
      @(){ watch = textFrameState children = dTextFrame({fontFxColor = Color(255, 155, 0, 0) color = Color(255,255,255) size=textFrameState.get().size})}
      dText("This is autoscrolled text with delay and speed")
      {clipChildren = true size=static [hdpx(100),SIZE_TO_CONTENT]  children = {behavior = Behaviors.Marquee size = FLEX_H delay = 1 speed = hdpx(50) children = dText("All you need is love and autoscroll", {color=white})}}
      dText("This is autoscrolled text on Hover")
      {clipChildren = true size=static [hdpx(100),SIZE_TO_CONTENT]  children = {behavior = [Behaviors.Marquee, Behaviors.Button] scrollOnHover = true size = FLEX_H delay = 0.1 speed = hdpx(50) children = dText("All you need is love and autoscroll onHover", {color=white})}}
    ]
  }
}
