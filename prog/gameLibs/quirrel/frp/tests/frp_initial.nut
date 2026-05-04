from "frp" import Watched, Computed, FRP_INITIAL, update_deferred

// Test 1: Computed with 2-param function receives FRP_INITIAL on first call
let w = Watched(10)

local initialReceived = false
local firstCurVal = null

let c = Computed(function(curVal) {
  if (curVal == FRP_INITIAL) {
    initialReceived = true
    firstCurVal = "FRP_INITIAL"
  }
  return w.get() * 2
})
update_deferred()

println($"initial received: {initialReceived}")
println($"first curVal: {firstCurVal}")
println($"c value: {c.get()}")

// Test 2: On subsequent recalculation, curVal is the actual previous value
local secondCurVal = null
let c2 = Computed(function(curVal) {
  secondCurVal = curVal
  return w.get() + 1
})
update_deferred()
println($"c2 initial: {c2.get()}")

w.set(20)
update_deferred()
// After recalc, curVal should be the OLD value (11), not FRP_INITIAL
println($"c2 after change: {c2.get()}")
println($"second curVal was: {secondCurVal}")
