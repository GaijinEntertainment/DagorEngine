from "%scripts/ui/ui_library.nut" import *

let ecs = require("%dngscripts/ecs.nut")

let { showWarning } = require("%scripts/ui/widgets/msgbox.nut")
let { hardPersistWatched } = require("%sqstd/globalState.nut")
let { get_setting_by_blk_path, set_setting_by_blk_path, set_setting_by_blk_path_and_save, save_changed_settings } = require("settings")
let { apply_video_settings } = require("videomode")
let { mkCombo } = require("%scripts/ui/widgets/simpleComponents.nut")
let { mkSettingsOption } = require("options_lib.nut")

let presets = ["low", "medium", "ultra"]

let allowCustomSettingsQuery = ecs.SqQuery("allowCustomSettingsQuery", {comps_rw=[["settings_override__useCustomSettings", ecs.TYPE_BOOL], ["settings_override__graphicsSettings", ecs.TYPE_OBJECT]]})
let setCustomSetting = function(blkPath, v) {
  allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__useCustomSettings"]=false)
  defer(function() {
    allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__graphicsSettings"][blkPath] = v)
    defer(@() allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__useCustomSettings"]=true))
  })
}
let getCustomSetting = @(blkPath) allowCustomSettingsQuery.perform(@(_, comp) comp["settings_override__graphicsSettings"][blkPath])

let mapOptionsByPresetTable = {
  ["graphics/shadowsQuality"] = ["low","low", "high"],
  ["graphics/dynamicShadowsQuality"] = ["low", "low", "high"],
  ["graphics/giAlgorithm"] = ["medium","high", "high"],
  ["graphics/giAlgorithmQuality"] = [0.5,0.2,0.75],
  ["graphics/skiesQuality"] = ["medium","medium", "high"],
  ["graphics/aoQuality"] = ["low","medium","high"],
  ["graphics/ssrQuality"] = ["low","medium","high"],
  ["graphics/ssssQuality"] = ["low", "low", "high"],
  ["graphics/anisotropy"] = [4, 8, 16],
  ["graphics/cloudsQuality"] = ["default", "default", "volumetric"],
  ["graphics/volumeFogQuality"] = ["close", "close", "far"],
  ["graphics/fxTarget"] = ["lowres","medres", "highres"],
  ["graphics/groundDisplacementQuality"] = [1, 2, 2],
  ["video/antiAliasingMode"] = [3, 3, 3],
  ["video/temporalUpsamplingRatio"] = [50.0, 100.0, 100.0],
}
let optionsRequireRestart = {
  ["graphics/texquality"] = true
}

let mapOptionsByPreset = {}
foreach (i, presetName in presets){
  let preset = {}
  mapOptionsByPreset[presetName] <- preset
  foreach (opt, perPreset in mapOptionsByPresetTable) {
    let blkPath = type(opt)=="string" ? opt : opt.blkPath
    preset[blkPath] <- perPreset[i]
  }
}

function showMsgboxOfRestart(){
  showWarning("Restart required to apply some changes")
}

function optionCtor(opt) {
  let var = opt?.var ?? Watched(opt.defVal)
  let needRestart = opt?.restart ?? false
  let set = opt?.setValue ?? @(v) var.set(v)
  let setValue = function(v) {
    set(v)
    set_setting_by_blk_path_and_save(opt.blkPath, v)
  if (needRestart)
    showMsgboxOfRestart()
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


const graphicsPresetBlkPath = "graphics/preset"
let graphicsPreset = hardPersistWatched("graphicsPreset", get_setting_by_blk_path(graphicsPresetBlkPath) ?? "ultra")

function mkGraphicsPresetUpdate(checkRestart=true){
  return function graphicsPresetUpdate(presetName) {
    assert(presetName in mapOptionsByPreset, @() $"unknownPreset: '{presetName}', supported = {", ".join(presets)}")
    let selectedPreset = mapOptionsByPreset[presetName]
    let changedSettings = []
    let prevPreset = mapOptionsByPreset?[graphicsPreset.get()] ?? mapOptionsByPreset["ultra"]
    graphicsPreset.set(presetName)
    set_setting_by_blk_path_and_save(graphicsPresetBlkPath, presetName)
    local needRestart = false
    foreach (blkpath, value in selectedPreset) {
      set_setting_by_blk_path(blkpath, value)
      if (prevPreset[blkpath] != value) {
        changedSettings.append(blkpath)
        if (blkpath in optionsRequireRestart)
          needRestart = true
      }
    }
    save_changed_settings(changedSettings)
    apply_video_settings(changedSettings)
    if (checkRestart && needRestart)
      showMsgboxOfRestart()
  }
}
let graphicsPresetUpdate = mkGraphicsPresetUpdate()

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
  graphicsPresetApply = @() mkGraphicsPresetUpdate(false)(graphicsPreset.get())
  optDriver
}