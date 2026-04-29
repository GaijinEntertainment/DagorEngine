from "frp" import Watched, Computed, update_deferred

// Test 1: Set Watched to same value -> no subscriber call
let w = Watched(42)
local subCount = 0
w.subscribe(@(_v) subCount++)
w.set(42)
update_deferred()
println($"same value sub count: {subCount}")

// Test 2: Set to different then back -> subscriber IS called (value changed between cycles)
w.set(99)
update_deferred()
subCount = 0
w.set(42)
update_deferred()
println($"changed back sub count: {subCount}")

// Test 3: Computed returns same value -> downstream not notified
let a = Watched(5)
let clamped = Computed(@() a.get() > 10 ? 10 : a.get())
update_deferred()

local downstreamCount = 0
clamped.subscribe(@(_v) downstreamCount++)

// Change a but clamped stays the same (both below 10)
a.set(5)
update_deferred()
println($"same computed sub count: {downstreamCount}")

// Change a so clamped changes
a.set(7)
update_deferred()
println($"diff computed sub count: {downstreamCount}")
println($"clamped: {clamped.get()}")

// Change a above 10 -> clamped = 10
a.set(15)
update_deferred()
println($"clamped at max: {clamped.get()}")

// Change a but stay above 10 -> clamped still 10 -> no notification
downstreamCount = 0
a.set(20)
update_deferred()
println($"above max sub count: {downstreamCount}")
