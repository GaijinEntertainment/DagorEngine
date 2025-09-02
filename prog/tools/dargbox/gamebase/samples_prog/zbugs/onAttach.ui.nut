from "%darg/ui_imports.nut" import *

/*
  Let's say a parent has a specific size,
  and his children of the first level have a flex() size,
  with max Width, then children of level 3 with a flex() size will also have to prescribe maxWidth,
  otherwise they will be located according to the parent with specific sizes,
  and not according to his parent.
  Is this behavior correct?
*/

let size = Watched("")
let elem = @() {
  onAttach = function(el){
    size.set($"x:{el.getScreenPosX()}, y:{el.getScreenPosY()}")
  }
  onDetach = @() size.set(null)
  size = 50
  rendObj = ROBJ_SOLID
  color = Color(100,100,100)
  children = @() {watch = size, rendObj = ROBJ_TEXT text=size.get()}
}

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10,10,100)
  flow = FLOW_HORIZONTAL
  children = [
    {size = static [sh(20), 0]}
    {
      flow = FLOW_VERTICAL
      children = array(10, {size = static [hdpx(100), hdpx(10)]}).append(elem)
    }
  ]
}