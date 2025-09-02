from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let showElem = Watched(false)

function extraItem() {
  let children = []
  if (showElem.get())
    children.append({
      rendObj = ROBJ_SOLID
      color = Color(0,40,0)
      size = static [300,200]
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      sound = {
        attach = "ui/onAttach"
        detach = ["ui/onDetach", {distance=123}, 0.5] //params and volume
      }
      children = {rendObj=ROBJ_TEXTAREA behavior=Behaviors.TextArea text="'Click' and 'Active' are different events (try to move cursor out of buttons after pressing LMB)\n\nThis item plays sound on appearing\\disappearing" size = flex()}
    })
  return {
    children = children
    size = SIZE_TO_CONTENT
    watch = [showElem]
    pos = [sw(60),sh(20)]
  }
}

function sampleButton() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(0,25,205)
    size = static [200,50]
    behavior = Behaviors.Button
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    onClick = @() showElem.set(false)
    sound = {
      click  = "ui/click"
      active = null
      hover  = "ui/hover"
    }
    children = {
      rendObj = ROBJ_TEXT
      text = "hide item"
    }
  }
}

function sampleButton2() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(0,205,25)
    size = static [200,50]
    behavior = Behaviors.Button
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    sound = {
      active  = "ui/active"
      hover  = "ui/hover"
    }
    children = {
      rendObj = ROBJ_TEXT
      text = "show item"
    }
    onClick = @() showElem.set(true)
  }
}

function soundsRoot() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    cursor = cursors.normal
    size = flex()
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        flow = FLOW_VERTICAL
        size = flex()
        gap = 10
        children = [
          sampleButton
          sampleButton2
        ]
      }
      extraItem
    ]
  }
}

return soundsRoot
