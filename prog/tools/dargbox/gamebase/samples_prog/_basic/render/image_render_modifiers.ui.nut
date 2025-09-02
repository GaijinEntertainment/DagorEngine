from "string" import format
from "%darg/ui_imports.nut" import *
//BUG: picture with ! mark has incorrect color when use opacity

let size = hdpxi(100)
let gap = hdpx(10)
let ovrList = [{ color = 0xFFFFFFFF },
  { color = 0xCCCCCCCC }, { color = 0xFFFFFFFF, opacity = 0.8 },
  { color = 0x80808080 }, { color = 0xFFFFFFFF, opacity = 0.5 },
  { color = 0x40808080 }, { color = 0x80404040 }, { color = 0x00808080 }, { color = 0xFF808080 }
]
  .map(@(v) v.__update({ size = [size, size] }))
let bgColors = [0xFF000000, 0xFF800000, 0xFF008000, 0xFF000080, 0xFF404040]
let bgColorIdx = Watched(0)

let mkRow = @(headerTexts, ctor) {
  flow = FLOW_HORIZONTAL
  gap
  children = [{
    vplace = ALIGN_CENTER
    flow = FLOW_VERTICAL
    padding = static [0, hdpx(20), 0, 0]
    children = headerTexts.map(@(text) { rendObj = ROBJ_TEXT, text, color = 0xFFFFFFFF , fontScale = 0.8 })
  }]
    .extend(ovrList.map(ctor))
}

let mkHeader = @(ovr) {
  flow = FLOW_VERTICAL
  children = [
    "color ="
    format("%X", ovr?.color ?? 0xFFFFFFFF)
    "opacity ="
    format("%.2f", ovr?.opacity ?? 1)
  ].map(@(text, idx) { rendObj = ROBJ_TEXT, text, hplace = (idx % 2) ? ALIGN_RIGHT : ALIGN_LEFT, color = ovr?.color ?? 0xFFFFFFFF , fontScale = 0.8 } )
}.__update(ovr)

let mkSolid = @(ovr) {
  rendObj = ROBJ_SOLID
}.__update(ovr)

let mkSvg = @(ovr) {
  rendObj = ROBJ_IMAGE
  keepAspect = KEEP_ASPECT_FIT
  image = Picture($"ui/hud_tank_arrow_right_02.svg:{size}:{size}")
}.__update(ovr)

let mkSvgPremultiplied = @(ovr) {
  rendObj = ROBJ_IMAGE
  keepAspect = KEEP_ASPECT_FIT
  image = Picture($"ui/hud_tank_arrow_right_02.svg:{size}:{size}:P")
}.__update(ovr)

let mkSvgAlphaBlend = @(ovr) {
  rendObj = ROBJ_IMAGE
  keepAspect = KEEP_ASPECT_FIT
  image = Picture($"!ui/hud_tank_arrow_right_02.svg:{size}:{size}")
}.__update(ovr)

let mkPng = @(ovr) {
  rendObj = ROBJ_IMAGE
  keepAspect = KEEP_ASPECT_FIT
  image = Picture($"ui/ca_cup1.png:{size}:{size}")
}.__update(ovr)

let mkPngPremultiplied = @(ovr) {
  rendObj = ROBJ_IMAGE
  keepAspect = KEEP_ASPECT_FIT
  image = Picture($"ui/ca_cup1.png:{size}:{size}:P")
}.__update(ovr)

let mkPngAlphaBlend = @(ovr) {
  rendObj = ROBJ_IMAGE
  keepAspect = KEEP_ASPECT_FIT
  image = Picture($"!ui/ca_cup1.png:{size}:{size}")
}.__update(ovr)

return @() {
  watch = bgColorIdx
  rendObj = ROBJ_SOLID
  color = bgColors[bgColorIdx.get() % bgColors.len()]
  behavior = Behaviors.Button
  onClick = @() bgColorIdx.set(bgColorIdx.get() + 1)

  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  padding = gap
  gap
  children = [
    mkRow(["ROBJ_TEXT"], mkHeader)
    mkRow(["ROBJ_SOLID"], mkSolid)
    mkRow(["ROBJ_IMAGE", "svg", "simple"], mkSvg)
    mkRow(["ROBJ_IMAGE", "svg", "postfix :P", "premultiplied"], mkSvgPremultiplied)
    mkRow(["ROBJ_IMAGE", "svg", "prefix !", "alphablend"], mkSvgAlphaBlend)
    mkRow(["ROBJ_IMAGE", "png", "simple"], mkPng)
    mkRow(["ROBJ_IMAGE", "png", "postfix :P", "premultiplied"], mkPngPremultiplied)
    mkRow(["ROBJ_IMAGE", "png", "prefix !", "alphablend"], mkPngAlphaBlend)
  ]
}