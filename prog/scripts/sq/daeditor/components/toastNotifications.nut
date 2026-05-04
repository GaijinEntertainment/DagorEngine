from "%darg/ui_imports.nut" import *
from "eventbus" import eventbus_subscribe

let entity_editor = require_optional("entity_editor")
let textButton = require("textButton.nut")

const toastWidth = 125
const toastHeigth = 75
const toastVisibilityTimeout = 3

let toastData = Watched(null)

function hideToast() {
  toastData.set(null)
}

function setToastTimeout() {
  gui_scene.resetTimeout(toastVisibilityTimeout, hideToast)
}

function onToastHover(on) {
  if (!on) {
    setToastTimeout()
  }
  else {
    gui_scene.clearTimer(hideToast)
  }
}

function getToastPosition() {
  local toastX = get_mouse_cursor_pos().x
  local toastY = get_mouse_cursor_pos().y
  if (toastX < 0) {
    toastX = 0
  }
  else if (toastX > sw(100) - hdpx(toastWidth)) {
    toastX = sw(100) - hdpx(toastWidth)
  }

  if (toastY < 0) {
    toastY = 0
  }
  else if (toastY > sh(100) - hdpx(toastHeigth)) {
    toastY = sh(100) - hdpx(toastHeigth)
  }

  return [toastX, toastY]
}

function mkToastWindow() {
  return {
    size = const [hdpx(toastWidth), hdpx(toastHeigth)]
    rendObj = ROBJ_WORLD_BLUR_PANEL
    fillColor = const Color(50,50,50,220)
    borderRadius = hdpx(15)
    behavior = Behaviors.TrackMouse
    pos = getToastPosition()
    padding = fsh(1.0)
    flow = FLOW_VERTICAL

    onHover = @(on) onToastHover(on)

    children = [
      {
        valign = ALIGN_TOP
        halign = ALIGN_LEFT
        rendObj = ROBJ_TEXT
        text = toastData.get() ? $"{toastData.get().wasUndo ? "Undo:" : "Redo:"} {toastData.get().text}" : ""
      }
      {
        valign = ALIGN_BOTTOM
        halign = ALIGN_RIGHT
        size = flex()
        children = textButton("Revert", function() {
          if (toastData.get().wasUndo) {
            entity_editor?.get_instance().redoLastOperation()
          }
          else {
            entity_editor?.get_instance().undoLastOperation()
          }
          hideToast()
          }, { onHover = @(on) onToastHover(on) })
      }
    ]
  }
}

eventbus_subscribe("undoNotification", function(event){
  toastData.set({text = event.name, wasUndo = event.wasUndo})
  gui_scene.resetTimeout(toastVisibilityTimeout, hideToast)
})

let toastNotificationManager = @() {
  size = flex()
  watch = toastData
  children = toastData.get() != null ? mkToastWindow() : null
}

return {
  toastNotificationManager
}
