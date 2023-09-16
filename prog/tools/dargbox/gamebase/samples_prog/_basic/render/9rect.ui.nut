from "%darg/ui_imports.nut" import *
let { mkBitmapPicture } = require("%darg/helpers/bitmap.nut")

let gradCircCornerSize = 20
let gradCircFullSize = 2 * gradCircCornerSize + 6
let objHeight = hdpx(100)
let objWidth = hdpx(300)
let objCorner = hdpx(40)

let cornerColor = 0xFFFF0000
let middleColor = 0xFF202020
let sideColor = 0xFFFFFF000
let borderInCorner = 0xFF00FF00
let borderOutCorner = 0xFF0000FF
let borderCross = 0xFFFFFFFF

let isSide = @(v) v < gradCircCornerSize || v >= gradCircFullSize - gradCircCornerSize
let isBorderIn = @(v) v == gradCircCornerSize - 1 || v == gradCircFullSize - gradCircCornerSize
let isBorderOut = @(v) v == gradCircCornerSize || v == gradCircFullSize - gradCircCornerSize - 1
let function chooseColor(func, x, y, colorDouble, colorSingle, colorNothing = null) {
  let isX = func(x)
  let isY = func(y)
  return isX && isY ? colorDouble
    : isX || isY ? colorSingle
    : colorNothing
}
let tex = mkBitmapPicture(gradCircFullSize, gradCircFullSize,
  function(_, bmp) {
    for (local x = 0; x < gradCircFullSize; x++)
      for (local y = 0; y < gradCircFullSize; y++)
        bmp.setPixel(x, y,
          chooseColor(isBorderIn, x, y, borderCross, borderInCorner)
          ?? chooseColor(isBorderOut, x, y, borderCross, borderOutCorner)
          ?? chooseColor(isSide, x, y, cornerColor, sideColor, middleColor))
  })

let mkGrad = @(header, ovr) {
  size = [flex(), SIZE_TO_CONTENT]
  flow = FLOW_HORIZONTAL
  gap = hdpx(20)
  valign = ALIGN_CENTER
  children = [
    {
      size = [flex(1.5), SIZE_TO_CONTENT]
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      halign = ALIGN_RIGHT
      text = header
    }
    {
      size = [flex(), SIZE_TO_CONTENT]
      children = {
        size = [objWidth, objHeight]
        rendObj = ROBJ_9RECT
        image = tex
        screenOffs = objCorner
        texOffs = gradCircCornerSize + 1
      }.__update(ovr)
    }
  ]
}

return {
  size = flex()
  flow = FLOW_VERTICAL
  gap = hdpx(20)
  children = [
    mkGrad("ROBJ_IMAGE pixelPerfect x4 size", { rendObj = ROBJ_IMAGE, size = array(2, gradCircFullSize * 4) })
    mkGrad("ROBJ_IMAGE", { rendObj = ROBJ_IMAGE })
    mkGrad("ROBJ_9rect screenOffs = objCorner", {})
    mkGrad("ROBJ_9rect screenOffs = [objCorner, 2 * objCorner]", { screenOffs = [objCorner, 2 * objCorner] })
    mkGrad("ROBJ_9rect screenOffs = 0.5 * objCorner", { screenOffs = 0.5 * objCorner })
    mkGrad("ROBJ_9rect screenOffs = 3 * objCorner", { screenOffs = 3 * objCorner })
    mkGrad("ROBJ_9rect screenOffs = 20 * objCorner", { screenOffs = 20 * objCorner })
    mkGrad("ROBJ_9rect \nscreenOffs = [objCorner, 2 * objCorner, 0.5 * objCorner, 3 * objCorner]",
      { screenOffs = [objCorner, 2 * objCorner, 0.5 * objCorner, 3 * objCorner] })
  ]
}