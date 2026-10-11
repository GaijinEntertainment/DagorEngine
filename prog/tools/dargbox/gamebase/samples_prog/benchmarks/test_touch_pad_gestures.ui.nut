// Regression test for touch pad gestures in Behaviors.ProcessGesture, sent through
// dargbox.send_touch_pad_gesture as a pad driver would send them. The hit element and
// then its parents are asked at BEGIN, the element that takes it keeps the gesture,
// a DOUBLE_TAP (one ACTIVE event) is a gesture of its own, and touchPadGestures filters
// the types.
// A taker that goes away gets CANCELLED, which ProcessGesture gives to onGestureEnd.
//
// Prints TEST_RESULT ok / TEST_RESULT fail:<reason> and exits with 1 on failure;
// see MULTI_FRAME_TESTS in test.py.
from "string" import format
from "dagor.math" import Point2
from "%darg/ui_imports.nut" import *

let { exit } = require("dagor.system")
let { send_touch_pad_gesture } = require("dargbox")

const FRAMES_PER_STEP = 3

gui_scene.setConfigProps({ touchPadWidthInScreenHeights = 2.0 })

let typeNames = {
  [GESTURE_DETECTOR_TAP] = "TAP",
  [GESTURE_DETECTOR_DOUBLE_TAP] = "DOUBLE_TAP",
  [GESTURE_DETECTOR_HOLD] = "HOLD",
  [GESTURE_DETECTOR_PINCH] = "PINCH",
  [GESTURE_DETECTOR_ROTATE] = "ROTATE",
}

let received = []
let showOuter = Watched(true)
local innerElem = null

function describe(name, phase, evt) {
  let gestureType = evt.type
  let values = gestureType == GESTURE_DETECTOR_PINCH ? format(" %.2f %.2f %.2f", evt.scale, evt.dscale, evt.dx / sh(100))
    : gestureType == GESTURE_DETECTOR_ROTATE ? format(" %.2f %.2f", evt.rotate, evt.drotate)
    : ""
  let isValid = evt.devId == DEVID_JOYSTICK && evt.hit && evt.targetRect.r > evt.targetRect.l
  let validity = isValid ? "" : " invalid"
  return $"{name} {phase} {typeNames[gestureType]}{values}{validity}"
}

function mkHandler(name, phase, result) {
  return function(evt) {
    received.append(describe(name, phase, evt))
    return result
  }
}

let inner = {
  size = flex()
  behavior = Behaviors.ProcessGesture
  touchPadGestures = GESTURE_DETECTOR_DOUBLE_TAP | GESTURE_DETECTOR_HOLD
  onGestureBegin = mkHandler("inner", "begin", 0)
  onGestureEnd = mkHandler("inner", "end", R_PROCESSED)
  function onAttach(elem) {
    innerElem = elem
  }
}

let outer = {
  size = flex()
  behavior = Behaviors.ProcessGesture
  touchPadGestures = GESTURE_DETECTOR_TAP | GESTURE_DETECTOR_HOLD | GESTURE_DETECTOR_PINCH | GESTURE_DETECTOR_ROTATE
  onGestureBegin = mkHandler("outer", "begin", R_PROCESSED)
  onGestureActive = mkHandler("outer", "active", R_PROCESSED)
  onGestureEnd = mkHandler("outer", "end", R_PROCESSED)
  children = inner
}

function sendAll(gestureType, gestures) {
  let cursor = Point2(sw(50), sh(50))
  foreach (g in gestures)
    send_touch_pad_gesture(gestureType, g[0], { ...g?[1], cursor })
}

let steps = [
  @() sendAll("tap", [["begin"], ["end"]])
  @() sendAll("double_tap", [["active"]])
  @() sendAll("hold", [["begin"], ["active"], ["end"]])
  @() sendAll("pinch", [["begin"], ["active", { scale = 1.5, dx = 960.0 }], ["active", { scale = 2.0 }], ["end", { scale = 2.0 }]])
  @() sendAll("rotate", [["begin"], ["active", { angle = 0.5 }]])
  @() showOuter.set(false)
  @() sendAll("rotate", [["active", { angle = 0.7 }], ["end", { angle = 0.7 }]])
]

let expected = [
  "outer end TAP"
  "inner end DOUBLE_TAP"
  "inner begin HOLD"
  "outer begin HOLD"
  "outer end HOLD"
  "outer begin PINCH 1.00 0.00 0.00"
  "outer active PINCH 1.50 0.50 1.00"
  "outer active PINCH 2.00 0.50 0.00"
  "outer end PINCH 2.00 0.00 0.00"
  "outer begin ROTATE 0.00 0.00"
  "outer active ROTATE 0.50 0.50"
  "outer end ROTATE 0.50 0.00"
]

function check() {
  foreach (i, want in expected)
    if (received?[i] != want)
      return $"event {i}: expected '{want}', got '{received?[i]}'"
  if (received.len() > expected.len())
    return $"unexpected event '{received[expected.len()]}'"
  return null
}

local frame = 0
local startFrame = null
gui_scene.setUpdateHandler(function(_dt) {
  frame++
  if (startFrame == null) {
    // the hit test needs the screen boxes of the first render
    if ((innerElem?.getTransformedRect().r ?? -1) > 0)
      startFrame = frame
    return
  }
  let elapsed = frame - startFrame
  if (elapsed % FRAMES_PER_STEP != 0)
    return
  let step = elapsed / FRAMES_PER_STEP - 1
  if (step < steps.len()) {
    steps[step]()
    return
  }
  let failed = check()
  println(failed == null ? "TEST_RESULT ok" : $"TEST_RESULT fail:{failed}")
  exit(failed == null ? 0 : 1)
})

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  children = @() {
    watch = showOuter
    size = flex()
    children = showOuter.get() ? outer : null
  }
}
