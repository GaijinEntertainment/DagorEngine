from "%darg/ui_imports.nut" import *
let { format } = require("string")

let bgColor = Watched(null)
let fgColor = Watched(null)

let calcPart = @(bg, fg, a, getPart) clamp((getPart(fg) + (1.0 - a) * getPart(bg) + 0.5).tointeger(), 0, 255)
let resColor = Computed(function() {
  let bg = bgColor.value
  let fg = fgColor.value
  if (bg == null || fg == null)
    return null
  let a = ((fg >> 24) & 0xff).tofloat() / 255
  return Color(calcPart(bg, fg, a, @(c) (c >> 16) & 0xff),
               calcPart(bg, fg, a, @(c) (c >>  8) & 0xff),
               calcPart(bg, fg, a, @(c) c & 0xff),
               0xff)
    & 0xffffffff
})

let function mkColorBox(color, hoverWatch, override = {}) {
  let stateFlags = Watched(0)
  return @() {
    watch = stateFlags
    size = flex()
    behavior = Behaviors.Button
    function onElemState(sf) {
      stateFlags(sf)
      hoverWatch(sf & S_HOVER ? color : null)
    }
    rendObj = ROBJ_SOLID
    color
    children = stateFlags.value & S_HOVER
      ? {
          size = flex()
          rendObj = ROBJ_BOX
          borderWidth = hdpx(2)
          borderColor = 0xFFFFFFFF
        }
      : null
  }.__update(override)
}

let mkColorTest = @(color) {
  size = [hdpx(600), hdpx(120)]
  children = [
    {
      size = flex()
      flow = FLOW_HORIZONTAL
      children = [0, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFF800000, 0xFF008000, 0xFF000080]
        .map(@(c) mkColorBox(c, bgColor))
    }
    mkColorBox(color, fgColor, { margin = [hdpx(60), hdpx(30), 0, 0] })
  ]
}

let colorBoxes = {
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = hdpx(10)
  children = [0xFFFF0000, 0x60600000, 0x60FF0000, 0xFF008000, 0x60003000, 0x60008000].map(mkColorTest)
}

let colorToText = @(c) ", ".concat(format("0x%08X", c),
  format("Color(%4d, %4d, %4d, %4d)", (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, (c >> 24) & 0xff))

let colorInfo = @(colorW, text) @() {
  watch = colorW
  flow = FLOW_HORIZONTAL
  children = colorW.value == null ? null
    : [
        { rendObj = ROBJ_TEXT, text = $"{text}: ", size = [hdpx(150), SIZE_TO_CONTENT] }
        { rendObj = ROBJ_TEXT, text = colorToText(colorW.value) }
      ]
}

let boxesInfo = {
  flow = FLOW_VERTICAL
  children = [
    colorInfo(bgColor, "background")
    colorInfo(fgColor, "foreground")
    colorInfo(resColor, "result")
  ]
}

return {
  size = flex()
  children = [
    colorBoxes
    boxesInfo
  ]
}