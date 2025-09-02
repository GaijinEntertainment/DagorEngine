from "%darg/ui_imports.nut" import *

let mkButtonFactory = @(overrides = {}) function() {
  let sf = Watched(0)
  return @() {
    watch = sf
    rendObj = ROBJ_SOLID
    size = 30
    color = sf.get() & S_ACTIVE ? 0xFFFFFFFF
      : sf.get() & S_HOVER ? 0x40996633
      : 0x10333333
    behavior = Behaviors.Button
    onElemState = @(val) sf.set(val)
  }.__update(overrides)
}

let mkOverlapping = mkButtonFactory()
let mkMouseBlocked = mkButtonFactory({ stopMouse = true })

let mkButtonsArray = @(btnComp) {
  flow = FLOW_VERTICAL
  children = array(10).map(@(_) {
    flow = FLOW_HORIZONTAL
    children = array(10).map(@(_) btnComp())
  })
}

let buttonsOverlapping = mkButtonsArray(mkOverlapping)
let buttonsMouseBlocked = mkButtonsArray(mkMouseBlocked)

// BUG:
// move mouse cursor precisely to the internal corners of
// buttons and click
//
// if it is made on the left block of buttons you can encounter
// that two or four elements interpreted as hovered, but only
// one element became active on mouse click
//
// on the right block only one button is hovered and activated
//
// layout of elements is correct, but bounding box of tracking
// hover state is 1 pixel larger by both dimensions

return {
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  flow = FLOW_HORIZONTAL
  gap = 100
  children = [
    buttonsOverlapping
    buttonsMouseBlocked
  ]
}