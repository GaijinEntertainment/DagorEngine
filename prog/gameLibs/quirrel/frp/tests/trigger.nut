from "frp" import Watched, update_deferred
let {fmtVal} = require("helpers.nut")

// Test 1: trigger forces notification without value change
let w = Watched(42)
local triggerCount = 0
local triggerVal = null
w.subscribe(function(v) {
  triggerCount++
  triggerVal = v
})

w.trigger()
update_deferred()
println($"trigger count: {triggerCount}")
println($"trigger val: {triggerVal}")

// Test 2: trigger on mutable container (common use case)
let arr = Watched([1, 2, 3])
local arrSubCount = 0
arr.subscribe(@(_v) arrSubCount++)

// Direct modification + trigger (bypass set)
arr.get().append(4)
arr.trigger()
update_deferred()
println($"arr trigger count: {arrSubCount}")
println($"arr after trigger: {fmtVal(arr.get())}")

// Test 3: Multiple triggers -> subscriber called
triggerCount = 0
w.trigger()
w.trigger()
update_deferred()
println($"multi trigger count: {triggerCount}")
