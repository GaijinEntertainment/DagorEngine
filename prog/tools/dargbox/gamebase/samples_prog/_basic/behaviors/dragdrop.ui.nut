from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let itemSize = [sh(10), sh(10)]
/*
Known issues and questions:
  - probably it is better to check drop availability with element box (2d math, check intersect of boxes with box) like
       bool DoBoxesIntersect(Box a, Box b) { return (abs(a.x - b.x) * 2 < (a.width + b.width)) && (abs(a.y - b.y) * 2 < (a.height + b.height));}
     or
       https://github.com/RandyGaul/tinyheaders/blob/master/tinyc2.h
     for any shaped collision check
*/

let draggedData = Watched(null)


function draggable(data) {
  function builder(sf) {
    return {
      rendObj = ROBJ_BOX
      fillColor = (sf & S_HOVER) ? Color(200,200,200) : Color(200,20,20)
      borderWidth = (sf & S_HOVER) ? 2 : 0
      borderColor = Color(255,180,50)
      opacity = (sf & S_DRAG) ? 0.5 : 1.0
      size = itemSize
      transform = {}
      behavior = [Behaviors.DragAndDrop]
      clipChildren = true
      children = { rendObj = ROBJ_TEXT text=data hplace=ALIGN_CENTER vplace=ALIGN_CENTER}

      dropData = data
      onDragMode = function(on, val) {
        draggedData.set(on ? val : null)
      }
      onDoubleClick = function(event) {
        if (event.button == 0) {
          vlog("Left button double click")
        }
      }
    }
  }
  return watchElemState(builder)
}


function target(label, color, can_drop) {
  let labelText = {
    rendObj = ROBJ_TEXT text=label hplace=ALIGN_CENTER vplace=ALIGN_CENTER
  }

  let dropMarker = {
    rendObj = ROBJ_SOLID
    size = flex()
    color = Color(0,0,0,0)
    animations = [
      { prop=AnimProp.color, from=Color(0,0,0,0), to=Color(80,80,40,40), duration=0.8, play=true, loop=true, easing=CosineFull }
    ]
  }

  function builder(sf) {
    let needMark = draggedData.get() && (!can_drop || can_drop(draggedData.get()))
    return {
      rendObj = ROBJ_BOX
      fillColor = color
      borderColor = Color(255,180,50)
      borderWidth = (sf & S_ACTIVE) ? 2 : 0
      size = itemSize

      watch = draggedData
      transform = {}
      behavior = Behaviors.DragAndDrop
      onDrop = function(data) {
        vlog($"{label} box: dropped data = {data}")
      }
      canDrop = can_drop
      children = [
        labelText
        needMark ? dropMarker : null
      ]
    }
  }
  return watchElemState(builder)
}


function pane(items) {
  return {
    padding = sh(2)
    gap = sh(2)
    flow = FLOW_VERTICAL
    size = SIZE_TO_CONTENT
    children = items
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(10,10,10)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = flex()

  children = {
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    cursor = cursors.normal
    flow = FLOW_HORIZONTAL
    gap = sh(5)

    children = [
      pane([
        draggable("A")
        draggable("B")
      ])
      pane([
        target("A", Color(50,50,220), @(data) data=="A")
        target("A|B", Color(20,160,20), null)
      ])
    ]
  }
}
