from "%sqstd/frp.nut" import Watched, Computed, ComputedImmediate, FRP_INITIAL, FRP_DONT_CHECK_NESTED, set_nested_observable_debug, make_all_observables_immutable, isObservable, isComputed

let {tostring_r} = require("%sqstd/string.nut")
let logLib = require("%sqstd/log.nut")

let log = logLib([
  {
    compare = @(val) isObservable(val)
    tostring = @(val) "Watched: {0}".subst(tostring_r(val.value,{maxdeeplevel = 3, splitlines=false}))
  }
  {
    compare = @(val) isComputed(val)
    tostring = @(val) "Computed: {0}".subst(tostring_r(val.value,{maxdeeplevel = 3, splitlines=false}))
  }
  {
    compare = @(val) type(val)=="instance" && "formatAsString" in val
    tostring = @(val) val.formatAsString()
  }
])

let logs = {
  dlog = log.dlog //warning disable: -dlog-warn
  log = log.log
  log_for_user = log.dlog //warning disable: -dlog-warn
  dlogsplit = log.dlogsplit //warning disable: -dlog-warn
  vlog = log.vlog
  console_print = log.console_print
  logerr = log.logerr
  wlog = log.wlog
  wdlog = @(watched, prefix = null, transform=null) log.wlog(watched, prefix, transform, log.dlog) //disable: -dlog-warn
}

let frpExtras = {
  Watched
  Computed
  ComputedImmediate
  isComputed
  isObservable
  FRP_INITIAL
  FRP_DONT_CHECK_NESTED
  set_nested_observable_debug
  make_all_observables_immutable
}

return require("daRg").__merge(
  require("darg_library.nut")
  require("%sqstd/functools.nut")
  logs
  frpExtras
  {Behaviors = require("daRg.behaviors")}
)
