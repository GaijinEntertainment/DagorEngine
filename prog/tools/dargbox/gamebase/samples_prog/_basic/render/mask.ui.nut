from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  children = {
     size = sh(50)
     rendObj = ROBJ_MASK
     //rendObj = ROBJ_IMAGE
     //image = Picture("!ui/non_premul.tga")
     image = Picture("ui/white_circle.png")
     //image = Picture($"ui/tiger.svg:{sh(50)}:{sh(50)}")
     halign = ALIGN_CENTER
     valign = ALIGN_CENTER

     children = {
       size = static [SIZE_TO_CONTENT, sh(50)]
       rendObj = ROBJ_IMAGE
       image = Picture("ui/loading.jpg")
     }
  }
}
