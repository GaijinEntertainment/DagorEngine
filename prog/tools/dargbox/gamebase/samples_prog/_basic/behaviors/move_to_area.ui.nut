from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let labels = [
  "foo"
  "barbaz"
  "snafu"
  "111"
  "abracadabra"
]

let activeSection = Watched(labels[0])
let markerTarget = MoveToAreaTarget()

let function requestMoveToElem(elem) {
  let x = elem.getScreenPosX()
  let y = elem.getScreenPosY()
  let w = elem.getWidth()
  let h = elem.getHeight()
  markerTarget.set(x,y,x+w,y+h)
}

let function item(label) {
  return watchElemState(@(sf) {
    watch = [activeSection]
    rendObj = ROBJ_TEXT
    text = label
    padding = sh(3)
    color = (sf & S_ACTIVE) ? Color(250,250,250)
              : (sf & S_HOVER) ? Color(200, 220, 210)
              : Color(180,180,180)
    behavior = [Behaviors.Button, Behaviors.RecalcHandler]
    function onClick(evt) {
      activeSection(label)
      requestMoveToElem(evt.target)
    }
    hotkeys = [["^J:A", {action = @() activeSection(labels[4])}]]
    function onRecalcLayout(initial, elem) {
      if (initial && label == activeSection.value) {
        requestMoveToElem(elem)
      }
    }
  })
}


let items = labels.map(item)

let marker = {
  rendObj = ROBJ_SOLID
  color=Color(40,60,180,180)
  behavior = Behaviors.MoveToArea
  target = markerTarget
  //speed = 800
  viscosity = 0.1
}

let container = {
  size = [sw(80), sh(80)]
  rendObj = ROBJ_FRAME
  borderWidth = sh(1)
  padding = sh(5)
  fillColor = Color(80, 80, 80)
  borderColor = Color(200,200,200)
  halign = ALIGN_CENTER
  //clipChildren = true
  children = [
    marker
    {
      flow = FLOW_HORIZONTAL
      children = items
    }
  ]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = container
}
