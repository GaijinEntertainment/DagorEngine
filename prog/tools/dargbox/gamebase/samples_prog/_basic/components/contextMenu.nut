from "daRg" import *
from "frp" import *

let {addModalWindow, removeModalWindow} = require("modalWindows.nut")

let style = {
  menuBgColor = Color(20, 30, 36)
  listItem = function (text, action) {
    let group = ElemGroup()
    let stateFlags = Watched(0)

    return @() {
      behavior = Behaviors.Button
      rendObj = ROBJ_SOLID
      color = (stateFlags.value & S_HOVER) ? Color(68, 80, 87) : Color(20, 30, 36)
      size = [flex(), SIZE_TO_CONTENT]
      group = group
      watch = stateFlags

      onClick = action
      onElemState = @(sf) stateFlags.update(sf)

      children = {
        rendObj = ROBJ_TEXT
        margin = sh(0.5)
        text = text
        group = group
        color = (stateFlags.value & S_HOVER) ? Color(255,255,255) : Color(120,150,160)
      }
    }
  }
}

local lastMenuIdx = 0

function contextMenu(x, y, width, actions, menu_style = style) {
  lastMenuIdx++
  let uid = "context_menu_{0}".subst(lastMenuIdx)
  let closeMenu = @() removeModalWindow(uid)
  let menuBgColor = menu_style?.menuBgColor ?? style.menuBgColor
  let closeHotkeys = menu_style?.closeHotkeys ?? [ ["Esc", closeMenu] ]
  function defMenuCtor(){
    return {
      rendObj = ROBJ_SOLID
      size = [width, SIZE_TO_CONTENT]
      pos = [x, y]
      flow = FLOW_VERTICAL
      color = menuBgColor
      safeAreaMargin = [sh(2), sh(2)]
      transform = {}
      behavior = Behaviors.BoundToArea

      hotkeys = closeHotkeys
      children = actions.map(@(item) menu_style.listItem(item.text,
        function () {
          item.action()
          closeMenu()
        }))
    }
  }

  let menuCtor = menu_style?.menuCtor ?? defMenuCtor
  set_kb_focus(null)
  addModalWindow({
    key = uid
    children = menuCtor()
  })
  return uid
}


return {
  contextMenu
  remove = removeModalWindow
}
