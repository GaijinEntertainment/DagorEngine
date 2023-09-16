from "%darg/ui_imports.nut" import *
let { abs } = require("math")
let { mkInclineGradient, gradSize } = require("incline_gradient.nut")
let textInput = require("samples_prog/_basic/components/textInput.nut")

let color1 = 0xFFFF8000
let color2 = 0xFF0080FF
let exampleSize = hdpx(100)

let x1 = mkWatched(persist, "x1", 0.25)
let y1 = mkWatched(persist, "y1", 0.25)
let x2 = mkWatched(persist, "x2", 0.75)
let y2 = mkWatched(persist, "y2", 0.75)

let mkInclineColored = @(x1v, y1v, x2v, y2v) mkInclineGradient(color1, color2, x1v, y1v, x2v, y2v)
let curImage = Computed(@() mkInclineColored(x1.value, y1.value, x2.value, y2.value))

let optionsCfg = [
  { watch = x1, text = "x1" }
  { watch = y1, text = "y1" }
  { watch = x2, text = "x2" }
  { watch = y2, text = "y2" }
]

let flaresExamplesList = [
  mkInclineColored(0.45, 0.5, 0.55, 0.5)
  mkInclineColored(0.25, 0.5, 0.75, 0.5)

  mkInclineColored(0.45, 0.45, 0.55, 0.55)
]

let examples = {
  flow = FLOW_HORIZONTAL
  gap = exampleSize / 10
  children = flaresExamplesList.map(@(image) {
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
  image = curImage.value
}

let curImageBig = @() {
  watch = curImage
  size = flex()
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = curImage.value
  keepAspect = true
  imageHalign = ALIGN_CENTER
  imageValign = ALIGN_CENTER
}

let mkPoint = @(x, y) {
  size = [6, 6]
  pos = [pw(100.0 * (x - 0.5)), ph(100.0 * (y - 0.5))]
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  rendObj = ROBJ_SOLID
  color = 0xFFFF0000
}

let curImageWithPoints = @() {
  watch = [x1, y1, x2, y2, curImage]
  size = flex()
  maxWidth = ph(100)
  maxHeight = pw(100)
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = curImage.value

  children = [
    mkPoint(x1.value, y1.value)
    mkPoint(x2.value, y2.value)
  ]
}

let function sqBtn(text, onClick) {
  let stateFlags = Watched(0)
  return @() {
    watch = stateFlags
    size = [ph(100), ph(100)]
    rendObj = ROBJ_BOX
    fillColor = stateFlags.value & S_HOVER ? 0xFF404060 : 0xFF404040
    borderColor = stateFlags.value & S_ACTIVE ? 0xFFFFFFFF : 0xFFA0A0A0
    borderWidth = hdpx(1)
    behavior = Behaviors.Button
    onElemState = @(sf) stateFlags(sf)
    onClick
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = { rendObj = ROBJ_TEXT, text }
  }
}

let mkControl = @(watch) {
  size = [flex(), SIZE_TO_CONTENT]
  flow = FLOW_HORIZONTAL
  gap = hdpx(5)
  children = [
    textInput(watch, {
      margin = 0
      charMask = "-.0123456789"
      setValue = @(v) v == "" ? null : watch(v.tofloat())
    })
    sqBtn("-", @() watch(watch.value - 0.01))
    sqBtn("+", @() watch(watch.value + 0.01))
  ]
}

let textStyle = { hplace = ALIGN_LEFT, color = 0xFFA0A0A0 }
let options = {
  size = [hdpx(250), flex()]
  flow = FLOW_VERTICAL
  gap = hdpx(20)
  children = optionsCfg.map(function(o) {
    let { watch, text } = o
    return withText(text, mkControl(watch), [flex(), SIZE_TO_CONTENT], textStyle)
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