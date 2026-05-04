from "frp" import Watched, update_deferred
from "dagor.system" import get_arg_value_by_name

// This test triggers a logerr (circular dependency), which makes csq exit
// with code 1. Skip when run outside test runner (e.g. CI syntax checks).
if (get_arg_value_by_name("frp-test") == null)
  return

let a = Watched(0)
let b = Watched(0)
a.subscribe(@(_v) b.set(a.get() + 1))
b.subscribe(@(_v) a.set(b.get() + 1))

// Kick off the cycle
a.set(1)

// In A<->B oscillation each observable appears every other frame,
// so ~20 frames needed for streak to reach the threshold of 10
for (local i = 0; i < 25; i++)
  update_deferred()
println("done")
