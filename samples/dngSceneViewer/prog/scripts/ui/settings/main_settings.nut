from "%scripts/ui/ui_library.nut" import *

let {menuBtn} = require("%scripts/ui/widgets/simpleComponents.nut")
let { graphicsOptions } = require("graphics_options.nut")
let {audio_options} = require("audio_options.nut")

let showSettingsMenu = mkWatched(persist, "showSettings", false)
let close = @() showSettingsMenu.set(false)
let closeBtn = menuBtn("Close", close)
let buttons = {size = [flex(), SIZE_TO_CONTENT] children = closeBtn halign = ALIGN_RIGHT padding = hdpx(5)}
let title = {rendObj = ROBJ_TEXT text = "SETTINGS" fontSize=hdpx(40) padding = hdpx(5)}
let subTitle = @(text) {rendObj = ROBJ_TEXT text}

let settings = {
  padding = [hdpx(40), hdpx(20)]
  rendObj = ROBJ_SOLID color = Color(0,0,0,80)
  flow = FLOW_VERTICAL
  gap = hdpx(10)
  size = flex()
  halign = ALIGN_CENTER
  children = [subTitle("Graphics")].extend(graphicsOptions).append(subTitle("Sound")).extend(audio_options)
}

function settingsMenuUi(){
  return {
    size = flex()
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = {
      rendObj = ROBJ_WORLD_BLUR
      fillColor = Color(30,30,30,230)
      padding = hdpx(5)
      size = [sw(66), sh(66)]
      flow = FLOW_VERTICAL
      children = [title, settings, buttons]
      hotkeys = [["Esc", close]]
    }
  }
}

return {
  showSettingsMenu
  settingsMenuUi
}