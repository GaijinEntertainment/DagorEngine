from "%darg/ui_imports.nut" import *

return {
  size = [sh(50), sh(50)]
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {
    rendObj = ROBJ_FRAME
    size = flex()
    maxWidth = sh(30)
    clipChildren = true
    children = {
      size = flex()
      rendObj = ROBJ_IMAGE
      keepAspect = KEEP_ASPECT_FILL
      image = Picture("ui/ca_cup1")
    }
  }
}