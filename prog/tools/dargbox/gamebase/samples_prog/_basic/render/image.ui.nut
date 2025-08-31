from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let picture = Picture("!ui/atlas#ca_cup1")

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex(10)
  minWidth = SIZE_TO_CONTENT
  minHeight = SIZE_TO_CONTENT
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  children = [
    {
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      size = sh(20)
      rendObj = ROBJ_IMAGE
      image=picture
    }
  ]
}
