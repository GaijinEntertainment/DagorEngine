from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")
let combobox = require("combobox.nut")

let comboDisable = Watched(false)

let items = array(20).map(@(_,k) [k+1, $"Item {k}"])
let comboValue = Watched(items[0][1])
// Bind only to current selection
//local combo = combobox(comboValue, items)

// Bind to selection and disable state
let combo = combobox({value=comboValue, disable=comboDisable}, items)


let toggleButton = @() {
  size = flex()
  rendObj = ROBJ_BOX
  fillColor = Color(200, 180, 100)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  behavior = Behaviors.Button
  children = {
    rendObj = ROBJ_TEXT
    text = comboDisable.value ? "Enable" : "Disable"
  }
  onClick = @() comboDisable.update(!comboDisable.value)
  watch = comboDisable
}


return {
  rendObj = ROBJ_SOLID
  color = Color(50,60,80)
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  cursor = cursors.normal
  children = {
    valign  = ALIGN_CENTER
    halign  = ALIGN_CENTER
    flow    = FLOW_VERTICAL
    gap  = sh(2)
    size = [sh(40), sh(20)]

    children = [
      toggleButton
      {
        size = flex()
        rendObj = ROBJ_SOLID
        color = Color(0,0,0)
        children = combo
      }
    ]
  }
}
