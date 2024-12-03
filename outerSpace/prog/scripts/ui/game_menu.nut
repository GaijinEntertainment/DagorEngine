from "%scripts/ui/ui_library.nut" import *

let { showControlsMenu } = require("settings/input_settings.nut")
let { showSettingsMenu } = require("settings/main_settings.nut")
let { exit_game, isDisableMenu, switch_to_menu_scene, isInMainMenu } = require("app_state.nut")
let { textBtn } = require("widgets/simpleComponents.nut")
let { logout, userUid } = require("login.nut")

let showGameMenu = mkWatched(persist, "showGameMenu", false)

let closeMenu = @() showGameMenu.set(false)
let gameMenu = @() {
  size = flex() rendObj = ROBJ_WORLD_BLUR fillColor = Color(0,0,0,220)
  padding = [sh(30),0,0,sw(30)]
  behavior = DngBhv.ActivateActionSet
  actionSet = "StopInput"
  hotkeys = [["@HUD.GameMenu", closeMenu]]
  watch = [isInMainMenu, userUid]
  children = {
    gap = hdpx(30)
    flow = FLOW_VERTICAL
    children = [
      textBtn("Resume", closeMenu)
      textBtn("Controls", function() {closeMenu(); showControlsMenu.set(true)})
      textBtn("Settings", function() {closeMenu(); showSettingsMenu.set(true)})
      !isDisableMenu && !isInMainMenu.get() ? textBtn("Leave", function() {
        closeMenu()
        switch_to_menu_scene()
      }) : null
      isInMainMenu.get() && userUid.get()!=null ? textBtn("Logout", function(){ closeMenu(); logout();}) : null
      textBtn("Exit", exit_game)
    ]
  }
}
return {
  gameMenu
  showGameMenu
}