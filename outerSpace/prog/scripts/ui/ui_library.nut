from "math" import min, max, clamp
from "%sqstd/frp.nut" import WatchedRo

let { loc } = require("%dngscripts/localizations.nut")
let { register_command, command } = require("console")
let { defer } = require("dagor.workcycle")

global enum Layers {
  Default
  ComboPopup
  MsgBox
  Tooltip
  Inspector
}

let export = {
  loc
  console_register_command = register_command
  console_command = command
  defer
}

let log= require("%sqstd/log.nut")()
let logs = {
  log_for_user = log.dlog //warning disable: -dlog-warn
  dlog = log.dlog //warning disable: -dlog-warn
  log = log.log
  with_prefix = log.with_prefix
  dlogsplit = log.dlogsplit
  vlog = log.vlog
  console_print = log.console_print
  wlog = log.wlog
  wdlog = @(watched, prefix = null, transform=null) log.wlog(watched, prefix, transform, log.dlog) //disable: -dlog-warn
  debugTableData = log.debugTableData
}

return export.__update(
  {min, max, clamp, WatchedRo},
  require("daRg"),
  require("frp"),
  logs,
  require("%darg/darg_library.nut"),
  require("%sqstd/functools.nut")
  {DngBhv = require("dng.behaviors")}
)
