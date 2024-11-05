from "%darg/ui_imports.nut" import *

/*
  This simple example shows observable concept. Whole darg build around observables = watched object.
  Each component is table (static data) or functions which returns table, that can contain some specific properties:
  watch = list of observables
  behaviors = list of behaviors
  children = list of components

  other properties (how to render, data for behaviors and how build layout)

  If component is a function AND have not empty watch property with observables - if will be called if observable changes.

  So darg is build around MVC - MODEL is all observables, VIEW is function that is called on model changes and returns a view, and you can CONTROL model with observable.update() on Events (like mouse clicks or hotkeys)
  You can also subscribe to observable changes manually (see below)
  Thats all!
*/
let cursors = require("samples_prog/_cursors.nut")

let counter = mkWatched(persist, "counter", 0) //or model

let counter2x = Computed(@() counter.value*2)
let counter4x = Computed(@() counter2x.value*2)
let counter5x = Computed(@() counter.value*5)
let counter20xAbs = Computed(@() counter4x.value*counter5x.value)

counter2x.subscribe(@(x) vlog($"2x = {x}"))
counter4x.subscribe(@(x) vlog($"4x = {x}"))
counter5x.subscribe(@(x) vlog($"5x = {x}"))
counter20xAbs.subscribe(@(x) vlog($"|20x| = {x}"))

function button(params) {
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
local prev_val = counter.value
counter.subscribe(function(new_val) {
  if (new_val == 0 && prev_val < 0) {
    vlog("Was negative and now zero")
  }
  prev_val = new_val
})

function increment() {
  counter(counter.value + 1)
}
function decrement() {
  counter(counter.value - 1)
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    {rendObj=ROBJ_TEXT text="Click with Any Mouse Button on '+' to increment counter or on '-' to decrement"}
    {rendObj=ROBJ_TEXT text="You can also press hotkey - 'd' for Decrement counter, and 'i' for Increment"}
    button({text="Increment", onClick = increment, hotkeys=[["^I", increment]]})
    @() {rendObj = ROBJ_TEXT text=$"Counter value = {counter.value}" watch=counter }
    @() {rendObj = ROBJ_TEXT text=$"2x = {counter2x.value}, |20x| = {counter20xAbs.value}", watch=[counter2x, counter20xAbs]}
    button({text="Decrement", onClick=decrement, hotkeys=[["^D", decrement]] }) //you can also make a callback here, it can be useful for manipulating data of object
  ]
}
