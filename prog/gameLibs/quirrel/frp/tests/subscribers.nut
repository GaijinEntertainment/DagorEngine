from "frp" import Watched, Computed, update_deferred

// Test 1: Basic subscribe -- receives new value
let w = Watched(1)
local lastVal = null
w.subscribe(function(v) {
  lastVal = v
})
w.set(42)
update_deferred()
println($"sub received: {lastVal}")

// Test 2: Multiple subscribers all called
let w2 = Watched(0)
local val1 = null
local val2 = null
local val3 = null
w2.subscribe(@(v) val1 = v)
w2.subscribe(@(v) val2 = v)
w2.subscribe(@(v) val3 = v)
w2.set(7)
update_deferred()
println($"multi sub: {val1}, {val2}, {val3}")

// Test 3: Unsubscribe removes subscriber
let w3 = Watched(0)
local unsub_val = null
let handler = function(v) { unsub_val = v }
w3.subscribe(handler)
w3.set(1)
update_deferred()
println($"before unsub: {unsub_val}")

w3.unsubscribe(handler)
w3.set(2)
update_deferred()
println($"after unsub: {unsub_val}")

// Test 4: Computed subscriber
let src = Watched(5)
let comp = Computed(@() src.get() * 3)
local compSubVal = null
comp.subscribe(@(v) compSubVal = v)
src.set(10)
update_deferred()
println($"computed sub: {compSubVal}")

// Test 5: Subscriber call order (same as subscribe order)
let w4 = Watched(0)
let order = []
w4.subscribe(@(_v) order.append(1))
w4.subscribe(@(_v) order.append(2))
w4.subscribe(@(_v) order.append(3))
w4.set(1)
update_deferred()
println($"order: {", ".join(order.map(@(v) v.tostring()))}")
