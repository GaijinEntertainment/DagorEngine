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
    size = static [sh(60), sh(40)]
    rendObj = ROBJ_MOVIE
    movie = "ui/gaijin.ogg"
    behavior = Behaviors.Movie
  }

}
