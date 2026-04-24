from "frp" import Watched, Computed, update_deferred
from "dagor.system" import get_arg_value_by_name

// This test triggers a logerr (circular dependency), which makes csq exit
// with code 1. Skip when run outside test runner (e.g. CI syntax checks).
if (get_arg_value_by_name("frp-test") == null)
  return

// Test: Circular dependency detection in pull()
// A and B only capture 'w' as a static source (first-level free variables).
// But at runtime, A's closure calls holder.b.get() and B's calls holder.a.get(),
// creating a cycle in the pull execution path (A pulls B pulls A).
// The isPulling flag detects this runtime cycle even though the static source
// graph is acyclic.

let holder = { a = null, b = null }
let w = Watched(1)

let a = Computed(function() {
  let bval = holder.b?.get() ?? 0
  return w.get() + bval
})

let b = Computed(function() {
  let aval = holder.a?.get() ?? 0
  return w.get() + aval
})

// Wire up the cycle
holder.a = a
holder.b = b

update_deferred()
println($"initial: a={a.get()}, b={b.get()}")

// Trigger an update - would infinite-recurse without cycle detection
w.set(2)
update_deferred()
println($"after: a={a.get()}, b={b.get()}")
println("no crash")
