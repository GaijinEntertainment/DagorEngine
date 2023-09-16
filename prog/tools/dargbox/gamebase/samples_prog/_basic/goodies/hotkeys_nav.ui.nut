from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")
let {msgbox} = require("msgbox.nut")

msgbox.styling.cursor = cursors.normal

let observable_counter = persist("counter", @() Watched(0)) //or model
let counter_doubled = Computed(@() observable_counter.value*2)

let hotkeysNavState = Watched([])

let function hotkeysButtonsBar() {
  let tips = []

  foreach (hotkey in hotkeysNavState.value) {
    if (hotkey.devId == DEVID_KEYBOARD || hotkey.devId == DEVID_JOYSTICK) {
      tips.append({
        size = SIZE_TO_CONTENT
        gap = sh(0.5)
        flow = FLOW_HORIZONTAL
        children = [
          {rendObj = ROBJ_TEXT text = hotkey.btnName}
          {rendObj = ROBJ_TEXT text = hotkey.description}
        ]
      })
    }
  }

  return {
    rendObj = ROBJ_SOLID
    color = Color(0,0,50,50)
    hplace = ALIGN_CENTER
    vplace = ALIGN_BOTTOM
    margin = sh(5)
    padding = sh(2)
    gap = sh(4)
    size = SIZE_TO_CONTENT
    flow = FLOW_HORIZONTAL
    watch = hotkeysNavState
    children = tips
  }
}


gui_scene.setHotkeysNavHandler(function(state) {
  //dlog(state)
  hotkeysNavState(state)
})


let function button(params) {
  let text = params?.text ?? ""
  let onClick = params?.onClick
  let onHover = params?.onHover

  return watchElemState(function(sf) {
    return{
      rendObj = ROBJ_BOX
      size = [sh(20),SIZE_TO_CONTENT]
      padding = sh(2)
      fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
      borderWidth = (sf & S_HOVER) ? 2 : 0
      behavior = [Behaviors.Button]
      halign = ALIGN_CENTER
      onClick = onClick
      onHover = onHover
      hotkeys = params?.hotkeys
      children = {rendObj = ROBJ_TEXT text = text color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }
    }
  })
}


//how to subscribe to observables without component
local prev_val = observable_counter.value
observable_counter.subscribe(function(new_val) {
  if (new_val == 0 && prev_val < 0) {
    vlog("Was negative and now zero")
  }
  prev_val = new_val
})

let function increment() {
  observable_counter.update(observable_counter.value + 1)
}
let function decrement() {
  observable_counter.update(observable_counter.value - 1)
}


let container = {
  size = flex()
  gap = sh(5)

  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    {rendObj=ROBJ_TEXT text="Click with Any Mouse Button on '+' to increment counter or on '-' to decrement"}
    {rendObj=ROBJ_TEXT text="You can also press hotkey - 'd' for Decrement counter, and 'i' for Increment"}
    button({
      text="Increment",
      onClick = increment,
      hotkeys=[
        ["^I", increment],
        ["J:X", increment, "Increment"]
      ]
    })
    @() {rendObj = ROBJ_TEXT text=$"Counter value = {observable_counter.value}" watch=observable_counter }
    @() {rendObj = ROBJ_TEXT text=$"2x counter = {counter_doubled.value}" watch=counter_doubled }
    button({
      text="Decrement",
      onClick=decrement,
      hotkeys=[
        "^D",
        ["^J:A", decrement, "Decrement"]
      ]
    }) //you can also make a callback here, it can be useful for manipulating data of object
  ]
}

let function msgboxes() {
  return {
    size = flex()
    children = msgbox.getCurMsgbox()
    watch = msgbox.msgboxGeneration
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  cursor = cursors.normal
  size = flex()
  children = [
    container
    msgboxes
    hotkeysButtonsBar
  ]

  hotkeys = [
    ["J:B | Esc ", function() {
      msgbox.show({text="Messagebox test"})
    }, "Show test messagebox"]
  ]
}
