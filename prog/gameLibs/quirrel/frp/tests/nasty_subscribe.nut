from "frp" import Watched, Computed, update_deferred

// Test 1: Subscriber mutates another Watched during notification
let w1 = Watched(0)
let w2 = Watched(0)
let c2 = Computed(@() w2.get() * 10)

w1.subscribe_with_nasty_disregard_of_frp_update(function(_v) {
  w2.set(w1.get() + 100)
})

w1.set(5)
update_deferred()
println($"w1={w1.get()}")
update_deferred()
println($"w2={w2.get()}")
println($"c2={c2.get()}")

// Test 2: Cascading -- subscriber sets a value that triggers another subscriber
let a = Watched(0)
let b = Watched(0)
let c = Watched(0)

a.subscribe_with_nasty_disregard_of_frp_update(@(_v) b.set(a.get() * 2))
b.subscribe_with_nasty_disregard_of_frp_update(@(_v) c.set(b.get() + 1))

a.set(3)
update_deferred()
update_deferred()
update_deferred()
println($"cascade: a={a.get()}, b={b.get()}, c={c.get()}")
