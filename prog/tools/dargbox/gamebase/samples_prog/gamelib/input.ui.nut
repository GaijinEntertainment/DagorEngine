from "%darg/ui_imports.nut" import *

import "samples_prog/_cursors.nut" as cursors
import "gamelib.input" as input
import "eventbus"


eventbus.subscribe("kb-input", function on_key(event) {
  dlog("Keyboard", event)
})

eventbus.subscribe("mouse-button", function on_mouse_button(event) {
  dlog("Mouse button", event)
})

eventbus.subscribe("mouse-wheel", function on_mouse_wheel(event) {
  dlog("Mouse wheel", event)
})

eventbus.subscribe("mouse-move", function on_mouse_move(event) {
  dlog("Mouse move", event)
  let pos = input.get_mouse_pos()
  dlog($"  > pos = {pos.x}, {pos.y}")
})

eventbus.subscribe("joystick-button", function on_joystick_button(event) {
  dlog("Joystick button", event)
})



function button(text, handler) {
  return {
    behavior = Behaviors.Button
    rendObj = ROBJ_SOLID
    color = Color(120, 120, 180)
    size = static [sh(40), SIZE_TO_CONTENT]
    halign = ALIGN_CENTER
    children = {
      rendObj = ROBJ_TEXT
      text
      padding = sh(2)
    }
    onClick = handler
  }
}


let axisX = Watched(null)
let axisY = Watched(null)
let axisText = Computed(function() {
  const NA = "N/A"
  return $"Joystick axes: {axisX.get() ?? NA} x {axisY.get() ?? NA}"
})

function axisState() {
  return {
    watch = axisText
    rendObj = ROBJ_TEXT
    text = axisText.get()
  }
}

gui_scene.setInterval(0.05, function() {
  axisX.set(input.get_joystick_axis(0))
  axisY.set(input.get_joystick_axis(1))
})


let content = @() {
  size = SIZE_TO_CONTENT
  gap = sh(10)

  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER

  children = [
    {rendObj = ROBJ_TEXT text = "Press keys and buttons to see event log"}
    button("Click to check SPACEBAR state", function() {
      //dlog(input.get_button_state(input.buttonIds["Space"]))
      dlog(input.get_button_state("Space"))
    })
    axisState
  ]
}


return function() {
  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = 0xFF202020
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    flow = FLOW_VERTICAL
    children = content
    cursor = cursors.normal
  }
}
