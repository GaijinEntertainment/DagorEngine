from "frp" import Watched, Computed, update_deferred, gather_graph_stats
from "debug" import collectgarbage

// Test 1: GC'd computed detaches from source -- source still works
let src = Watched(1)
{
  let _c = Computed(@() src.get() * 10) //-declared-never-used
  update_deferred()
}
collectgarbage()
update_deferred()

// Source should still be settable without issues
src.set(2)
update_deferred()
println($"src after computed GC: {src.get()}")

// Test 2: Surviving computed still works after sibling computed is GC'd
let baseW = Watched(5)
let survivor = Computed(@() baseW.get() + 1)
{
  let _sibling = Computed(@() baseW.get() + 2) //-declared-never-used
  update_deferred()
}
collectgarbage()
update_deferred()

baseW.set(10)
update_deferred()
println($"survivor after sibling GC: {survivor.get()}")

// Test 3: Chain where middle node is GC'd
let root = Watched(1)
local leafVal = -1
{
  let middle = Computed(@() root.get() * 2)
  let leaf = Computed(@() middle.get() + 100)
  update_deferred()
  leafVal = leaf.get()
}
println($"leaf before GC: {leafVal}")
collectgarbage()
update_deferred()

// Root should still work, chain is gone
root.set(5)
update_deferred()
println($"root after chain GC: {root.get()}")

// Test 4: Subscriber on a watched, then GC other nodes -- subscriber still fires
let w = Watched(0)
local subCount = 0
w.subscribe(@(_v) subCount++)

{
  let _noise = array(5).map(@(_, i) Watched(i * 100)) //-declared-never-used
  update_deferred()
}
collectgarbage()
update_deferred()

w.set(1)
update_deferred()
println($"sub after GC noise: {subCount}")

w.set(2)
update_deferred()
println($"sub after second set: {subCount}")

// Test 5: Trigger pattern (like ScrollHandler) -- watched with null, trigger fires subscriber
let trigger_w = Watched(null)
local triggerFired = 0
trigger_w.subscribe(@(_v) triggerFired++)

trigger_w.trigger()
update_deferred()
println($"trigger fired: {triggerFired}")

// Trigger after GC of unrelated nodes
{
  let _tmp = Watched("discard") //-declared-never-used
  update_deferred()
}
collectgarbage()
update_deferred()

trigger_w.trigger()
update_deferred()
println($"trigger after GC: {triggerFired}")

// Test 6: Graph stats are consistent after mixed lifecycle
let s = gather_graph_stats()
// We should have: src, baseW, w, trigger_w, root = 5 watched; survivor = 1 computed
println($"final watched: {s.watchedTotal}")
println($"final computed: {s.computedTotal}")
