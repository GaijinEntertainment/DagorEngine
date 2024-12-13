from "%scripts/ui/ui_library.nut" import *

let {get_setting_by_blk_path, set_setting_by_blk_path_and_save} = require("settings")

let mkOptText = @(text) {rendObj = ROBJ_TEXT text size = [flex(1), SIZE_TO_CONTENT] halign = ALIGN_RIGHT fontSize = hdpx(25)}

function mkSaveData(saveId, defValueFunc = @() null, validateFunc = @(v) v, setFunc = null) {
  let def = defValueFunc()
  local v = get_setting_by_blk_path(saveId) ?? def
  let var = Watched(v)
  function setValue(value) {
    v = validateFunc(value)
    set_setting_by_blk_path_and_save(saveId, v)
    var.set(v)
    setFunc?(v)
  }
  return {
    var
    setValue
  }
}


function mkSettingsOption(title, widget){
  return {
    flow = FLOW_HORIZONTAL
    children = [mkOptText(title), {size= [flex(2), flex()] children = widget}],
    size = [flex(), hdpx(35)] gap = hdpx(20)
  }
}
return {
  mkSaveData
  mkSettingsOption
  mkOptText
}