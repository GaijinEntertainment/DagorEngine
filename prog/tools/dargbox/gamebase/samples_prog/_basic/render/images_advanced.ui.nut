from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let picture_from_atlas = Picture("!ui/atlas#button") //! in begining means non premultiplied alpha
let picture_from_tga = Picture("ui/a-20g.tga")
let picture_from_jpg = Picture("ui/loading.jpg")
  //let picture_from_ddsx = Picture("ui/login_layer_3_l.ddsx") // disabled because of incompatibilities between platforms
let picture_from_tga_premul = Picture("!ui/non_premul.tga")
let picture_from_tga_non_premul = Picture("ui/non_premul.tga")
let picture_from_png = Picture("ui/ca_cup1.png")


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = 20

  children = [
    {
      size = sh(10)
      keepAspect = true
      rendObj = ROBJ_IMAGE
      image = picture_from_atlas
    }
    {
      size = sh(10)
      rendObj = ROBJ_IMAGE
      image = picture_from_atlas
      picSaturate = 0
      color = Color(30,40,150)
    }
    {
      flow = FLOW_HORIZONTAL
      children = [
        {
          size = static [sh(40), sh(8)] //means it will keep aspect ratio
          rendObj = ROBJ_IMAGE
          keepAspect = true
          image = picture_from_tga
          picSaturate = 3.1
          children = {rendObj=ROBJ_FRAME size=flex()}
        }
        {
          size = static [sh(40), sh(8)] //means it will keep aspect ratio
          rendObj = ROBJ_IMAGE
          keepAspect=true
          image = picture_from_tga
          picSaturate = 0.1
          children = {rendObj=ROBJ_FRAME size=flex()}
        }
      ]
    }
    {
      size = static [sh(40), sh(20)]
      keepAspect = true
      rendObj = ROBJ_IMAGE
      image = picture_from_jpg
    }
    {
      size = sh(10)
      rendObj = ROBJ_IMAGE
      image = picture_from_png
    }
/*
    {
      size = static [sh(20), SIZE_TO_CONTENT] //means it will keep aspect ratio
      rendObj = ROBJ_IMAGE
      image = picture_from_ddsx
    }
*/
    {
      flow = FLOW_HORIZONTAL
      size = static [sh(10),sh(5)]
      children = [
        {
          size = sh(5) //means it will keep aspect ratio
          rendObj = ROBJ_IMAGE
          image = picture_from_tga_premul
          keepAspect = true
        }
        {
          size = sh(5) //means it will keep aspect ratio
          rendObj = ROBJ_IMAGE
          keepAspect = true
          image = picture_from_tga_non_premul
        }
      ]
    }
  ]
}
