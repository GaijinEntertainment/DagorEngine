from "%darg/ui_imports.nut" import *

let cursorC = Color(255,255,255,255)
let normalCursor = Cursor({
  rendObj = ROBJ_VECTOR_CANVAS
  size = [fsh(2), fsh(2)]
  hotspot = [0, 0]
  commands = [
    [VECTOR_WIDTH, hdpx(1)],
    [VECTOR_FILL_COLOR, cursorC],
    [VECTOR_COLOR, Color(20, 40, 70, 250)],
    [VECTOR_POLY, 0,0, 100,50, 56,56, 50,100],
  ]
    transform = {
      pivot = [0, 0]
      rotate = 29
    }
})

let menuBtnTextColorNormal = Color(150,250,200)
let menuBtnTextColorHover = Color(255,255,255)
let menuBtnFillColorNormal = 0
let menuBtnFillColorHover = Color(50,50,50,60)

let mkCorner = @(borderWidth, style=null) freeze({rendObj = ROBJ_FRAME borderWidth color = Color(105,105,105,105) size = [hdpx(10), hdpx(10)]}.__update(style ?? {}))
let ulCorner = mkCorner([1,0,0,1])
let urCorner = mkCorner([1,1,0,0], {hplace = ALIGN_RIGHT})
let drCorner = mkCorner([0,1,1,0], {hplace = ALIGN_RIGHT, vplace = ALIGN_BOTTOM})
let dlCorner = mkCorner([0,0,1,1], {vplace = ALIGN_BOTTOM})
let corners = freeze({children = [ulCorner, dlCorner, urCorner, drCorner] size = flex()})

function menuBtn(text, onClick, style = null) {
  let stateFlags = Watched(0)
  let keyOnHover = {}
  let key = {}

  return @() {
    hotkeys = stateFlags.get() & S_HOVER ? [["Enter | Space"]] : null
    key = stateFlags.get() & S_HOVER ? keyOnHover : key
    watch = stateFlags
    behavior = Behaviors.Button
    onClick
    onElemState = @(s) stateFlags.set(s)
    rendObj = ROBJ_SOLID
    color = stateFlags.get() & S_HOVER ? menuBtnFillColorHover : menuBtnFillColorNormal
    children = [
      corners
      {
        padding = [hdpx(4), hdpx(10)]
        children = {
          rendObj = ROBJ_TEXT
          text
          color = stateFlags.get() & S_HOVER ? menuBtnTextColorHover : menuBtnTextColorNormal
          fontSize = hdpx(20)
        }
      }
    ]
  }.__update(style ?? {})
}
return {
  normalCursor
  menuBtn
}