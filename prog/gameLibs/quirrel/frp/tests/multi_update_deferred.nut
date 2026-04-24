from "frp" import Watched, Computed, update_deferred

// Test 1: Subscriber in cycle 1 sets a new Watched value
// -> it propagates in cycle 2
let trigger = Watched(0)
let result = Watched(0)
let resultComp = Computed(@() result.get() * 2)

trigger.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  result.set(v + 1000)
})

trigger.set(5)

// First update_deferred: trigger's subscriber runs, sets result
update_deferred()
println($"cycle1 trigger: {trigger.get()}")
println($"cycle1 result: {result.get()}")

// Second update_deferred: result change propagates to resultComp
update_deferred()
println($"cycle2 resultComp: {resultComp.get()}")

// Test 2: Multiple deferred cycles with computed chain
let src = Watched(1)
let mid = Watched(0)
let chain = Computed(@() mid.get() + src.get())

src.subscribe_with_nasty_disregard_of_frp_update(@(v) mid.set(v * 10))

src.set(3)
update_deferred()
println($"src={src.get()}, mid={mid.get()}")
update_deferred()
println($"chain={chain.get()}")
