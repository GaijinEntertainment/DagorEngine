from "%darg/ui_imports.nut" import *

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(2)
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    {
      size = [sh(5), sh(5)]
      rendObj = ROBJ_IMAGE
      image= Picture("ui/image.png")
    }
    {
      rendObj = ROBJ_TEXT
      text = "Hello World!"
    }
  ]
}
