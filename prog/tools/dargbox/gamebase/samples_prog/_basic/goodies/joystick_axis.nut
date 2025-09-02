from "%darg/ui_imports.nut" import *

let axis = gui_scene.getJoystickAxis(AXIS_R_THUMB_V)
axis.deadzone = 0.1
axis.resolution = 0.05

return function() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30, 40, 50)
    size = flex()
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    flow = FLOW_VERTICAL

    children = [
      {
        rendObj = ROBJ_TEXT
        text = "Move right stick vertically"
      }
      @(){
        watch = axis
        rendObj = ROBJ_TEXT
        text = axis.get()
      }
    ]
  }
}
