from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

function item(text) {
  return {
    color = Color(200, 50, 50)
    size = [flex(), SIZE_TO_CONTENT]
    minWidth = SIZE_TO_CONTENT

    children = [
      {
        rendObj = ROBJ_SOLID
        color = Color(20, 20, 180)
        size = [pw(100), ph(100)]
      }
      {
        margin = 5
        rendObj = ROBJ_TEXT
        text = text
      }
    ]
  }
}

let menu = {
  pos = [200, 200]
  size = SIZE_TO_CONTENT
  rendObj = ROBJ_SOLID
  color = Color(50, 200, 50)
  flow = FLOW_VERTICAL
  padding = 5
  gap = 5

  children = [
    item("blablah")
    item("1234567890")
    item("exit")
    item("?")
  ]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(10,30,50)
  size =flex()
  cursor = cursors.active

  children = menu
}
