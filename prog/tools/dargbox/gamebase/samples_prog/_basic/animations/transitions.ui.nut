from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let stateFlags = Watched(0)

let function button0() {
  let sf = stateFlags.value
  return {
    watch = [stateFlags]
    onElemState = @(sf) stateFlags.update(sf)
    rendObj = ROBJ_BOX
    size = [sh(20),SIZE_TO_CONTENT]
    padding = sh(2)
    fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
    borderWidth = (sf & S_HOVER) ? 2 : 0
    behavior = [Behaviors.Button]
    halign = ALIGN_CENTER
    children = {
      rendObj = ROBJ_TEXT
      text = "my button0"
      color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0)
      transitions = [
        { prop=AnimProp.color, duration = (sf & S_ACTIVE) ? 0.8 : 0.3 }
      ]
    }
    transitions = [
      { prop=AnimProp.fillColor, duration = (sf & S_ACTIVE) ? 0.8 : 0.3,
        onEnter=@() vlog("onEnter")
        onExit=@() vlog("onExit")
        onFinish=@() vlog("onFinish")
        onAbort=@() vlog("onAbort")
      }
    ]
  }
}

let button1 = watchElemState( @(sf) {
    rendObj = ROBJ_BOX
    size = [sh(20),SIZE_TO_CONTENT]
    padding = sh(2)
    fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
    borderWidth = (sf & S_HOVER) ? 2 : 0
    opacity = (sf & S_HOVER) ? 1.0 : 0.3
    behavior = [Behaviors.Button]
    halign = ALIGN_CENTER
    onClick = function() { vlog("clicked")}
    children = {
        rendObj = ROBJ_TEXT text = (sf & S_ACTIVE) ? "pressed": "my button1" color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0]
        transitions = [{ prop=AnimProp.color, duration = 1.0, easing=OutCubic }]
      }

    transform = {
      scale = (sf & S_HOVER) ? [1.2,1.2] : [1,1]
    }
    transitions = [
      { prop=AnimProp.opacity, duration = 0.5, easing=OutCubic }
      { prop=AnimProp.fillColor, duration = 0.25, easing=OutCubic }
      { prop=AnimProp.scale, duration = 0.25, easing=InOutCubic }
    ]
})

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    button0
    button1
  ]
}
