from "%darg/ui_imports.nut" import *
from "dagor.workcycle" import resetTimeout
import "components/style.nut" as style
from "frp" import set_nested_observable_debug, set_subscriber_validation, warn_on_deprecated_methods
from "dagor.system" import DBGLEVEL

//set_nested_observable_debug( DBGLEVEL>0)
//set_subscriber_validation( DBGLEVEL>0)
warn_on_deprecated_methods( DBGLEVEL>0)

let {Preferences} = require("components/preferences.nut")
let {set_app_window_title, dgs_get_settings} = require("dagor.system")
let {system} = require("system")
let {mkSelect, modalWindowsComponent, mkCombo, mkButton, mkCheckbox, cursors} = require("components/components.nut")
let {scheme, gameSchemaById, killGame} = require("schema.nut")
let {set_clipboard_text} = require("dagor.clipboard")
let fa = require("%darg/helpers/fontawesome.map.nut")
let buildStrByTemplates = require("templateStr.nut")
let {file_exists} = require("dagor.fs")

let allScenes = ["planet_orbit.blk", "gaija_planet.blk"]

let buildCommandLines = function(commands, params={}, formatters={}) {
  local cmds = commands.map(@(cmd) buildStrByTemplates(cmd, params.map(@(v) v instanceof Watched ? v.get() : v), formatters))
  local final_cmds = []
  foreach (ln in cmds)
    if (ln != "")
      final_cmds.append(ln)
  return final_cmds
}

const game = "OuterSpace"

set_app_window_title($"{game} Dev Launcher")

gui_scene.setConfigProps({
  defTextColor = style.NORMAL_TXT
  defaultFontSize = style.NORMAL_FONT_SIZE
})

let txt = @(text, override = null ) {rendObj = ROBJ_TEXT text color = style.NORMAL_TXT}.__update(override ?? {})

function mkSel(options, title="", defoptidx=0){
  let watch = Watched(options[defoptidx])
  return [watch, mkCombo(watch, options, title)]
}

function get_executable_targets(){
  let host_platform = require("platform").get_host_platform()
  let host_arch = require("platform").get_host_arch()
  local list = []
  if (host_platform == "windows") {
    if (host_arch == "arm64")
      list.append("windows-arm64")
    list.append("windows-x86_64", "windows-x86")
  } else if (host_platform == "macOS") {
    list.append("macOS-x86_64")
    if (host_arch == "arm64")
      list.append("macOS-arm64")
  } else if (host_platform == "linux")
    list.append("-".concat(host_platform, host_arch))
  if (list.len() == 0)
    list.append("")
  return list
}
function get_d3d_targets(){
  let host_platform = require("platform").get_host_platform()
  let host_arch = require("platform").get_host_arch()
  local list = ["game defined"]
  if (host_platform == "windows") {
    list.append("dx11")
    if (["x86_64", "arm64"].contains(host_arch))
      list.append("dx12", "vulkan")
  }
  if (host_platform == "linux")
    list.append("vulkan")
  if (host_platform == "macOS")
    list.append("Metal")
  return list
}

let [selectedPlatform, comboPlatform] = mkSel(get_executable_targets(), "Platform:")
let [playersNum, comboPlayersNum] = mkSel([1,2,3,4], "Players num:")
let [selectedVideoDriver, comboVideoDriver] = mkSel(get_d3d_targets(), "Video Driver:")
//let [selectedQualityPreset, comboQualityPreset] = mkSel(["minimum", "low","medium","high", "ultra"], "Quality Preset:", 3)
let [selectedVromsMode, comboVromsMode] = mkSel(["game defined", "vroms", "source"], "Vroms:")
let allSchemas     = WatchedRo(scheme.schema.map(@(v) [v.id, v.title]) ?? [])
let selectedSchema = Watched(allSchemas.get()?[0][0])
let comboSchemas   = mkCombo(selectedSchema, allSchemas, "Launch:")
//let [selectedScene, comboScene ] = mkSel(allScenes, "Scene:")
let selectedScene = Watched(allScenes?[0])
let comboScene = mkSelect({options = allScenes, selected = selectedScene, label = "Scene: ", title="Select Scene"}).selectBtn

let allWindowModes = ["game defined", "fullscreenwindowed", "fullscreen", "windowed"]

//let consoleCmdsEnabled = Watched(false)

let windowMode  = Watched(allWindowModes[0])
let maxTimeLimit = Watched(12)
let maxTimeLimitCombo = mkCombo(maxTimeLimit, [ 0, 2, 5, 8, 10, 12, 15, 45], "Time limit (min):")

let enableSound = Watched("")
let enableSoundChck = mkCheckbox(enableSound, "Enable Sound:", {maxWidth=sh(12)})

let dasDebug = Watched("")
let dasDebugChck = mkCheckbox(dasDebug, "Das Debug:", {maxWidth=sh(12)})

let selections = {
  schema      = selectedSchema
  scene       = selectedScene
  platform    = selectedPlatform
  videoDriver = selectedVideoDriver
  vromsMode   = selectedVromsMode
//  quality = selectedQualityPreset
  maxTimeLimit
  playersNum
  windowMode
  enableSound
  dasDebug
}.map(@(v) v instanceof Watched ? {get = @() v.get(), watch = [v], set = @(n) v.set(n)} : v)

let effectiveSchema = Computed(@() selectedSchema.get())

let hdr = txt("Outer Space Launch Settings", {fontSize = style.H_FONT_SIZE, color = Color(100,100,120, 120) margin = [0,0,hdpx(20), 0]})

