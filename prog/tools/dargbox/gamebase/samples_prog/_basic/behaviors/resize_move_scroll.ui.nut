from "%darg/ui_imports.nut" import *
from "math" import max
/*
  KNOWN ISSUES:
    - vertical align not working for text area       valign = ALIGN_TOP | ALIGN_BOTTOM | ALIGN_CENTER
    - ellipsis should be done by character by default (or may be even always)

*/

let cursors = require("samples_prog/_cursors.nut")

let textFish = @"Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Sed efficitur felis tortor, sed finibus elit hendrerit et. Vivamus in tortor sagittis, tempor turpis a, laoreet augue. Aliquam maximus scelerisque elit, a elementum nulla ullamcorper eget. Sed fermentum sit amet leo eget semper. Sed at turpis sed massa fermentum dignissim. Vestibulum condimentum viverra consectetur. Donec consequat egestas efficitur. Sed id massa ligula. Nulla vel neque rutrum, imperdiet ante at, aliquet nunc. Curabitur faucibus odio sed lorem fermentum consequat. Praesent at vulputate libero. Vestibulum ultrices ligula semper, sagittis nulla scelerisque, finibus lectus. Vivamus non cursus massa, sed placerat massa. Cras a gravida neque. Maecenas tempus ac ante quis aliquet.
Donec urna nisi, cursus eget mauris at, congue bibendum mauris. Nunc porta dui in auctor sagittis. Integer et magna in quam tristique efficitur nec vel mauris. Maecenas lobortis, turpis ut hendrerit posuere, arcu velit euismod mauris, et pretium sem dolor eget nisl. Fusce nisl mauris, varius et iaculis quis, lobortis quis mauris. Mauris dapibus fringilla turpis viverra dignissim. Donec mattis in erat eu ornare. Donec scelerisque et augue pretium lobortis. Pellentesque consectetur nunc magna, vel scelerisque diam dictum id. Nam scelerisque luctus mollis. Curabitur dignissim dolor ut arcu lacinia, id finibus dui tincidunt.
Quisque ipsum purus,  hendrerit eget  ex  eget, tempus  condimentum ligula. Morbi sed urna felis. Integer semper sollicitudin eros at aliquam. Aliquam erat volutpat. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque laoreet est in nisi euismod commodo. Sed at tellus at lacus ultricies tempor nec vel sem. Quisque urna nisi, pretium et dolor ac, tempus laoreet lorem. Aenean magna ante, tempus id lacinia non, faucibus a ante. Integer condimentum et libero at blandit. Aenean in elementum est.
"

function sText(text, params={}) {
  return {
    rendObj = ROBJ_TEXT
    ellipsis = true
    color = Color(198,198,128)
    textOverflowX = TOVERFLOW_CHAR
    text
  }.__update(params, {rendObj = ROBJ_TEXT})
}


let textAreaFrameState_default = {
  size = static [sw(40), 60]
  pos  = [100, 100]
}

let state = mkWatched(persist,"state")

function textAreaContainer() {
  let w = state.get() ?? textAreaFrameState_default
  return {
    rendObj = ROBJ_BOX
    borderColor = Color(200,100,100)
    fillColor = Color(10,40,40,120)

    children = {
      rendObj = ROBJ_TEXTAREA
      textOverflowY = TOVERFLOW_LINE
      halign = ALIGN_LEFT
      ellipsis = true
      ellipsisSepLine = false
      text = textFish
      behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
      size = flex()
    }
    padding = 5
    size = w.size
    pos  = w.pos
    valign = ALIGN_TOP
    behavior = Behaviors.MoveResize
    moveResizeCursors = cursors.moveResizeCursors
    watch = [state]
    onMoveResize = function(dx, dy, dw, dh) {
      let n = {size = [max(5, w.size[0]+dw), max(20, w.size[1]+dh)] pos = [w.pos[0]+dx, w.pos[1]+dy]}
      state.set(n)
      return n
    }
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(80,80,80)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = flex()
  cursor = cursors.normal
  children = {
//    flow = FLOW_VERTICAL
//    halign = ALIGN_CENTER
//    valign = ALIGN_CENTER
    size = static [sw(80),sh(80)]
    gap = 10
    children = [
      sText("Resizble, moveble and Scrollable by wheel text area with split by line")
      textAreaContainer
    ]
  }
}
