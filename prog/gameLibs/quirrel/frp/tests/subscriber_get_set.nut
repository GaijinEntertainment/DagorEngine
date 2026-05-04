from "frp" import Watched, Computed, update_deferred

// Regression test: subscriber that calls .set() on one observable and .get() on a
// deferred Computed that depends on it, then .set() on another observable.
// This should NOT produce "Triggered while propagating" errors.
//
// Scenario:
//   1. Subscriber on X calls B.set() → markDependentsDirty marks C (Computed) as CHECK
//   2. Same subscriber calls C.get() → ensureUpToDate → pull → C recalculates
//   3. Same subscriber calls D.set() → triggerRoot → notifyQueueCache must be empty
//
// Before the fix, step 2 added C to notifyQueueCache (via pull), and step 3 saw it
// as non-empty → spurious "Triggered while propagating" error.

let source = Watched(1)
let target = Watched(0)
let derived = Computed(@() target.get() * 10)
let result = Watched(0)

// Subscriber: set target, read derived (triggers lazy pull), then set result
source.subscribe(function(_v) {
  target.set(source.get() + 100)
  let d = derived.get()  // lazy pull of deferred Computed during subscriber dispatch
  result.set(d)
})

source.set(5)
update_deferred()
println($"source={source.get()}")
update_deferred()
println($"target={target.get()}")
println($"derived={derived.get()}")
update_deferred()
println($"result={result.get()}")

// Test 2: Multiple subscribers in same batch — one sets, another gets+sets
let w1 = Watched(0)
let w2 = Watched(0)
let comp = Computed(@() w2.get() + 1)
let w3 = Watched(0)

w1.subscribe(@(_v) w2.set(w1.get() * 2))
w1.subscribe(function(_v) {
  let c = comp.get()  // comp depends on w2, which was just set by previous subscriber
  w3.set(c)
})

w1.set(10)
update_deferred()
update_deferred()
update_deferred()
println($"w2={w2.get()}, comp={comp.get()}, w3={w3.get()}")
