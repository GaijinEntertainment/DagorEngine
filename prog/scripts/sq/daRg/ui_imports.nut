import "%sqstd/log.nut" as logLib
from "%sqstd/string.nut" import tostring_r
from "%sqstd/frp.nut" import Watched, Computed, ComputedImmediate, FRP_INITIAL, make_all_observables_immutable, isObservable, isComputed

let log = logLib([
  {
    compare = @(val) isObservable(val)
    tostring = @(val) "Watched: {0}".subst(tostring_r(val.get(),{maxdeeplevel = 3, splitlines=false}))
  }
  {
    compare = @(val) isComputed(val)
    tostring = @(val) "Computed: {0}".subst(tostring_r(val.get(),{maxdeeplevel = 3, splitlines=false}))
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
  wdlog = log.wdlog //disable: -dlog-warn
}

let frpExtras = {
  Watched
  Computed
  ComputedImmediate
  isComputed
  isObservable
  FRP_INITIAL
  make_all_observables_immutable
}

return freeze(require("daRg").__merge(
  require("darg_library.nut")
  require("%sqstd/functools.nut")
  logs
  frpExtras
  {Behaviors = require("daRg.behaviors")}
))
