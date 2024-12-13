from "%scripts/ui/ui_library.nut" import *

let {sliderWithText} = require("%scripts/ui/widgets/slider.nut")
let {round_by_value} = require("%sqstd/math.nut")
let {sound_set_volume = @(...) null} = require_optional("sound")
let {mkSaveData, mkSettingsOption} = require("options_lib.nut")

let mkRounded = @(val) round_by_value(val, 0.01)
let clamp_0_1 = @(v) v<0.0 ? 0.0 : (v>1.0 ? 1.0 : v)

let mkSettingsOpt = kwarg(function(blk_path, name, widgetCtor, defValue=null, defFunc=null, validateFunc=null, morphFunc=null, setFunc = null){
  let opt = mkSaveData(blk_path, defFunc ?? @() defValue, validateFunc, setFunc).__update({morphText = morphFunc ?? mkRounded})
  let group = ElemGroup()
  let xmbNode = XmbNode()
  let widget = widgetCtor(opt, group, xmbNode)
  return mkSettingsOption(name, widget)
})

let audio_options = [
  { blk_path = "sound/volume/master", name = "Master Sound Volume", widgetCtor = sliderWithText, defValue = 1.0, validateFunc = clamp_0_1, setFunc = @(v) sound_set_volume("master", v)}
//  { blk_path = "sound/volume/ui", name = "UI Sound Volume", widgetCtor = sliderWithText, defValue = 1.0, validateFunc = clamp_0_1, setFunc = @(v) sound_set_volume("ui", v)}
//  { blk_path = "sound/volume/sfx", name = "SFX Sound Volume", widgetCtor = sliderWithText, defValue = 1.0, validateFunc = clamp_0_1, setFunc = @(v) sound_set_volume("sfx", v)}
//  { blk_path = "sound/volume/music", name = "Music Volume", widgetCtor = sliderWithText, defValue = 1.0, validateFunc = clamp_0_1, setFunc = @(v) sound_set_volume("music", v)}
].map(mkSettingsOpt)

return {
  audio_options
  mkSaveData
}