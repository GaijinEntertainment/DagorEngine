from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let pic = Picture("ui/chat.svg")
let pic_custom_size = Picture("ui/chat.svg:400:400.5")
let pic_custom_size_k = Picture("ui/chat.svg:200:200:K")
//local pic_custom_size_f = Picture("ui/chat.svg:200:200:F")

let image1 = {
  size = [sh(25), sh(25)] //means it will keep aspect ratio
  keepAspect = true
  rendObj = ROBJ_IMAGE
  image=pic
}

let image2 = {
  size = [sh(25), sh(25)] //means it will keep aspect ratio
  keepAspect = true
  rendObj = ROBJ_IMAGE
  image=pic_custom_size
}
let image3 = {}.__update(image2).__update({image=pic_custom_size_k})

return {
  rendObj = ROBJ_SOLID
  size = flex()
  color = Color(30,40,50)
  cursor = cursors.normal
  flow = FLOW_HORIZONTAL
  gap = sh(5)
  children = [
    image1
    image2
    image3
    image2.__merge({color = Color(255, 0, 0)})
  ]
}
