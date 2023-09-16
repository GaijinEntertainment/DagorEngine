from "%darg/ui_imports.nut" import *
from "math" import fabs
let { mkBitmapPicture } = require("%darg/helpers/bitmap.nut")

let gradSize = 64

let colorParts = @(color) {
  r = (color >> 16) & 0xFF
  g = (color >> 8) & 0xFF
  b = color & 0xFF
  a = (color >> 24) & 0xFF
}
let partsToColor = @(c) Color(c.r, c.g, c.b, c.a)
let lerpColorParts = @(c1, c2, value) value <= 0 ? c1
  : value >= 1 ? c2
  : c1.map(@(v1, k) v1 + ((c2[k] - v1) * value + 0.5).tointeger())

let mkGradientCtorInclined = function(color1, color2, x1, y1, x2, y2) {
  let dx = x2 - x1
  let dy = y2 - y1
  if (dx == 0) {
    logerr("mkGradientCtorInclined does not support gradient with x0 == x1. Use mkColoredGradientY instead.")
    return @(_, __) null
  }
  return function(params, bmp) {
    let { w, h } = params
    local startTop = (w - 1).tofloat() * (x1 + y1 * dy / dx)
    let startFullDiff = -(w - 1).tofloat() * dy / dx
    local width = (w - 1).tofloat() * (dx + dy * dy / dx)
    let colorStart = width < 0 ? color2 : color1
    let colorEnd = width < 0 ? color1 : color2
    let c1 = colorParts(colorStart)
    let c2 = colorParts(colorEnd)
    if (width < 0) {
      startTop = startTop + width
      width = -width
    }
    for (local y = 0; y < h; y++) {
      let start = startTop + y * startFullDiff / (h - 1)
      for (local x = 0; x < w; x++) {
        let xf = x.tofloat()
        let color = xf <= start ? colorStart
          : xf >= start + width ? colorEnd
          : partsToColor(lerpColorParts(c1, c2, (xf - start) / width))
        bmp.setPixel(x, h - 1 - y, color)
      }
    }
  }
}

return {
  gradSize
  mkInclineGradient = @(color1, color2, x1, y1, x2, y2)
    mkBitmapPicture(gradSize, gradSize, mkGradientCtorInclined(color1, color2, x1, y1, x2, y2))
}