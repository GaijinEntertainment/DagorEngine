from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

//gui_scene.config.kbCursorControl = true
require("daRg").gui_scene.config.kbCursorControl = true


try {
  gui_scene.xmbMode.subscribe(function(on) {
    vlog($"XMB mode = {on}")
  })
} catch(e) {
  return {
    size = flex()
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    rendObj = ROBJ_SOLID
    color = Color(40,0,0)
    children = {
      rendObj = ROBJ_TEXT
      text = "This script uses features not avalable within current executable"
    }
  }
}


let xmbDebugCursor = Cursor({
  hotspot = [sh(2), sh(2)]
  rendObj = ROBJ_FRAME
  size = [sh(4), sh(4)]
  color = Color(255,0,0)
})


let function btn(params) {
  local mainNode

  let leftBtnNode = XmbNode({
    function onLeave(dir) {
      if (dir != null)
        gui_scene.setXmbFocus(mainNode)
    }
  })

  let rightBtnNode = XmbNode({
    function onLeave(dir) {
      if (dir != null)
        gui_scene.setXmbFocus(mainNode)
    }
  })

  mainNode = XmbNode({
    function onLeave(dir) {
      dlog($"onLeave({dir})")
      if (dir == DIR_LEFT)
        gui_scene.setXmbFocus(leftBtnNode)
      else if (dir == DIR_RIGHT)
        gui_scene.setXmbFocus(rightBtnNode)
    }
  })

  let leftButton = watchElemState(function(sf) {
    return {
      rendObj = ROBJ_BOX
      fillColor = (sf & S_HOVER) ? Color(200,200,0,200) : Color(200,100,0,200)
      borderColor = Color(220, 220, 180, 180)
      size = [sh(3), sh(3)]
      hplace = ALIGN_LEFT
      behavior = Behaviors.Button
      xmbNode = leftBtnNode
    }
  })

  let rightButton = watchElemState(function(sf) {
    return {
      rendObj = ROBJ_BOX
      fillColor = (sf & S_HOVER) ? Color(0,200,200,200) : Color(0,100,200,200)
      borderColor = Color(180, 220, 220, 180)
      size = [sh(3), sh(3)]
      hplace = ALIGN_RIGHT
      behavior = Behaviors.Button
      xmbNode = rightBtnNode
    }
  })


  return watchElemState(function(sf) {
    return {
      rendObj = ROBJ_BOX
      fillColor = (sf & S_HOVER) ? Color(60,80,100) : Color(0,60,75)
      borderColor = (sf & S_HOVER) ? Color(255,255,255) : Color(160,160,160)
      borderWidth = 2
      size = [flex(), SIZE_TO_CONTENT]
      behavior = Behaviors.Button
      function onClick() {
        vlog($"{params.text}")
      }
      xmbNode = mainNode
      halign = ALIGN_CENTER
      padding = sh(2)
      children = [
        leftButton
        {
          rendObj = ROBJ_TEXT
          text = params.text
        }
        rightButton
      ]
    }.__merge(params)
  })
}


let function hPanel() {
  let children = array(20).map(function(_, idx) {
    return btn({
      text = idx
    })
  })

  return function() {
    return {
      watch = gui_scene.xmbMode
      rendObj = ROBJ_SOLID
      color = gui_scene.xmbMode.value ? Color(30,30,80) : Color(0,0,50)
      size = [sh(40), flex()]
      flow = FLOW_VERTICAL
      xmbNode = XmbNode({
        canFocus = false
        //wrap = false
      })
      children = children
      gap = sh(2)
      padding = sh(2)
      joystickScroll = true
      behavior = Behaviors.WheelScroll
    }
  }
}


return function Root() {
  return {
    size = flex()
    watch = gui_scene.xmbMode
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    cursor = gui_scene.xmbMode.value ? /*null*/ xmbDebugCursor : cursors.normal
    padding = sh(10)
    gap = sh(10)
    halign = ALIGN_CENTER
    clipChildren = true
    flow = FLOW_HORIZONTAL

    xmbNode = XmbNode({
      canFocus = false
    })

    children = [
      hPanel()
      hPanel()
    ]
  }
}
