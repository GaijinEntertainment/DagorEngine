from "frp" import Watched, Computed, FRP_INITIAL, update_deferred

// Test 1: Computed(fn) where fn has 2 params -- receives current value
let w = Watched(1)

let accumulated = Computed(function(curVal) {
  if (curVal == FRP_INITIAL)
    return w.get()
  return curVal + w.get()
})
update_deferred()
println($"initial: {accumulated.get()}")

w.set(2)
update_deferred()
println($"after w=2: {accumulated.get()}")

w.set(3)
update_deferred()
println($"after w=3: {accumulated.get()}")

// Test 2: Multiple sources with curVal
let a = Watched(10)
let b = Watched(20)
let combined = Computed(function(curVal) {
  let sum = a.get() + b.get()
  if (curVal == FRP_INITIAL)
    return sum
  return sum
})
update_deferred()
println($"combined: {combined.get()}")

a.set(100)
update_deferred()
println($"combined after a=100: {combined.get()}")
