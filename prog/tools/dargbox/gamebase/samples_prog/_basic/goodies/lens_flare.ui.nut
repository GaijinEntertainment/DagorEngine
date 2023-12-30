from "%darg/ui_imports.nut" import *
let { abs } = require("math")
let { mkLensFlareCutRadiusLeft } = require("lens_flare.nut")
let textInput = require("samples_prog/_basic/components/textInput.nut")

let radius = mkWatched(persist, "radius", 50)
let outherWidth = mkWatched(persist, "outherWidth", 3)
let innerWidth = mkWatched(persist, "innerWidth", 30)
let cutRadius = mkWatched(persist, "cutRadius", 75)
let cutWidth = mkWatched(persist, "cutWidth", 12)
let cutOffset = mkWatched(persist, "cutOffset", 15)

let lensImage = Computed(@() mkLensFlareCutRadiusLeft(radius.value, outherWidth.value,
  innerWidth.value, cutRadius.value, cutWidth.value, cutOffset.value))

let optionsCfg = [
  { watch = radius, text = "radius" }
  { watch = outherWidth, text = "outherWidth" }
  { watch = innerWidth, text = "innerWidth" }
  { watch = cutRadius, text = "cutRadius (can be negative)", hasNegative = true }
  { watch = cutWidth, text = "cutWidth" }
  { watch = cutOffset, text = "cutOffset  (can be negative)", hasNegative = true }
]

let exampleWidth = hdpx(100)
let flaresExamplesList = [
  mkLensFlareCutRadiusLeft(50, 3, 20, 65, 6, 9)
  mkLensFlareCutRadiusLeft(50, 3, 30, 75, 12, 15)
  mkLensFlareCutRadiusLeft(50, 3, 30, 85, 16, 19)

  mkLensFlareCutRadiusLeft(50, 3, 30, -120, 20, -15)
  mkLensFlareCutRadiusLeft(50, 13, 10, -180, 6, -30)
  mkLensFlareCutRadiusLeft(50, 13, 40, -60, 6, -30)
]

let examples = {
  size = [exampleWidth * flaresExamplesList.len(), exampleWidth * 2]
  flow = FLOW_HORIZONTAL
  children = flaresExamplesList.map(@(image) {
    size = flex()
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

let lensImageWidth = Computed(@() radius.value + outherWidth.value + 1)
let pxLens = @() {
  watch = [lensImageWidth, lensImage]
  size = [lensImageWidth.value, lensImageWidth.value * 2]
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = lensImage.value
}

let bigLens = @() {
  watch = lensImage
  size = flex()
  rendObj = ROBJ_IMAGE
  color = 0xFFFFFFFF
  image = lensImage.value
  keepAspect = true
  imageHalign = ALIGN_CENTER
  imageValign = ALIGN_CENTER
}

function sqBtn(text, onClick) {
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

function mkControl(watch, hasNegative) {
  let setValue = @(v) watch(hasNegative ? v : abs(v))
  return {
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    gap = hdpx(5)
    children = [
      textInput(watch, {
        margin = 0
        charMask = "-0123456789"
        setValue = @(v) v == "" ? null :  setValue(v.tointeger())
      })
      sqBtn("-", @() setValue(watch.value - 1))
      sqBtn("+", @() setValue(watch.value + 1))
    ]
  }
}

let textStyle = { hplace = ALIGN_LEFT, color = 0xFFA0A0A0 }
let options = {
  size = [hdpx(250), flex()]
  flow = FLOW_VERTICAL
  gap = hdpx(20)
  children = optionsCfg.map(function(o) {
    let { watch, text, hasNegative = false } = o
    return withText(text, mkControl(watch, hasNegative), [flex(), SIZE_TO_CONTENT], textStyle)
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
        withText("Pixel perfect", pxLens)
        withText("Scaled", bigLens, [flex(), ph(33)])
        withText("Big scaled", bigLens, flex(3))
      ]
    }
  ]
}