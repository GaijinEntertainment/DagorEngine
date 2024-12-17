from "%scripts/ui/ui_library.nut" import *
from "dagor.system" import dgs_get_settings

let ecs = require("%dngscripts/ecs.nut")

let { get_setting_by_blk_path, set_setting_by_blk_path_and_save, save_changed_settings } = require("settings")
let { apply_video_settings } = require("videomode")
let { mkCombo } = require("%scripts/ui/widgets/simpleComponents.nut")
let { mkSettingsOption } = require("options_lib.nut")

let presets = []
foreach (block in (dgs_get_settings()?["consoleGraphicalPresets"] ?? {}))
  presets.append(block.getBlockName())

let allowCustomSettingsQuery = ecs.SqQuery("allowCustomSettingsQuery", {comps_rw=[["settings_override__useCustomSettings", ecs.TYPE_BOOL], ["settings_override__graphicsSettings", ecs.TYPE_OBJECT]]})
let setCustomSetting = function(blkPath, v) {
  allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__useCustomSettings"]=false)
  defer(function() {
    allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__graphicsSettings"][blkPath] = v)
    defer(@() allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__useCustomSettings"]=true))
  })
}
let getCustomSetting = @(blkPath) allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__graphicsSettings"][blkPath])

function optionCtor(opt) {
  let var = opt?.var ?? Watched(opt.defVal)
  let setValue = opt?.setValue ?? function(v) {
    var.set(v)
    set_setting_by_blk_path_and_save(opt.blkPath, v)
  }
  return mkSettingsOption(opt.name, opt.widgetCtor(opt.__merge({var, setValue})))
}

let postfxOpts = [
  {
    name = "Chromatic Aberration"
    blkPath = "graphics/chromaticAberration"
    widgetCtor = mkCombo
    values = [true, false]
  },
  {
    name = "Motion Blur"
    blkPath = "graphics/motionBlur"
    widgetCtor = mkCombo
    values = [true, false]
  },
  {
    name = "Sharpening"
    blkPath = "graphics/sharpening"
    widgetCtor = mkCombo
    values = [0.0, 1.0, 10.0, 50.0, 100.0]
  },
  {
    name = "Lense Flares"
    blkPath = "graphics/lensFlares"
    widgetCtor = mkCombo
    values = [true, false]
  },
].map(function(opt){
  opt.var <- Watched(getCustomSetting(opt.blkPath))
  opt.setValue <- function(v){
    opt.var.set(v)
    setCustomSetting(opt.blkPath, v)
  }
  return optionCtor(opt)
})


const graphicsPresetBlkPath = "graphics/consolePreset"
let graphicsPreset = Watched(get_setting_by_blk_path(graphicsPresetBlkPath) ?? "ultra")

function getVideoSettings(){
  let res = {graphics={}, video={}}
  let settings = dgs_get_settings()
  foreach (k, r in res){
    foreach (opt, val in (settings?[k] ?? {}))
      r[opt] <- val
  }
  return res
}
function graphicsPresetUpdate(presetName) {
  let changedSettings = []
  let prev = getVideoSettings()
  graphicsPreset.set(presetName)
  set_setting_by_blk_path_and_save(graphicsPresetBlkPath, presetName)
  save_changed_settings([graphicsPresetBlkPath])
  let new = getVideoSettings()
  foreach (r, v in prev) {
    foreach (opt, val in v)
      if (val != new[r]?[opt])
        changedSettings.append($"{r}/{opt}")
  }
  foreach (r, v in new) {
    foreach (opt, val in v) {
      let optKey = $"{r}/{opt}"
      if (val != prev[r]?[opt] && optKey not in changedSettings)
        changedSettings.append(optKey)
    }
  }
  apply_video_settings(changedSettings)
}

let optPreset = optionCtor({
  name = "Quality Preset"
  var = graphicsPreset
  blkPath = graphicsPresetBlkPath
  widgetCtor = mkCombo
  defVal = "ultra"
  setValue = graphicsPresetUpdate
  values = presets
})

let optDriver = optionCtor({
  name = "Driver"
  widgetCtor = mkCombo
  blkPath = "video/driver"
  defVal = "auto"
  values = ["auto", "dx11", "dx12", "vulkan"]
  restart = true
})

let graphicsOptions = [optPreset].extend(postfxOpts)//, optDriver]

return {
  graphicsOptions
  graphicsPresetUpdate
  graphicsPresetApply = @() graphicsPresetUpdate(get_setting_by_blk_path(graphicsPresetBlkPath))
  optDriver
}