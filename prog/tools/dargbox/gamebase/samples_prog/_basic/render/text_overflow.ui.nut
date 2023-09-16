from "%darg/ui_imports.nut" import *

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
  size   = [107, 30]
  pos    = [0, 0]
})

let function text_element(text) {
  return {
    rendObj = ROBJ_TEXT
    ellipsis = true
    color = Color(198,198,128)
    textOverflowX = TOVERFLOW_CHAR
  }.__update({text=text})
}

let function dText(text,params={}) {
  return text_element(text).__update(params).__update({rendObj = ROBJ_TEXT})
}

let function sText(text, params={}) {
  return text_element(text).__update(params).__update({rendObj = ROBJ_INSCRIPTION})
}

let function dTextFrame(params={}) {
  let text = dText("Съешь ещё этих мягких французских булок, да выпей же чаю").__update(params).__update({size=flex()})

  return @() {
    rendObj = ROBJ_FRAME
    color = Color(255,0,0)
    size = textFrameState.value.size
    pos = textFrameState.value.pos
    children = text
    behavior = Behaviors.MoveResize
    moveResizeCursors = cursors.moveResizeCursors
    watch = textFrameState

    function onMoveResize(dx, dy, dw, dh) {
      let w = textFrameState.value
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
    size = [sw(80),sh(80)]
    gap = 10
    children = [
      sText("This is auto-clipped overflowed text")
      {rendObj = ROBJ_FRAME size = [100,30].map(hdpx) children = {size =flex() rendObj = ROBJ_TEXT text="This is long-long-long text"}}
      sText("This is nonlcipped overflowed text (text element has size by content and parent is not clipping children)")
      {rendObj = ROBJ_FRAME  size = [100,30].map(hdpx) children = {  rendObj = ROBJ_TEXT text="This is long-long-long text"}}
      sText("This is clipped overflowed text")
      {rendObj = ROBJ_FRAME size = [100,30].map(hdpx) clipChildren = true children = {rendObj = ROBJ_TEXT text="This is long-long-long text"}}
      sText("This is ellipsis overflowed text, split by chars. It works currently only with rendObj=ROBJ_TEXT, and you need to set some size for text, otherwise it is SIZE_TO_CONTENT")
      {rendObj = ROBJ_FRAME size = [150,30].map(hdpx) children = {rendObj = ROBJ_TEXT text="Съешь ещё этих мягких французских булок, да" ellipsis = true textOverflowX = TOVERFLOW_CHAR size = flex() }}
      sText("This is ellipsis overflowed (d)text, split by words.")
      {rendObj = ROBJ_FRAME size = [150,30].map(hdpx) children = {rendObj = ROBJ_TEXT text="Съешь ещё этих мягких французских булок, да" ellipsis = true textOverflowX = TOVERFLOW_WORD size = flex()}}
      sText("This is resizable overflowed text, split by chars")
      dTextFrame({fontFxColor = Color(255, 155, 0, 0) color = Color(255,255,255) size=textFrameState.value.size})
      sText("This is autoscrolled text with dealy and speed")
      sText("All you need is love and autoscroll" {behavior = Behaviors.Marquee delay = 1 speed = 50 size=[hdpx(100),SIZE_TO_CONTENT] color=white })
      sText("This is autoscrolled text on Hover")
      sText("All you need is love and autoscroll on Hover" {behavior = [Behaviors.Marquee, Behaviors.Button] color=white delay = 0.5 speed = [50,600] scrollOnHover = true size=[hdpx(100),SIZE_TO_CONTENT]})
      sText("This is DTEXT autoscrolled text on Hover, not WORKING", {color = Color(255,0,0)})
      {behavior = [Behaviors.Marquee, Behaviors.Button] clipChildren=true color=white delay = 0.5 speed = [50,600] scrollOnHover = true size=[hdpx(100),SIZE_TO_CONTENT] children = dText("All you need is love and autoscroll on Hover" {color=white size=SIZE_TO_CONTENT})}
    ]
  }
}
