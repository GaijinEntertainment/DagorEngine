from "frp" import Watched, Computed, update_deferred

// Test 1: Immediate Watched -> Computed updates immediately
let w = Watched(1)
w.setDeferred(false)
let c = Computed(@() w.get() * 2)
c.setDeferred(false)
update_deferred()
println($"initial: {c.get()}")

w.set(5)
println($"after immediate set: {c.get()}")

// Test 2: Subscriber called synchronously during set
local immSubVal = null
local immSubCount = 0
w.subscribe(function(v) {
  immSubVal = v
  immSubCount++
})
w.set(10)
println($"imm sub value: {immSubVal}")
println($"imm sub count: {immSubCount}")

// Test 3: Chained immediate
let x = Watched(1)
x.setDeferred(false)
let step1 = Computed(@() x.get() + 1)
step1.setDeferred(false)
let step2 = Computed(@() step1.get() * 2)
step2.setDeferred(false)
update_deferred()
println($"chain initial: {step2.get()}")

x.set(4)
println($"chain after x=4: {step2.get()}")

// Test 4: Mix immediate watched with deferred computed
let iw = Watched(1)
iw.setDeferred(false)
let dc = Computed(@() iw.get() * 3)
// dc is deferred by default
update_deferred()
println($"mix initial: {dc.get()}")

iw.set(2)
update_deferred()
println($"mix after set+update: {dc.get()}")
