from "%darg/ui_imports.nut" import *

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(120,120,120)
  children = {
    image = Picture("+ui/glow.png")
    rendObj = ROBJ_IMAGE
    size = [hdpx(100), hdpx(100)]
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
  }
}