let {presetsList} = Preferences({
  fileName = $".saved_selection.ver1.{game}.json",
  selections,
  filterKeysForBuiltin = ["videoDriver","vromsMode", "platform", "dedPlatform"]
})

let buildCmdLines = @() buildCommandLines(gameSchemaById[effectiveSchema.get()]["commands"],
  {
    game         = "outer_space"
    dasDebug
    scene        = selectedScene.get()
    platform     = selectedPlatform.get()
    videoDriver  = selectedVideoDriver.get()
    vromsMode    = selectedVromsMode.get()
    playersNum = playersNum.get()
    maxTimeLimit = maxTimeLimit.get()
//    quality = selectedQualityPreset
    windowMode
    enableSound
  },
  scheme.formatters
)

let killButton = mkButton({
  txtIcon = fa["close"]
  onClick = @() killGame()
  hotkeys = [["^L.Ctrl X | R.Ctrl X"]]
  tooltip = "Kill all outer spaces apps\nhotkey: Ctrl+X"
})

let launchPanelWidth = calc_str_box({fontSize = style.C_FONT_SIZE text = "DO AA RESTART"})[0]

let closeAndStartButton = mkButton({
  txtIcon = "{0} {1}".subst(fa["rotate-right"], fa["play"])
  fontSize = style.C_FONT_SIZE*0.75
  onClick = function() {
    killGame()
    buildCmdLines().apply(@(val, idx) resetTimeout(idx, @() system(val), $"launch_{idx}"))
  }
  text = "(RE)START APP"
  hotkeys = [["^L.Ctrl Space | R.Ctrl Space"]]
  tooltip = "Kill all and Start\nhotkey: Ctrl+Space"
  override = {minWidth = launchPanelWidth}
})

let startButton = mkButton({
  txtIcon = fa["play"]
  //text = "START"
  onClick = @() buildCmdLines().apply(@(val, idx) resetTimeout(idx, @() system(val), $"launch_{idx}"))
  //fontSize = style.C_FONT_SIZE
  hotkeys = [["^L.Shift Space | R.Shift Space"]]
  tooltip = "hotkey: Shift+Space"
  //override = {minWidth = launchPanelWidth}
})

let clipboardButton = mkButton({
  txtIcon = fa["clipboard"]
  function onClick() {
    let windows_host = selectedPlatform.get().startswith("windows-")
    // splitting by ' -' to avoid console commands being split as they can look like 'command.name param1 param2'
    let cmds = windows_host ? buildCmdLines().map(@(v) " ^\n  -".join(v.split(" -"))) : buildCmdLines().map(@(v) v)
    if (!windows_host && cmds.len() == 1)
      cmds[0] = $"{cmds[0]} &"

    // at least 2 seconds because Windows will wait until the next second, so timeout 1 might actually happen in the same second
    set_clipboard_text(windows_host ? " & timeout 2\n".join(cmds) : " read -t 2\n".join(cmds))
    vlog("Copied to clipboard")
  }
  tooltip = "Copy launch cmd to Clipboard\nhotkey: Ctrl+C"
  hotkeys = [["^L.Ctrl C | R.Ctrl C"]]
})

let launchPanel = {
  size = flex()
  valign = ALIGN_BOTTOM
  halign = ALIGN_RIGHT
  flow = FLOW_VERTICAL
  gap = hdpx(10)
  padding = hdpx(10)
  children = [
    presetsList
    {size = flex()}
    closeAndStartButton
  ]
}
let controls = function() {
  return {
    padding = [sh(8), hdpx(2), hdpx(2), sh(6)]
    size = [sw(32), flex()]
    watch = selectedSchema
    flow = FLOW_VERTICAL
    gap = hdpx(20)
    children = [
      comboSchemas,
      selectedSchema.get()=="menu" ? null : comboScene,
      selectedSchema.get()=="client_server" ? comboPlayersNum : null,
      ["local", "client_server"].contains(selectedSchema.get()) ? maxTimeLimitCombo : null,
      comboPlatform,
      comboVideoDriver,
//      comboQualityPreset,
      comboVromsMode,
      enableSoundChck,
      dasDebugChck,
    ]
  }
}

let toolBar = {
  hplace = ALIGN_RIGHT
  gap = hdpx(2)
  size = [flex(), SIZE_TO_CONTENT]
  children = [hdr, {size = [flex(), 0]}, clipboardButton, killButton, startButton]
  flow = FLOW_HORIZONTAL
}
let bottomBar = {size = [flex(), hdpx(50)]} // ?? status?

function niceLauncher(){
  const bkg = "launcher_back.png"
  let use_image = file_exists(bkg)

  let host_platform = require("platform").get_host_platform()
  let host_arch = require("platform").get_host_arch()
  print($"running on host: platform={host_platform} arch={host_arch}\n")

  return {
    size = flex()
    cursor = dgs_get_settings()?["use_system_cursor"] ? cursors.hidden : cursors.normal
    rendObj = ROBJ_SOLID
    color = style.backgroundColor
    behavior = Behaviors.Button
    onClick = @() set_kb_focus(null)
    children = [
      use_image ? {image = Picture(bkg) rendObj = ROBJ_IMAGE, keepAspect = KEEP_ASPECT_FILL size=flex()} : null,
      {
        size = flex()
        gap = hdpx(10)
        padding = hdpx(30)
        flow = FLOW_VERTICAL
        children = [
          toolBar
          {
            children = [controls, launchPanel]
            size = flex()
            flow = FLOW_HORIZONTAL
          }
          bottomBar
        ]
      },
      modalWindowsComponent
    ]
  }
}
return niceLauncher