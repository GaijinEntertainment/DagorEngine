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
    children = {rendObj = ROBJ_TEXT text = "my button0" color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) }

    sound = {
      click  = "ui/button_click"
      hover  = "ui/menu_highlight"
      active = "ui/button_action"
    }

  }
}

let button1 = watchElemState( @(sf) {
    onElemState = @(sf) stateFlags.update(sf)
    rendObj = ROBJ_BOX
    size = [sh(20),SIZE_TO_CONTENT]
    padding = sh(2)
    fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
    borderWidth = (sf & S_HOVER) ? 2 : 0
    behavior = [Behaviors.Button]
    halign = ALIGN_CENTER
    onClick = function() { vlog("clicked")}
    onHover = function(hover) {vlog($"hover: {hover}")}
    children = {rendObj = ROBJ_TEXT text = (sf & S_ACTIVE) ? "pressed": "my button1" color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }

    sound = {
      click  = "ui/button_click"
      hover  = "ui/menu_highlight"
      active = "ui/button_action"
    }

})

let buttonInside = @(parentStateFlags) @() {
  watch = parentStateFlags
  children = (parentStateFlags.value & S_HOVER)
    ? watchElemState( @(sf) {
        rendObj = ROBJ_BOX
        size = [sh(20),SIZE_TO_CONTENT]
        padding = sh(2)
        fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
        borderWidth = (sf & S_HOVER) ? 2 : 0
        behavior = [Behaviors.Button]
        halign = ALIGN_CENTER
        onClick = function() { vlog("clicked")}
        onHover = function(hover) {vlog($"hover: {hover}")}
        children = {rendObj = ROBJ_TEXT text = (sf & S_ACTIVE) ? "pressed": "inside" color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }
      })
    : null
}

let function doubleButton() {
  let sFlags = Watched(0)
  return @() {
    watch = [sFlags]
    onElemState = @(sf) sFlags.update(sf)
    rendObj = ROBJ_BOX
    size = [sh(40), sh(40)]
    padding = sh(2)
    fillColor = (sFlags.value & S_HOVER) ? Color(0,0,0) : Color(200,200,200)
    behavior = [Behaviors.Button]
    halign = ALIGN_CENTER
    children = [
      {
        rendObj = ROBJ_TEXT
        vplace = ALIGN_CENTER
        text = (sFlags.value & S_ACTIVE) ? "pressed": "outside"
        color = (sFlags.value & S_HOVER) ? Color(255,255,255): Color(0,0,0)
        pos = (sFlags.value & S_ACTIVE) ? [0,2] : [0,0]
      }
      buttonInside(sFlags)
    ]
  }
}

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
    doubleButton()
  ]
}
