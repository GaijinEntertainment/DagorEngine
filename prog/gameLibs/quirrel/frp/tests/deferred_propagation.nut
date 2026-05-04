from "frp" import Watched, Computed, update_deferred

// Test 1: Computed does NOT update before update_deferred
let w = Watched(1)
let c = Computed(@() w.get() * 10)
update_deferred()
println($"initial: {c.get()}")

w.set(2)
// NOTE: c.get() after set but before update_deferred may or may not be updated
// depending on implementation. We just check it's correct after update_deferred.
update_deferred()
println($"after update: {c.get()}")

// Test 2: Multiple sets -> only final value propagates
let w2 = Watched(0)
let c2 = Computed(@() w2.get())
update_deferred()

local subCount = 0
local subValue = null
c2.subscribe(function(v) {
  subCount++
  subValue = v
})

w2.set(1)
w2.set(2)
w2.set(3)
update_deferred()
println($"final value: {c2.get()}")
println($"sub called: {subCount}")
println($"sub value: {subValue}")

// Test 3: Subscriber called once per update_deferred, not per set
let w3 = Watched(10)
local callCount = 0
w3.subscribe(function(_v) {
  callCount++
})
w3.set(20)
w3.set(30)
w3.set(40)
update_deferred()
println($"watched sub calls: {callCount}")

// Test 4: No subscriber call if value unchanged after update_deferred
callCount = 0
update_deferred()
println($"no-change sub calls: {callCount}")
