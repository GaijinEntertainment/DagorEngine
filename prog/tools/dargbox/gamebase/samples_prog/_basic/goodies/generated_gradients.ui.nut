from "%darg/ui_imports.nut" import *

let {mkLinearGradientImg
  ,mkRadialGradientImg
//  ,mkLinearGradSvgTxt
  ,mkRadialGradSvgTxt
} = require("%darg/helpers/mkGradientImg.nut")

let red = [255,0,0]
let blue = [0, 0, 255]

let mkImage = @(img) {rendObj=ROBJ_IMAGE image=img size = [hdpx(300), hdpx(300)]}
let linGradParams1 = {
  points = [red, blue, {color = [0, 255, 0] opacity = 0.2}]
  width = 64
  height = 4
}

//print(mkLinearGradSvgTxt(linGradParams1))
let radGradParams1=freeze({
  points = [
    {offset = 40 color = [255, 255, 255]},
    {color = [0, 250, 0] opacity = 0.1}
  ]
  width = 256
  height = 256
//  r = 256/2.0
//  fx = 0.6
//  fy = 0.7
})
print(mkRadialGradSvgTxt(radGradParams1))

return {
  flow = FLOW_HORIZONTAL
  gap = sh(1)
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  children = [
    mkLinearGradientImg(linGradParams1)
    mkLinearGradientImg({
      points = [red, [0, 0, 255, 190], [0, 0, 0, 0]]
      width = 4
      height = 64
      x2 = 0
      y2 = 64
    })
    mkRadialGradientImg(radGradParams1)
  ].map(mkImage)
}
