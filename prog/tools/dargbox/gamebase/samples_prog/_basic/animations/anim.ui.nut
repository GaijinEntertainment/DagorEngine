from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")


let item = {
  size = sh(20)
  rendObj = ROBJ_SOLID
  color = Color(220, 180, 120)

  behavior = Behaviors.Button

  function onClick() {
    anim_skip("initial")
  }

  transform = {
    pivot = [0.5, 0.5]
  }

  animations = [
    { prop=AnimProp.scale, from=[0,0], to=[1,1], duration=3, play=true, delay = 0, easing=OutCubic, trigger="initial" }
    { prop=AnimProp.rotate, from=-100, to=0, duration=3, play=true, easing=OutCubic, trigger="initial", onExit="keyframe2" }

    { prop=AnimProp.scale, from=[1,1], to=[1.5,1.5], duration=1, easing=InOutCubic, trigger="keyframe2", onExit="keyframe3", onEnter=@() vlog("Keyframe") }
    { prop=AnimProp.scale, from=[1.5,1.5], to=[1,1], duration=1, easing=InOutCubic, trigger="keyframe3", onFinish=@() vlog("Finished"), onExit="keyframe4", onAbort=@() vlog("Aborted") }
    { prop=AnimProp.scale, from=[1,1], to=[1.2,1.2], duration=3, easing=CosineFull, trigger="keyframe4", loop=true, onAbort=@() vlog("Aborted")}
    //avaialable keys for animations: loop=true/false, duration, prop, from, to, trigger, easing, onExit, onFinish, onAbort, onEnter
  ]
}


return {
  rendObj = ROBJ_SOLID
  size = flex()
  color = Color(30,40,50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  children = [
    item
  ]
}
