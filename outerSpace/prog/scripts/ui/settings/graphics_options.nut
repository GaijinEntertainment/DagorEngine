from "%scripts/ui/ui_library.nut" import *

let { showWarning } = require("%scripts/ui/widgets/msgbox.nut")
let { hardPersistWatched } = require("%sqstd/globalState.nut")
let { get_setting_by_blk_path, set_setting_by_blk_path, set_setting_by_blk_path_and_save, save_changed_settings } = require("settings")
let { apply_video_settings } = require("videomode")
let { mkCombo } = require("%scripts/ui/widgets/simpleComponents.nut")
let { mkSettingsOption } = require("options_lib.nut")

let presets = ["bareMinimum", "minimum", "low", "medium", "high", "ultra"]

let mapOptionsByPresetTable = {
  ["graphics/texquality"]                     = ["low",         "low",     "medium", "high",    "high",     "high"],
  ["graphics/giAlgorithm"]                    = ["low",         "low",     "medium", "medium",  "high",     "high"],
  ["graphics/giAlgorithmQuality"]             = [0.0,            0.5,       0.5,      1.0,      0.5,        0.8],
  ["graphics/anisotropy"]                     = [1,             1,         2,        4,         8,          16],
  ["graphics/skiesQuality"]                   = ["low",         "low",     "low",    "medium",  "high",     "high"],
//  ["graphics/aoQuality"]                      = ["low",         "low",     "low",    "medium",  "high",     "high"],
  ["graphics/shadowsQuality"]                 = ["low",         "low",     "low",    "low",     "medium",   "high"],
  ["graphics/effectsShadows"]                 = [false,         false,     false,    false,     true,       true],
  ["graphics/cloudsQuality"]                  = ["default",     "default", "default","default", "highres",  "volumetric"],
  ["graphics/volumeFogQuality"]               = ["close",       "close",   "close",  "close",   "close",    "far"],
  ["graphics/waterQuality"]                   = ["low",         "low",     "low",    "low",     "medium",   "high"],
  ["graphics/groundDisplacementQuality"]      = [0,             0,         0,        1,         1,          2],
  ["graphics/groundDeformations"]             = ["off",         "off",     "low",    "medium",  "high",     "high"],
  ["graphics/fxTarget"]                       = ["lowres",      "lowres",  "lowres", "medres",  "medres",   "highres"],
  ["graphics/screenSpaceWeatherEffects"]      = [false,         false,     false,    true,      true,       true],
  ["graphics/ssrQuality"]                     = ["low",         "low",     "low",    "low",     "medium",   "high"],
  ["graphics/scopeImageQuality"]              = [0,             0,         0,        1,         2,          3],
  ["graphics/fftWaterQuality"]                = ["low",         "low",     "medium", "high",    "ultra",    "ultra"],
  ["graphics/HQProbeReflections"]             = [false,         false,     false,    true,      true,       true],
  ["graphics/HQVolumetricClouds"]             = [false,         false,     false,    false,     false,      false], // disabled everywhere by default, way too expensive
  ["graphics/HQVolfog"]                       = [false,         false,     false,    false,     false,      false], // disabled everywhere by default, way too expensive
  ["graphics/ssssQuality"]                    = ["off",         "off",     "off",    "low",     "high",     "high"],
  ["graphics/rendinstTesselation"]            = [false,         false,     false,    false,     false,      false], // disabled everywhere by default, way too expensive
  ["graphics/sharpening"]                     = [0.0,           0.0,       0.0,      0.0,       0.0,        0.0],
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


const graphicsPresetBlkPath = "graphics/preset"
let graphicsPreset = hardPersistWatched("graphicsPreset", get_setting_by_blk_path(graphicsPresetBlkPath) ?? "high")

function showMsgboxOfRestart(){
  showWarning("Restart required to apply some changes")
}

function mkGraphicsPresetUpdate(checkRestart=true){
  return function graphicsPresetUpdate(presetName) {
    assert(presetName in mapOptionsByPreset, @() $"unknownPreset: '{presetName}', supported = {", ".join(presets)}")
    let selectedPreset = mapOptionsByPreset[presetName]
    let changedSettings = []
    let prevPreset = mapOptionsByPreset[graphicsPreset.get()]
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

let optPreset = optionCtor({
  name = "Quality Preset"
  var = graphicsPreset
  blkPath = graphicsPresetBlkPath
  widgetCtor = mkCombo
  defVal = "high"
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
let graphicsOptions = [optPreset, optDriver]
return {
  graphicsOptions
  graphicsPresetUpdate
  graphicsPresetApply = @() mkGraphicsPresetUpdate(false)(graphicsPreset.get())
}