from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

// Visual check of Oklch color interpolation. Every swatch returns a constant from its
// easing function, freezing it at one point of the ramp, so one screenshot shows the
// whole interpolation. Rows are: animation / transition / plain sRGB lerp in script.

let STEPS = 11
let SWATCH_W = hdpx(46)
let ENGINE_H = hdpx(30)
let NAIVE_H = hdpx(16)
let LABEL_W = hdpx(190)

let pairs = [
  { name = "red -> green", from = [255, 0, 0, 255], to = [0, 255, 0, 255] }
  { name = "red -> blue", from = [255, 0, 0, 255], to = [0, 0, 255, 255] }
  { name = "yellow -> blue", from = [255, 255, 0, 255], to = [0, 0, 255, 255] }
  { name = "white -> black", from = [255, 255, 255, 255], to = [0, 0, 0, 255] }
  { name = "black -> red", from = [0, 0, 0, 255], to = [255, 0, 0, 255] }
  { name = "grey -> blue", from = [128, 128, 128, 255], to = [0, 0, 255, 255] }
  { name = "ui idle -> hover", from = [60, 60, 60, 255], to = [100, 150, 200, 255] }
  { name = "transparent -> red", from = [0, 0, 0, 0], to = [255, 0, 0, 255] }
]

function toColor(c) {
  return Color(c[0], c[1], c[2], c[3])
}

function mkEngineSwatch(from, to, k) {
  return {
    size = [SWATCH_W, ENGINE_H]
    rendObj = ROBJ_SOLID
    color = toColor(from)
    animations = [
      { prop = AnimProp.color, from = toColor(from), to = toColor(to), duration = 1.0, play = true, loop = true,
        easing = @(_) k }
    ]
  }
}

function mkNaiveSwatch(from, to, k) {
  return {
    size = [SWATCH_W, NAIVE_H]
    rendObj = ROBJ_SOLID
    color = Color(from[0] + (to[0] - from[0]) * k, from[1] + (to[1] - from[1]) * k, from[2] + (to[2] - from[2]) * k,
      from[3] + (to[3] - from[3]) * k)
  }
}

// transitions are a separate code path; flip the script color once to start them
let transitionPhase = Watched(false)
let startTransition = @() transitionPhase.set(true)

function mkTransitionSwatch(from, to, k) {
  return function() {
    return {
      watch = transitionPhase
      size = [SWATCH_W, ENGINE_H]
      rendObj = ROBJ_SOLID
      color = toColor(transitionPhase.get() ? to : from)
      // duration long enough that it never finishes and resets
      transitions = [{ prop = AnimProp.color, duration = 1000.0, easing = @(_) k }]
    }
  }
}

function mkStrip(mkSwatch, p) {
  let children = []
  for (local i = 0; i < STEPS; ++i)
    children.append(mkSwatch(p.from, p.to, i.tofloat() / (STEPS - 1)))
  return { flow = FLOW_HORIZONTAL, children }
}

function mkRow(p) {
  return {
    flow = FLOW_HORIZONTAL
    gap = hdpx(8)
    valign = ALIGN_CENTER
    children = [
      { size = [LABEL_W, SIZE_TO_CONTENT], rendObj = ROBJ_TEXT, text = p.name, color = Color(200, 200, 200) }
      {
        // mid grey so that partial alpha is visible
        rendObj = ROBJ_SOLID
        color = Color(110, 110, 110)
        size = SIZE_TO_CONTENT
        flow = FLOW_VERTICAL
        children = [mkStrip(mkEngineSwatch, p), mkStrip(mkTransitionSwatch, p), mkStrip(mkNaiveSwatch, p)]
      }
    ]
  }
}

return {
  rendObj = ROBJ_SOLID
  size = flex()
  color = Color(30, 40, 50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  padding = hdpx(20)

  children = {
    flow = FLOW_VERTICAL
    gap = hdpx(10)
    size = SIZE_TO_CONTENT

    onAttach = @() gui_scene.setTimeout(0.5, startTransition)
    onDetach = @() gui_scene.clearTimer(startTransition)

    children = [
      { rendObj = ROBJ_TEXT, text = "per row: engine animation / engine transition / plain sRGB byte lerp",
        color = Color(160, 170, 180) }
    ].extend(pairs.map(mkRow))
  }
}
