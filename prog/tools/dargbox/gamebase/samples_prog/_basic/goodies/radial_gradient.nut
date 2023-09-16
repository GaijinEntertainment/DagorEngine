from "%darg/ui_imports.nut" import *
let { sqrt } = require("math")
let { mkBitmapPicture } = require("%darg/helpers/bitmap.nut")

let gradSize = 64

let getDistance = @(x, y) sqrt(x * x + y * y)

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

let mkGradientCtorRadial = @(color1, color2, r1, gradWidth, cx, cy) function(params, bmp) {
  let { w, h } = params
  let r2 = r1 + gradWidth
  let c1 = colorParts(color1)
  let c2 = colorParts(color2)
  for (local y = 0; y < h; y++)
    for (local x = 0; x < w; x++) {
      let r = getDistance(x - cx, y - cy)
      let color = r <= r1 ? color1
        : r >= r2 ? color2
        : partsToColor(lerpColorParts(c1, c2, (r - r1) / gradWidth))
      bmp.setPixel(x, h - 1 - y, color)
    }
}

return {
  gradSize
  mkRadialGradient = @(color1, color2, radius, gradWidth, cx, cy)
    mkBitmapPicture(gradSize, gradSize, mkGradientCtorRadial(color1, color2, radius, gradWidth, cx, cy))
}