from "dagor.clipboard" import set_clipboard_text
from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let state = mkWatched(persist, "state", [])

let mkCell = @(cellSize, x, y) {
  rendObj = ROBJ_SOLID
  size = cellSize
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  color = Color(50, 50, 50)
  skipDirPadNav = true
  behavior = Behaviors.Button
  onClick = @() state.modify(@(v) [].extend(v).append(x, y))
}
let gap = hdpx(5)
let mkRow = @(cellSize, y, colsNum){
  flow = FLOW_HORIZONTAL
  children = array(colsNum).map(@(_, x) mkCell(cellSize, x, y))
  gap
}
function mkTable(colsNumState, rowsNumState, widthState=null, height=null) {
  let colsNum = colsNumState.get()
  let rowsNum = rowsNumState.get()
  let width = widthState?.get() ?? sh(50)
  height = height ?? width
  let cellSize = [(width-(colsNum-1)*gap)/colsNum, (height-(rowsNum-1*gap))/rowsNum]
  return {
    flow = FLOW_VERTICAL
    gap
    watch = [colsNumState, rowsNumState, widthState]
    children = array(rowsNum).map(@(_, y) mkRow(cellSize, y, colsNum))
  }
}
let desc = {
  rendObj = ROBJ_TEXT
  padding = hdpx(5)
  text = "Click to copy poly commands to clipboard"
}
/*
let tas = {rendObj = ROBJ_TEXTAREA behavior=Behaviors.TextArea}
function ta(text, style = null) {
  let obj = (typeof text == "table")
    ? text.__merge(tas)
    : tas.__merge({text})
  return obj.__update(style??{})
}
*/
let clearBtn = comp(txt("clear"), Button, OnClick(@() state.set([])))

let tableWidth = Watched(sh(50))
let tableCols = Watched(11)
let vectorCmd = Computed(@() [].extend(state.get().map(@(v) v.tofloat()*100/(tableCols.get()-1))))

let show = @() {
  watch = state
  onClick = @() dlog(state.get()) ?? set_clipboard_text(", ".join(["VECTOR_POLY"].extend(vectorCmd.get())))
  children = @() txt(", ".join(state.get()))
}.__update(Button.value)

let polygon = function(){
  return {
    rendObj = ROBJ_VECTOR_CANVAS
    size = flex()
    watch = [vectorCmd]
    color = 0
    commands = vectorCmd.get().len() > 5 ? [
      [VECTOR_FILL_COLOR, Color(50,50,50, 50)],
      [VECTOR_POLY].extend(vectorCmd.get())
    ] : null
  }
}
return {
  flow = FLOW_VERTICAL
  vplace = ALIGN_CENTER
  gap = hdpx(10)
  children = [
    desc
    show
    clearBtn
    comp(
      @() mkTable(tableCols, tableCols, tableWidth)
      polygon
    )
  ]
}
