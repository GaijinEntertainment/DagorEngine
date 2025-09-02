from "math" import abs
from "%darg/ui_imports.nut" import *
let { gradSize, mkRadialGradient } = require("radial_gradient.nut")
let textInput = require("samples_prog/_basic/components/textInput.nut")

let color1 = 0xFFFF8000
let color2 = 0xFF0080FF
let exampleSize = hdpx(100)

let radius = mkWatched(persist, "radius", 10)
let width = mkWatched(persist, "width", 20)
let centerX = mkWatched(persist, "centerX", 20)
let centerY = mkWatched(persist, "centerY", 15)

let mkImage = @(r, w, cx, cy) mkRadialGradient(color1, color2, r, w, cx, cy)

let curImage = Computed(@() mkImage(radius.get(), width.get(), centerX.get(), centerY.get()))

let optionsCfg = [
  { watch = radius, text = "radius" }
  { watch = width, text = "width" }
  { watch = centerX, text = "centerX" }
  { watch = centerY, text = "centerY" }
]

let examplesList = [
  mkImage(0, gradSize, 0, 0)
  mkImage(20, 20, 20, 20)
  mkImage(50, 10, 10, 50)
]

let examples = {
  flow = FLOW_HORIZONTAL
  gap = exampleSize / 10
  children = examplesList.map(@(image) {
    size = [exampleSize, exampleSize]
    rendObj = ROBJ_IMAGE
    color = 0xFFFFFFFF
    image
  })
}

let withText = @(text, component, size = SIZE_TO_CONTENT, textOvr = {}) {
  size
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = hdpx(5)
  children = [
    {
      rendObj = ROBJ_TEXT
      text
      color = 0xFFFFFFFF
    }.__update(textOvr)
    component
  ]
}

let curImagePixelPerfect = @() {
  watch = curImage
  size = [gradSize, gradSize]
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = curImage.get()
}

let curImageBig = @() {
  watch = curImage
  size = flex()
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = curImage.get()
  keepAspect = true
  imageHalign = ALIGN_CENTER
  imageValign = ALIGN_CENTER
}

let mkPoint = @(x, y) {
  size = 6
  pos = [pw(100.0 * (x - 0.5)), ph(100.0 * (y - 0.5))]
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  rendObj = ROBJ_SOLID
  color = 0xFFFF0000
}

let gradSizeDiv = 1.0 / gradSize
let curImageWithPoints = @() {
  watch = [curImage, centerX, centerY]
  size = flex()
  maxWidth = ph(100)
  maxHeight = pw(100)
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = curImage.get()

  children = mkPoint(gradSizeDiv * centerX.get(), gradSizeDiv * centerY.get())
}

function sqBtn(text, onClick) {
  let stateFlags = Watched(0)
  return @() {
    watch = stateFlags
    size = ph(100)
    rendObj = ROBJ_BOX
    fillColor = stateFlags.get() & S_HOVER ? 0xFF404060 : 0xFF404040
    borderColor = stateFlags.get() & S_ACTIVE ? 0xFFFFFFFF : 0xFFA0A0A0
    borderWidth = hdpx(1)
    behavior = Behaviors.Button
    onElemState = @(sf) stateFlags.set(sf)
    onClick
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = { rendObj = ROBJ_TEXT, text }
  }
}

function mkControl(watch, hasNegative) {
  let setValue = @(v) watch.set(hasNegative ? v : abs(v))
  return {
    size = FLEX_H
    flow = FLOW_HORIZONTAL
    gap = hdpx(5)
    children = [
      textInput(watch, {
        margin = 0
        charMask = "-0123456789"
        setValue = @(v) v == "" ? null :  setValue(v.tointeger())
      })
      sqBtn("-", @() setValue(watch.get() - 1))
      sqBtn("+", @() setValue(watch.get() + 1))
    ]
  }
}

let textStyle = { hplace = ALIGN_LEFT, color = 0xFFA0A0A0 }
let options = {
  size = static [hdpx(250), flex()]
  flow = FLOW_VERTICAL
  gap = hdpx(20)
  children = optionsCfg.map(function(o) {
    let { watch, text, hasNegative = false } = o
    return withText(text, mkControl(watch, hasNegative), FLEX_H, textStyle)
  })
}

return {
  size = flex()
  padding = hdpx(30)
  flow = FLOW_VERTICAL
  gap = hdpx(30)
  children = [
    examples
    {
      size = flex()
      flow = FLOW_HORIZONTAL
      gap = hdpx(30)
      children = [
        options
        withText("Pixel perfect", curImagePixelPerfect)
        withText("Scaled", curImageWithPoints, flex())
        withText("Big scaled", curImageBig, flex(3))
      ]
    }
  ]
}