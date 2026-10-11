from "string" import format
from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let gestureTypes = [
  { type = GESTURE_DETECTOR_PINCH, name = "PINCH", valueText = @(evt) format("scale %.2f", evt.scale) }
  { type = GESTURE_DETECTOR_ROTATE, name = "ROTATE", valueText = @(evt) format("angle %.2f", evt.rotate) }
  { type = GESTURE_DETECTOR_DRAG, name = "DRAG", valueText = @(evt) format("drag %.0f, %.0f", evt.drag.x, evt.drag.y) }
]

let activeColor = Color(255, 220, 100)
let idleColor = Color(200, 200, 200)

let overlay = {
  flow = FLOW_VERTICAL
  children = [
    {
      rendObj = ROBJ_TEXT
      text = "Both areas get every touch, and each area must react by its own limits."
    }
    {
      rendObj = ROBJ_TEXT
      text = "Under 100 px apart at the start: A drags, pinches and rotates, B only drags; over 100 px: A pinches and rotates, B does all three (drags under 500 px)."
    }
    {
      rendObj = ROBJ_TEXT
      text = "After the release every row shows begin == end, and no row stays lit."
    }
    {
      rendObj = ROBJ_TEXT
      color = Color(255, 150, 150)
      text = "On Windows use two fingers on the DualSense touchpad, or on a laptop touchpad with `touchpadAsTouch:b=yes` in settings.blk."
    }
    {
      rendObj = ROBJ_TEXT
      color = Color(255, 150, 150)
      text = "Mouse: with `touchScreen:b=yes` and `emuTouchScreenWithMouse:b=yes` hold R_MOUSE_BTN, move away, hold L_MOUSE_BTN and move."
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
    return
  stats.mutate(function(s) {
    let st = s[g.name]
    st[phase]++
    st.isActive = phase != "end"
    st.value = g.valueText(evt)
  })
}

function mkTypeRow(g, st) {
  return {
    rendObj = ROBJ_TEXT
    color = st.isActive ? activeColor : idleColor
    text = $"{g.name}: begin {st.begin}, update {st.update}, end {st.end}, {st.value}"
  }
}

function mkArea(title, limits, fillColor) {
  let stats = Watched(mkStats())
  let limitRows = limits.len() == 0
    ? [{ rendObj = ROBJ_TEXT, text = "default limits" }]
    : limits.keys().sort().map(@(k) { rendObj = ROBJ_TEXT, text = $"{k} = {limits[k]}" })

  return {
    ...limits
    rendObj = ROBJ_BOX
    fillColor
    borderWidth = 2
    borderColor = Color(255, 255, 255)
    size = flex()
    padding = sh(2)
    gap = sh(1)
    flow = FLOW_VERTICAL
    behavior = Behaviors.ProcessGesture
    onGestureBegin = @(evt) onGesture(stats, "begin", evt)
    onGestureActive = @(evt) onGesture(stats, "update", evt)
    onGestureEnd = @(evt) onGesture(stats, "end", evt)
    children = [
      { rendObj = ROBJ_TEXT, text = title, fontSize = sh(3) }
      { flow = FLOW_VERTICAL, children = limitRows }
      @() {
        watch = stats
        flow = FLOW_VERTICAL
        gap = sh(0.5)
        children = gestureTypes.map(@(g) mkTypeRow(g, stats.get()[g.name]))
      }
    ]
  }
}

let areas = {
  size = const [pw(95), ph(75)]
  flow = FLOW_HORIZONTAL
  gap = sh(2)
  children = [
    mkArea("A", {}, Color(50, 110, 40))
    mkArea("B", { gestureDragDistanceMax = 500, gesturePinchDistanceMin = 100, gestureRotateDistanceMin = 100 },
      Color(40, 70, 130))
  ]
}

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(60, 70, 80)
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  gap = sh(2)
  children = [
    overlay
    areas
  ]
  cursor = cursors.normal
}
