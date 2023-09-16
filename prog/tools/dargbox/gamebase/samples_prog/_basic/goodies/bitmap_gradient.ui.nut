from "%darg/ui_imports.nut" import *
from "%darg/helpers/bitmap.nut" import mkBitmapPicture
from "math" import abs, fabs, sqrt

import "samples_prog/_cursors.nut" as cursors


let function gradient(params, bmp) {
  let {w, h} = params
  let { setPixel } = bmp
  for (local y=0; y<h; ++y) {
    for (local x=0; x<w; ++x) {
      setPixel(x, y, Color(y*200/h+1, x*200/w+1, 5, 255))
    }
  }
}

let function gradientRounded(params, bmp) {
  let {w, h} = params
  let rounding = 10
  let { setPixel } = bmp
  let cx = w/2, cy = h/2 // center, also half-size
  for (local y=0; y<h; ++y) {
    for (local x=0; x<w; ++x) {
      let r = y*200/h+1
      let g = x*200/w+1
      let b = 5
      let a = 255
      let vx = max(0, abs(x-cx)-cx+rounding)
      let vy = max(0, abs(y-cy)-cy+rounding)
      let raduis = sqrt(vx*vx+vy*vy)
      let k = clamp(rounding-raduis, 0.0, 1.0)
      setPixel(x, y, Color(r*k, g*k, b*k, a*k))
    }
  }
}

let function mkImage(func) {
  let pic = mkBitmapPicture(80, 80, func)
  return {
    rendObj = ROBJ_IMAGE
    size = [sh(20), sh(20)]
    image = pic
  }
}


return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = 0xFF202020
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(5)
  children = [
    mkImage(gradient)
    mkImage(gradientRounded)
  ]
  cursor = cursors.normal
}
