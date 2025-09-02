from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")


function makeCells() {
  let colors = [
    Color(200,30,20), Color(20,200,30), Color(0,30,200),
    Color(180,160,20), Color(150,30,170), Color(50,140,160),
    Color(140,160,180)
  ]


  let rows = []
  local colorIdx = 0
  for (local i=0; i<10; ++i) {
    let cols = []
    for (local j=0; j<16; ++j, ++colorIdx) {
      cols.append({
        rendObj = ROBJ_SOLID
        size = sh(17)
        color = colors[colorIdx % colors.len()]
      })
    }
    rows.append({
      flow = FLOW_HORIZONTAL
      size = SIZE_TO_CONTENT
      children = cols
    })
  }

  let container = {
    size = static [sw(80), sh(80)]
    rendObj = ROBJ_FRAME
    color = Color(220, 220, 0)
    borderWidth = 4
    padding = 4

    children = {
      size = flex()
      clipChildren = true

      children = {
        size = flex()
        behavior = Behaviors.Pannable

        children = {
          size = SIZE_TO_CONTENT
          flow = FLOW_VERTICAL
          children = rows
        }
      }
    }
  }

  return container
}


let Root = {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = makeCells()
  cursor = cursors.normal
}

return Root
