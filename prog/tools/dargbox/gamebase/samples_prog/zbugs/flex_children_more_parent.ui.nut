from "%darg/ui_imports.nut" import *

/*
  Let's say a parent has a specific size,
  and his children of the first level have a flex() size,
  with max Width, then children of level 3 with a flex() size will also have to prescribe maxWidth,
  otherwise they will be located according to the parent with specific sizes,
  and not according to his parent.
  Is this behavior correct?
*/


let childrenFlex = @(color) {
  rendObj = ROBJ_BOX
  size = static [flex(), sh(10)]
  borderWidth = hdpx(1)
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  valign = ALIGN_CENTER
  maxWidth = hdpx(135)
  margin = hdpx(40)
  children = {
    size = static [flex(), hdpx(60)]
    rendObj = ROBJ_BOX
    borderWidth = hdpx(3)
    borderColor = color
  }
}

return {
  size = static [sw(50), fsh(6)]
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  flow = FLOW_HORIZONTAL
  children = [
    childrenFlex(Color(66, 170, 255))
    childrenFlex(Color(255, 36, 36))
    childrenFlex(Color(63, 255, 5))
    childrenFlex(Color(255, 229, 20))
  ]
}