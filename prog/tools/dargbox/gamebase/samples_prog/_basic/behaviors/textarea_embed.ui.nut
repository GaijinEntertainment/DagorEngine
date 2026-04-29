from "%darg/ui_imports.nut" import *
from "math" import max

let cursors = require("samples_prog/_cursors.nut")

/*
let editableText = EditableText(@"Lorem ipsum

dolor")
*/


let text = @"Lorem ipsum <pic1/> felis
tortor, sed finibus elit hendrerit et. <canvas/>Vivamus in tortor sagittis."


let textarea = {
  size = flex()
  rendObj = ROBJ_TEXTAREA
  behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
  color = Color(150,150,120)

  text

  embed = {
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
