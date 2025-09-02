from "%darg/ui_imports.nut" import *
let {scan_folder} = require("dagor.fs")
let all = scan_folder({root="samples_prog", file_suffix = ".nut"})
let {normal} = require("_cursors.nut")

let not_working = ["das_canvas.ui.nut", "das_radar.ui.nut"]

let is_ui = function(v){
  if (v.indexof("all_ui.ui.nut")!=null)
    return false
  if (!v.endswith(".ui.nut"))
    return false
  if (v.indexof("zbugs/")!=null)
    return false
  foreach(f in not_working)
    if (v.endswith(f))
      return false
  return true
}
return @() {
  size = flex()
  cursor = normal
  children = all.filter(is_ui).map(@(v) require(v))
}
