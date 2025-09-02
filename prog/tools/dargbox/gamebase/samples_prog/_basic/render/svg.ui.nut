from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let picture = Picture("ui/tiger.svg")

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  cursor = cursors.normal
  size = flex()

  children = [
    {
      size = sh(50)
      pos = [sh(10), sh(10)]
      rendObj = ROBJ_IMAGE
      image=picture
    }
  ]
}
