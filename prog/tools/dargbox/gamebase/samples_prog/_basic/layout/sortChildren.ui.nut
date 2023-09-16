from "%darg/ui_imports.nut" import *

let function box(offs, color, sortOrder) {
  return {
    rendObj = ROBJ_SOLID
    color = color
    size = [sh(50), sh(50)]
    pos = [sh(10)*offs, sh(10)*offs]
    sortOrder = sortOrder
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()

  sortChildren = true

  children = [
    box(1, Color(200,0,0,180), 3)
    box(2, Color(0,200,0,180), 6)
    box(3, Color(0,0,200,180), 1)
  ]
}
