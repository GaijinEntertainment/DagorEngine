from "%scripts/ui/ui_library.nut" import *

let { showControlsMenu } = require("%scripts/ui/settings/input_settings.nut")
let { showSettingsMenu } = require("%scripts/ui/settings/main_settings.nut")
let { exit_game, isDisableMenu, switch_to_menu_scene, isInMainMenu } = require("%scripts/ui/app_state.nut")
let { textBtn } = require("%scripts/ui/widgets/simpleComponents.nut")
let { logout, userUid } = require("%scripts/ui/login.nut")

let showGameMenu = mkWatched(persist, "showGameMenu", false)

let closeMenu = @() showGameMenu.set(false)
let gameMenu = @() {
  size = flex() rendObj = ROBJ_WORLD_BLUR fillColor = Color(0,0,0,220)
  padding = static [sh(30),0,0,sw(30)]
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