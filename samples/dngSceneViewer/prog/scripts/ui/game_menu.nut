from "%scripts/ui/ui_library.nut" import *

let { showSettingsMenu } = require("settings/main_settings.nut")
let { exit_game } = require("app")
let { textBtn } = require("widgets/simpleComponents.nut")

let showGameMenu = mkWatched(persist, "showGameMenu", false)

let closeMenu = @() showGameMenu.set(false)
let gameMenu = @() {
  size = flex() rendObj = ROBJ_WORLD_BLUR fillColor = Color(0,0,0,220)
  padding = [sh(30),0,0,sw(30)]
  behavior = DngBhv.ActivateActionSet
  actionSet = "StopInput"
  hotkeys = [["@HUD.GameMenu", closeMenu]]
  children = {
    gap = hdpx(30)
    flow = FLOW_VERTICAL
    children = [
      textBtn("Resume", closeMenu)
      textBtn("Settings", function() {closeMenu(); showSettingsMenu.set(true)})
      textBtn("Exit", exit_game)
    ]
  }
}
return {
  gameMenu
  showGameMenu
}