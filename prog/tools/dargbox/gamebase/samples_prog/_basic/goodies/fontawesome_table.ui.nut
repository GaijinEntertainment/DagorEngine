from "%darg/ui_imports.nut" import *

let fa = require("fontawesome.map.nut")
let { ceil, sqrt } = require("math")
let { set_clipboard_text } = require("dagor.clipboard")
let scrollbar = require("samples_prog/_basic/components/scrollbar.nut")

let availHeight = sh(200)
let availWidth = sw(80)

let mkCell = @(key, cellSize) {
  rendObj = ROBJ_SOLID
  size = cellSize
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  padding = hdpx(5)
  color = Color(50, 50, 50)
  skipDirPadNav = true
  behavior = Behaviors.Button
  onClick = @() set_clipboard_text(key)
  children = {
    rendObj = ROBJ_INSCRIPTION
    text = fa[key]
    font = Fonts?.fontawesome
    color = Color(200, 200, 200)
    fontSize = hdpx(cellSize[0] * 0.45)
    validateStaticText = false
  }
}

function mkTable() {
  let count = fa.len()
  let cols = (availWidth / sqrt(availWidth * availHeight / count)).tointeger()
  let rows = ceil(count / cols)
  let size = ((availHeight - hdpx(rows)) / rows).tointeger()
  let cellSize = [size, size]
  let children = []
  local line
  local col = cols
  let keys = fa.keys().sort()
  foreach (key in keys) {
    if (++col <= cols) {
      line.append(mkCell(key, cellSize))
    } else {
      col = 0
      line = []
      children.append({
        size = [flex(), SIZE_TO_CONTENT]
        minWidth = SIZE_TO_CONTENT
        flow = FLOW_HORIZONTAL
        gap = hdpx(1)
        children = line
      })
    }
  }
  line.append({
    size = flex()
    rendObj = ROBJ_TEXT
    padding= hdpx(5)
    valign = ALIGN_CENTER
    text = "Click to copy icon key to clipboard"
  })
  return children
}

return scrollbar.makeVertScroll({
  hplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = hdpx(1)
  children = mkTable()
})
