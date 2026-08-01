from "%darg/ui_imports.nut" import *
from "math" import max

let cursors = require("samples_prog/_cursors.nut")

let text = @"Lorem ipsum <pic1/> felis tortor,
sed finibus <pic1/> elit hendrerit et. <canvas/>Vivamus in tortor <button/> sagittis."

let defBtnStyle = { color = Color(120, 150, 150) }

// Height is the font ascent: embeds anchor bottom-to-baseline, so this aligns
// the button's text baseline with the line's.
function mkEmbedButton(label, onClick, style = defBtnStyle) {
  return {
    rendObj = ROBJ_TEXT
    text = label
    size = [calc_str_box(label, style)[0], get_font_metrics(style?.font ?? 0, style?.fontSize ?? 0).ascent]
    behavior = Behaviors.Button
    onClick
  }.__merge(style)
}

let button = mkEmbedButton("button", @() dlog("clicked"))

let textarea = {
  size = flex()
  rendObj = ROBJ_TEXTAREA
  behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
  color = Color(150,150,120)

  text

  embed = {
    button
    pic1 = {
      size = [fontH(150), fontH(150)]
      rendObj = ROBJ_IMAGE
      image = Picture("!ui/atlas#ca_cup1")
    }
    canvas = {
      rendObj = ROBJ_VECTOR_CANVAS
      size = [sh(5), sh(5)]
      lineWidth = hdpx(2.5)
      color = Color(50, 200, 255)
      fillColor = Color(122, 1, 0, 0)
      commands = [
        [VECTOR_ELLIPSE, 50, 50, 50, 50],
      ]
    }
  }
}

let textAreaFrameState = Watched({
  size = const [sh(80), sh(50)]
  pos  = [sh(10), sh(10)]
})


return {
  rendObj = ROBJ_SOLID
  color = Color(80,80,80)
  cursor = cursors.normal
  size = flex()
  children = [
    @() {
      rendObj = ROBJ_BOX
      borderWidth = 2
      padding = hdpx(10)
      borderColor = Color(200, 200, 200)
      fillColor = Color(30, 40, 50)

      pos = textAreaFrameState.get().pos
      size = textAreaFrameState.get().size
      behavior = Behaviors.MoveResize
      moveResizeCursors = cursors.moveResizeCursors
      watch = textAreaFrameState
      onMoveResize = function(dx, dy, dw, dh) {
        let w = textAreaFrameState.get()
        w.pos = [w.pos[0]+dx, w.pos[1]+dy]
        w.size = [max(5, w.size[0]+dw), max(20, w.size[1]+dh)]
        return w
      }

      children = textarea
    }
  ]
}
