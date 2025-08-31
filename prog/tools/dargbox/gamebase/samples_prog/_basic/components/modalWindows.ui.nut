from "%darg/ui_imports.nut" import *
let {textButton} = require("textButton.nut")
let {addModalWindow, modalWindowsComponent} = require("modalWindows.nut")
let {contextMenu} = require("contextMenu.nut")

let someWindow = freeze({
  rendObj = ROBJ_SOLID
  color = Color(0,0,0,240)
  halign = ALIGN_CENTER
//  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  size = flex()
  children = [
    {size = static [0, sh(20)]}
    {rendObj=ROBJ_TEXT text = "Modal Window" }
    {size = static [0, sh(20)]}
    {rendObj=ROBJ_TEXT text = "this is modal window. Click anywhere to close it" }
  ]
})

let someContextMenuActions = freeze([
    {text = "menu 1", action = @() dlog("menu 1 clicked")}
    {text = "menu 2", action = @() dlog("menu 2 clicked")}
])

let buttons = freeze({
  valign  = ALIGN_CENTER
  halign  = ALIGN_CENTER
  flow    = FLOW_VERTICAL
  gap  = sh(2)
  size = flex()
  padding= sh(5)
  children = [
    textButton("Open modal window", @() addModalWindow(someWindow))
    textButton("Open context menu", @(event) contextMenu(event.screenX, event.screenY, sw(10), someContextMenuActions))
  ]
})

return {
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  cursor = Cursor({ rendObj = ROBJ_IMAGE size = 32 image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32))) })
  children = [
    buttons
    modalWindowsComponent
  ]
}

