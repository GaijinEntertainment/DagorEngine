from "%darg/ui_imports.nut" import *

let { mkLinearGradientImg } = require("%darg/helpers/mkGradientImg.nut")
let cursors = require("samples_prog/_cursors.nut")


let item = {
  size = [sh(20), sh(20)]
  rendObj = ROBJ_SOLID
  color = Color(255,255,255)

  transform = {
    pivot = [0.5, 0.5]
  }

  animations = [
    { prop=AnimProp.color, from=Color(255,255,0), to=Color(0,0,255), duration=5, play=true, loop=true, easing=Linear, trigger="initial" }
  ]
}

let yellow = [255, 255, 0]
let blue = [0, 0, 255]

let mkImage = @(img) {rendObj=ROBJ_IMAGE image=img size = [hdpx(300), hdpx(25)]}
let gradientParams = {
  points = [yellow, blue]
  width = 64
  height = 4
}

let gradient = mkImage(mkLinearGradientImg(gradientParams))


return {
  rendObj = ROBJ_SOLID
  flow = FLOW_VERTICAL
  gap = sh(2)
  size = flex()
  color = Color(30,40,50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  children = [
    item
    gradient
  ]
}
