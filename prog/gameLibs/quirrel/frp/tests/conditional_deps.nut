from "frp" import Watched, Computed, update_deferred

// Pattern from active_matter: Computed reads different Watched based on a condition
// This tests that dependency tracking works when the set of sources varies per evaluation

let mode = Watched("a")
let srcA = Watched(10)
let srcB = Watched(20)

let result = Computed(function() {
  if (mode.get() == "a")
    return srcA.get()
  return srcB.get()
})
update_deferred()
println($"mode=a: {result.get()}")

// Change the source that IS being read
srcA.set(11)
update_deferred()
println($"srcA changed: {result.get()}")

// Change the source that is NOT being read — should not affect result
srcB.set(99)
update_deferred()
println($"srcB changed (unused): {result.get()}")

// Switch mode — now reads from srcB
mode.set("b")
update_deferred()
println($"mode=b: {result.get()}")

// Now srcA changes shouldn't propagate, srcB should
srcA.set(999)
update_deferred()
println($"srcA changed (now unused): {result.get()}")

srcB.set(30)
update_deferred()
println($"srcB changed (active): {result.get()}")

// Test 2: Multiple conditional branches
let selector = Watched(0)
let vals = [Watched(100), Watched(200), Watched(300)]
let selected = Computed(@() vals[selector.get()].get())
update_deferred()
println($"sel[0]: {selected.get()}")

selector.set(2)
update_deferred()
println($"sel[2]: {selected.get()}")

vals[2].set(333)
update_deferred()
println($"val[2] changed: {selected.get()}")
