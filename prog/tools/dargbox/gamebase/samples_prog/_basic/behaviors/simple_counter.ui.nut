from "%darg/ui_imports.nut" import *

function button(text, onClick, hotkey=null) {
  let stateFlags = Watched(0)
  let hotkeys = hotkey==null ? null : [[hotkey, onClick]]
  let onElemState = @(s) stateFlags.set(s)
  return function() {
    let sf = stateFlags.get()
    return {
      rendObj = ROBJ_BOX
      padding = sh(2)
      watch = stateFlags
      onElemState
      fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
      borderWidth = (sf & S_HOVER) ? 2 : 0
      behavior = Behaviors.Button
      hotkeys
      halign = ALIGN_CENTER
      onClick
      children = {rendObj = ROBJ_TEXT text = text color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }
    }
  }
}

let counter = Watched(0)
let increment = @() counter.modify(@(v) v+1)
let decrement = @() counter.modify(@(v) v-1)

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    button("Increment", increment, "^I")
    @() {rendObj = ROBJ_TEXT text=$"Counter value = {counter.value}" watch=counter }
    button("Decrement", decrement, "^D")
  ]
}
