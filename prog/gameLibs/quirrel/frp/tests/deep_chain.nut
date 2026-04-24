from "frp" import Watched, Computed, update_deferred

// Test: Chain of 20 Computed values
// Each adds 1 to the previous
let root = Watched(0)

let c1 = Computed(@() root.get() + 1)
let c2 = Computed(@() c1.get() + 1)
let c3 = Computed(@() c2.get() + 1)
let c4 = Computed(@() c3.get() + 1)
let c5 = Computed(@() c4.get() + 1)
let c6 = Computed(@() c5.get() + 1)
let c7 = Computed(@() c6.get() + 1)
let c8 = Computed(@() c7.get() + 1)
let c9 = Computed(@() c8.get() + 1)
let c10 = Computed(@() c9.get() + 1)
let c11 = Computed(@() c10.get() + 1)
let c12 = Computed(@() c11.get() + 1)
let c13 = Computed(@() c12.get() + 1)
let c14 = Computed(@() c13.get() + 1)
let c15 = Computed(@() c14.get() + 1)
let c16 = Computed(@() c15.get() + 1)
let c17 = Computed(@() c16.get() + 1)
let c18 = Computed(@() c17.get() + 1)
let c19 = Computed(@() c18.get() + 1)
let c20 = Computed(@() c19.get() + 1)

update_deferred()
println($"chain initial: c20={c20.get()}")

root.set(100)
update_deferred()
println($"chain after root=100: c20={c20.get()}")

root.set(0)
update_deferred()
println($"chain after root=0: c20={c20.get()}")

// Test 2: Subscriber at end of chain
local endVal = null
c20.subscribe(@(v) endVal = v)
root.set(50)
update_deferred()
println($"end subscriber: {endVal}")
