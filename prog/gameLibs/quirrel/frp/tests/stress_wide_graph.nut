from "frp" import Watched, Computed, update_deferred

// Test: 1 Watched -> 100 Computed dependents
let src = Watched(1)
let computeds = []

for (local i = 0; i < 100; i++) {
  let idx = i
  computeds.append(Computed(@() src.get() + idx))
}
update_deferred()

// Verify initial values
let allCorrectInitial = computeds.reduce(function(ok, c, i) {
  return ok && c.get() == (1 + i)
}, true)
println($"initial correct: {allCorrectInitial}")
println($"c[0]={computeds[0].get()}, c[99]={computeds[99].get()}")

// Change source
src.set(1000)
update_deferred()

let allCorrectAfter = computeds.reduce(function(ok, c, i) {
  return ok && c.get() == (1000 + i)
}, true)
println($"after change correct: {allCorrectAfter}")
println($"c[0]={computeds[0].get()}, c[99]={computeds[99].get()}")

// Test 2: Subscriber on all 100
local subCount = 0
foreach (c in computeds)
  c.subscribe(@(_v) subCount++)

src.set(2000)
update_deferred()
println($"subscriber count: {subCount}")
