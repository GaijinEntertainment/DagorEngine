from "string" import format
from "%darg/ui_imports.nut" import *

let { send_touch_pad_gesture } = require("dargbox")
let cursors = require("samples_prog/_cursors.nut")

let gestureTypes = [
  { type = GESTURE_DETECTOR_TAP, name = "TAP", valueText = @(_evt) "" }
  { type = GESTURE_DETECTOR_DOUBLE_TAP, name = "DOUBLE_TAP", valueText = @(_evt) "" }
  { type = GESTURE_DETECTOR_HOLD, name = "HOLD", valueText = @(_evt) "" }
  { type = GESTURE_DETECTOR_DRAG, name = "DRAG", valueText = @(evt) format("drag %.0f, %.0f", evt.drag.x, evt.drag.y) }
  { type = GESTURE_DETECTOR_PINCH, name = "PINCH", valueText = @(evt) format("scale %.2f, dscale %.2f", evt.scale, evt.dscale) }
  { type = GESTURE_DETECTOR_ROTATE, name = "ROTATE", valueText = @(evt) format("angle %.2f, drotate %.2f", evt.rotate, evt.drotate) }
]

let keyGestures = [
  { key = "T", hint = "tap", events = [["tap", "begin"], ["tap", "end"]] }
  { key = "D", hint = "double tap", events = [["tap", "begin"], ["tap", "cancelled"], ["double_tap", "active"]] }
  { key = "H", hint = "hold", events = [["hold", "begin"], ["hold", "end"]] }
  { key = "G", hint = "drag", events = [["drag", "begin"], ["drag", "active", { x = 200.0, dx = 200.0 }], ["drag", "end", { x = 200.0 }]] }
  { key = "X", hint = "pinch out", events = [["pinch", "begin"], ["pinch", "active", { scale = 1.25 }], ["pinch", "end", { scale = 1.25 }]] }
  { key = "Z", hint = "pinch in", events = [["pinch", "begin"], ["pinch", "active", { scale = 0.8 }], ["pinch", "end", { scale = 0.8 }]] }
  { key = "E", hint = "rotate clockwise", events = [["rotate", "begin"], ["rotate", "active", { angle = 0.3 }], ["rotate", "end", { angle = 0.3 }]] }
  { key = "Q", hint = "rotate counterclockwise", events = [["rotate", "begin"], ["rotate", "active", { angle = -0.3 }], ["rotate", "end", { angle = -0.3 }]] }
]

function sendGestures(events) {
  foreach (e in events)
    send_touch_pad_gesture(e[0], e[1], e?[2] ?? {})
}

let activeColor = Color(255, 220, 100)
let idleColor = Color(200, 200, 200)

let overlay = {
  flow = FLOW_VERTICAL
  children = [
    {
      rendObj = ROBJ_TEXT
      text = "Touch pad gestures go to the element under the cursor, then to its parents, by their touchPadGestures."
    }
    {
      rendObj = ROBJ_TEXT
      text = "The panel takes DOUBLE_TAP only: a double tap on it stays there, other gestures go to the map."
    }
    {
      rendObj = ROBJ_TEXT
      color = Color(255, 150, 150)
      text = " ".join(keyGestures.map(@(k) $"{k.key}: {k.hint}."))
    }
  ]
}

function mkStats() {
  let res = {}
  foreach (g in gestureTypes)
    res[g.name] <- { isActive = false, begin = 0, update = 0, end = 0, value = "" }
  return res
}

function onGesture(stats, phase, evt) {
  let g = gestureTypes.findvalue(@(v) v.type == evt.type)
  if (g == null)
    return 0
  stats.mutate(function(s) {
    let st = s[g.name]
    st[phase]++
    st.isActive = phase != "end"
    st.value = g.valueText(evt)
  })
  return R_PROCESSED
}

function mkTypeRow(g, st) {
  return {
    rendObj = ROBJ_TEXT
    color = st.isActive ? activeColor : idleColor
    text = $"{g.name}: begin {st.begin}, update {st.update}, end {st.end}, {st.value}"
  }
}

function mkArea(title, gestures, fillColor, size, child = null) {
  let stats = Watched(mkStats())
  let types = gestureTypes.filter(@(g) (gestures & g.type) != 0)
  return {
    rendObj = ROBJ_BOX
    fillColor
    borderWidth = 2
    borderColor = Color(255, 255, 255)
    size
    padding = sh(2)
    gap = sh(1)
    flow = FLOW_VERTICAL
    behavior = Behaviors.ProcessGesture
    touchPadGestures = gestures
    onGestureBegin = @(evt) onGesture(stats, "begin", evt)
    onGestureActive = @(evt) onGesture(stats, "update", evt)
    onGestureEnd = @(evt) onGesture(stats, "end", evt)
    children = [
      { rendObj = ROBJ_TEXT, text = title, fontSize = sh(3) }
      @() {
        watch = stats
        flow = FLOW_VERTICAL
        gap = sh(0.5)
        children = types.map(@(g) mkTypeRow(g, stats.get()[g.name]))
      }
      child
    ]
  }
}

let panel = mkArea("Panel", GESTURE_DETECTOR_DOUBLE_TAP, Color(40, 70, 130), [pw(60), flex()])

let map = mkArea("Map",
  GESTURE_DETECTOR_TAP | GESTURE_DETECTOR_DOUBLE_TAP | GESTURE_DETECTOR_HOLD | GESTURE_DETECTOR_DRAG
    | GESTURE_DETECTOR_PINCH | GESTURE_DETECTOR_ROTATE,
  Color(50, 110, 40), const [pw(90), ph(75)], panel)

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(60, 70, 80)
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  gap = sh(2)
  hotkeys = keyGestures.map(@(k) [$"^{k.key}", @() sendGestures(k.events)])
  children = [
    overlay
    map
  ]
  cursor = cursors.normal
}
