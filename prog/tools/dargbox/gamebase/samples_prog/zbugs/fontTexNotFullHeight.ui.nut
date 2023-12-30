from "%darg/ui_imports.nut" import *
let { mkBitmapPicture } = require("%darg/helpers/bitmap.nut")

let gradient_ht = 11
let middle = 6
let spread = 3
let colorTop = 0xFFFF0000
let colorGradTop = 0xFFFFFFFF
let colorGradBottom = 0xFF0000FF
let colorBottom = 0xFF00FF00

let bovVariants = [@(_) 0, @(s) 0.525 * s]

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

let gradient = mkBitmapPicture(2, gradient_ht,
  function(params, bmp) {
    let { w, h } = params
    let c1 = colorParts(colorGradBottom)
    let c2 = colorParts(colorGradTop)
    let y1 = middle - spread - 1
    let y2 = middle + spread - 1
    for(local y = 0; y < h; y++) {
      let color = y == 0 ? colorBottom
        : y == h-1 ? colorTop
        : y <= y1 ? colorGradBottom
        : y >= y2 ? colorGradTop
        : partsToColor(lerpColorParts(c1, c2, (y - y1).tofloat() / (spread * 2)))
      for(local x = 0; x < w; x++)
        bmp.setPixel(x, y, color)
    }
  })

function full_font_line_ht(fontSize) {
  let mx = get_font_metrics(0, fontSize)
  return mx.ascent+mx.descent
}
let mkGradRow = @(fontSize) {
  size = [SIZE_TO_CONTENT, 0.71 * fontSize]
  flow = FLOW_HORIZONTAL
  valign = ALIGN_CENTER
  gap = 0.1* fontSize
  children = bovVariants.map(@(_calcBov) {
    size = [SIZE_TO_CONTENT, flex()]
    valign = ALIGN_CENTER
    children = [
      {
        size = [0.5 * fontSize, full_font_line_ht(fontSize)]
        rendObj = ROBJ_IMAGE
        image = gradient
      }
      {
        rendObj = ROBJ_TEXT
        text = "WpBAЁу"
        fontSize = fontSize
        fontTex = gradient
        //fontTexSv = 32*full_font_line_ht(fontSize)/gradient_ht
        fontTexSv = 0
      }
    ]
  })
}

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = 0xFF000000
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = hdpx(70)
  children = [hdpx(110), hdpx(100), hdpx(50), hdpx(30), hdpx(25)]
    .map(mkGradRow)
}