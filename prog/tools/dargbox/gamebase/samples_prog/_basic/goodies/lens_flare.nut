from "%darg/ui_imports.nut" import *
from "math" import min, sqrt, max
let { mkBitmapPicture } = require("%darg/helpers/bitmap.nut")

let mkWhite = @(part) part + (part << 8) + (part << 16) + (part << 24)
let getDistance = @(x, y) sqrt(x*x + y*y)

//cutRadius > 0 - cutFrom inner side.  cutRadius < 0 - cutFrom outher side
function mkLensFlareCutRadiusLeft(radius, outherWidth, innerWidth, cutRadius, cutWidth, cutOffset) {
  let center = radius + outherWidth + 1
  let cutCenter = cutRadius > 0 ? cutRadius + cutWidth : center + cutRadius
  let calcCutMul = cutRadius > 0
    ? function(x, y) {
        let distanceCut = getDistance(0.5 + x - cutCenter - cutOffset, 0.5 + y - center) - cutRadius
        if (distanceCut <= 0)
          return 0
        return cutWidth == 0 ? 1.0 : min(1.0, distanceCut / cutWidth)
      }
    : function(x, y) {
        let distanceCut = getDistance(0.5 + x - cutCenter - cutOffset, 0.5 + y - center) + cutRadius
        if (distanceCut >= 0)
          return 0
        return cutWidth == 0 ? 1.0 : min(1.0, -distanceCut / cutWidth)
      }

  return mkBitmapPicture(center, center * 2,
    function(params, bmp) {
      let { w, h } = params
      for (local y = 0; y < h; y++)
        for (local x = 0; x < w; x++) {
          let cutMul = calcCutMul(x, y)
          if (cutMul == 0) {
            bmp.setPixel(x, y, 0)
            continue
          }
          let distance = getDistance(0.5 + x - center, 0.5 + y - center) - radius
          let mul = distance >= 0 ? distance / (outherWidth != 0 ? outherWidth : 1) : - distance / (innerWidth != 0 ? innerWidth : 1)
          bmp.setPixel(x, y, mkWhite((0xFF * max(0.0, (1.0 - mul) * cutMul)).tointeger()))
        }
    })
}

return {
  mkLensFlareCutRadiusLeft
}