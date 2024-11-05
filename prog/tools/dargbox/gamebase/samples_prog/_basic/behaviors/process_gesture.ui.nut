from "%darg/ui_imports.nut" import *
from "math" import sqrt, floor

let cursors = require("samples_prog/_cursors.nut")

let processorState = Watched({
  scale = null
  rotate = null
  drag = null
})

let overlay = {
  flow = FLOW_VERTICAL
  children = [
    {
      rendObj = ROBJ_TEXT
      color = Color(255,150,150)
      text = "NOTE! For now it should work only on touch devices."
    }
    {
      rendObj = ROBJ_TEXT
      text = "You can try to run it on PC with `touchScreen:b=yes` and `emuTouchScreenWithMouse:b=yes` in settings.blk"
    }
    {
      rendObj = ROBJ_TEXT
      text = "To emulate gesture press R_MOUSE_BTN, move the pointer a little away and press L_MOUSE_BTN"
    }
  ]
}

let processor = @() {
  watch = processorState
  rendObj = ROBJ_BOX
  fillColor = Color(50,150,25)
  borderWidth = 2
  borderColor = Color(255,255,255)
  size = [pw(80), ph(80)]
  flow = FLOW_VERTICAL
  behavior = Behaviors.ProcessGesture

  function onGestureBegin(evt) {
    dlog($"Gesture | begin, type={evt.type}, r={evt.r}")
  }

  function onGestureActive(evt) {
    if (evt.type == GESTURE_DETECTOR_PINCH) processorState.mutate(function(v) {
      v.scale = evt.scale
    })
    else if (evt.type == GESTURE_DETECTOR_ROTATE) processorState.mutate(function(v) {
      v.rotate = evt.rotate
    })
    else if (evt.type == GESTURE_DETECTOR_DRAG) processorState.mutate(function(v) {
      v.drag = evt.drag
    })
  }

  function onGestureEnd(evt) {
    dlog($"Gesture | end, type={evt.type}")
    if (evt.type == GESTURE_DETECTOR_PINCH) processorState.mutate(function(v) {
      v.scale = null
    })
    else if (evt.type == GESTURE_DETECTOR_ROTATE) processorState.mutate(function(v) {
      v.rotate = null
    })
    else if (evt.type == GESTURE_DETECTOR_DRAG) processorState.mutate(function(v) {
      v.drag = null
    })
  }

  children = [
    processorState.value.scale == null
    ? null
    : {
      rendObj = ROBJ_TEXT
      text=$"Gesture PINCH = {processorState.value.scale}"
      fontSize=sh(4)
      padding=sh(2)
    }
    processorState.value.rotate == null
    ? null
    : {
      rendObj = ROBJ_TEXT
      text=$"Gesture ROTATE = {processorState.value.rotate}"
      fontSize=sh(4)
      padding=sh(2)
    }
    processorState.value.drag == null
    ? null
    : {
      rendObj = ROBJ_TEXT
      text=$"Gesture DRAG = {processorState.value.drag}"
      fontSize=sh(4)
      padding=sh(2)
    }
  ]
}

let root = {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(60,70,80)

  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    overlay
    processor
  ]

  cursor = cursors.normal
}

return root
