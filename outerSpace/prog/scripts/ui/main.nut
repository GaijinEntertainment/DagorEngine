#default:forbid-root-table

from "%darg/ui_imports.nut" import *
from "%scripts/ui/widgets/simpleComponents.nut" import normalCursor
from "%scripts/ui/widgets/msgbox.nut" import msgboxComponent, hasMsgBoxes
import "console" as console
from "frp" import warn_on_deprecated_methods //set_nested_observable_debug, set_subscriber_validation
from "dagor.system" import DBGLEVEL

//set_nested_observable_debug( DBGLEVEL>0)
//set_subscriber_validation( DBGLEVEL>0)
warn_on_deprecated_methods( DBGLEVEL>0)

let ecs = require_optional("ecs")
ecs?.clear_vm_entity_systems()

require("%scripts/ui/ui_config.nut")
require("%scripts/ui/settings/graphics_options.nut").graphicsPresetApply()
let { mainMenu, background} = require("%scripts/ui/main_menu.nut")
let { showGameMenu, mkHud, mkDebriefing} = require("%scripts/ui/hud.nut")
let { sessionResult, isInMainMenu, isLoadingState } = require("%scripts/ui/app_state.nut")
let { editor, showUIinEditor, editorIsActive} = require("%scripts/ui/editor.nut")
let { take_screenshot_nogui, take_screenshot} = require("screencap")
let { showControlsMenu, controlsMenuUi } = require("%scripts/ui/settings/input_settings.nut")
let { showSettingsMenu, settingsMenuUi } = require("%scripts/ui/settings/main_settings.nut")
let { showLicense, licenseUi } = require("%scripts/ui/licenseTxt.nut")
let inspectorToggle = require("%darg/helpers/inspector.nut")


let eventHandlers = {
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
  if (showLicense.get())
    children.append(licenseUi)

  let showCursor = isInMainMenu.get() || hasMsgBoxes.get() || showGameMenu.get() || showControlsMenu.get() || showSettingsMenu.get() || showLicense.get()
  return {
    watch = [editorIsActive, showUIinEditor, isInMainMenu, sessionResult, isLoadingState, hasMsgBoxes, showGameMenu, showControlsMenu, showSettingsMenu, showLicense]
    size = flex()
    children
    eventHandlers
  }.__update( showCursor ? {cursor = normalCursor} : {})
}