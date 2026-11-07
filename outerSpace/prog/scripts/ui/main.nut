#default:forbid-root-table

from "%darg/ui_imports.nut" import *
from "%scripts/ui/widgets/simpleComponents.nut" import normalCursor
from "%scripts/ui/widgets/msgbox.nut" import msgboxComponent, hasMsgBoxes
import "console" as console
from "screencap" import take_screenshot_nogui, take_screenshot
import "%darg/helpers/inspector.nut" as inspectorToggle

//set_subscriber_validation( DBGLEVEL>0)

let ecs = require_optional("ecs")
// ES registered by a module loaded before this wipe would be deleted by it,
// so app_state.nut and everything reaching it must stay a body-level require
ecs?.clear_vm_entity_systems()

require("%scripts/ui/ui_config.nut")
require("%scripts/ui/settings/graphics_options.nut").graphicsPresetApply()
let { mainMenu, background } = require("%scripts/ui/main_menu.nut")
let { showControlsMenu, controlsMenuUi } = require("%scripts/ui/settings/input_settings.nut")
let { showGameMenu, mkHud, mkDebriefing} = require("%scripts/ui/hud.nut")
let { sessionResult, isInMainMenu, isLoadingState } = require("%scripts/ui/app_state.nut")
let { editor, showUIinEditor, editorIsActive} = require("%scripts/ui/editor.nut")
let { showSettingsMenu, settingsMenuUi } = require("%scripts/ui/settings/main_settings.nut")
let { showLicense, licenseUi } = require("%scripts/ui/licenseTxt.nut")


const eventHandlers = {
  ["Global.Screenshot"] = @(...) take_screenshot(),
  ["Global.ScreenshotNoGUI"] = @(...) take_screenshot_nogui()
}

console.register_command(@() inspectorToggle(), "ui.inspector")

let debriefing = mkDebriefing(true)
let loadingUi = {
  size = flex()
  children = [
    background
    {size = flex() halign = ALIGN_RIGHT valign = ALIGN_BOTTOM padding = sh(5) children = {rendObj = ROBJ_TEXT text = "Loading..." fontSize = hdpx(80)} }
  ]
}
return function(){
  let backgroundComp = isInMainMenu.get() ? background : null
  let children = isLoadingState.get()
    ? [loadingUi]
    : showLicense.get()
      ? [licenseUi]
      : isInMainMenu.get()
        ? [sessionResult.get() ? debriefing : mainMenu ]
        : [ mkHud() ]
  if (showControlsMenu.get())
    children.replace([backgroundComp, controlsMenuUi])
  if (showSettingsMenu.get())
    children.replace([backgroundComp, settingsMenuUi])
  children.append(msgboxComponent)
  if (editorIsActive.get()) {
    if (!showUIinEditor.get())
      children.clear()
    children.append(editor)
  }

  let showCursor = isInMainMenu.get() || hasMsgBoxes.get() || showGameMenu.get() || showControlsMenu.get() || showSettingsMenu.get() || showLicense.get()
  return {
    watch = [editorIsActive, showUIinEditor, isInMainMenu, sessionResult, isLoadingState, hasMsgBoxes, showGameMenu, showControlsMenu, showSettingsMenu, showLicense]
    size = flex()
    children
    eventHandlers
  }.__update( showCursor ? (const {cursor = normalCursor}) : {})
}