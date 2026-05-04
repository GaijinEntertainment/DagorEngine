from "frp" import Watched, Computed, update_deferred, gather_graph_stats
from "debug" import collectgarbage

// Test 1: Watched creation reflected in graph_stats
let w1 = Watched(1)
let w2 = Watched(2)
let w3 = Watched(3)
update_deferred()

let s1 = gather_graph_stats()
println($"watched after create: {s1.watchedTotal}")

// Test 2: Computed creation reflected in graph_stats
let c1 = Computed(@() w1.get() + w2.get())
let c2 = Computed(@() w2.get() + w3.get())
update_deferred()

let s2 = gather_graph_stats()
println($"computed after create: {s2.computedTotal}")

// Test 3: GC of watched nodes decreases count
{
  let _tmp1 = Watched(100) //-declared-never-used
  let _tmp2 = Watched(200) //-declared-never-used
  update_deferred()
  let s = gather_graph_stats()
  println($"watched with temps: {s.watchedTotal}")
}
collectgarbage()
update_deferred()

let s3 = gather_graph_stats()
println($"watched after GC: {s3.watchedTotal}")

// Test 4: GC of computed nodes decreases count
{
  let _tmpC = Computed(@() w1.get() * 10) //-declared-never-used
  update_deferred()
  let s = gather_graph_stats()
  println($"computed with temp: {s.computedTotal}")
}
collectgarbage()
update_deferred()

let s4 = gather_graph_stats()
println($"computed after GC: {s4.computedTotal}")

// Test 5: Slot reuse -- new nodes work correctly after GC frees slots
{
  for (local i = 0; i < 10; i++) {
    let _w = Watched(i) //-declared-never-used
  }
}
collectgarbage()
update_deferred()

// Now create new nodes in the (potentially reused) slots
let w4 = Watched(40)
let c3 = Computed(@() w4.get() + 2)
update_deferred()
println($"reused slot watched: {w4.get()}")
println($"reused slot computed: {c3.get()}")

// Verify they propagate correctly
w4.set(50)
update_deferred()
println($"reused slot after set: {c3.get()}")

// Test 6: Interleaved create/destroy cycles
for (local cycle = 0; cycle < 3; cycle++) {
  let _ws = array(5).map(@(_, i) Watched(i)) //-declared-never-used
  let _cs = array(3).map(@(_, i) Computed(@() w1.get() + i)) //-declared-never-used
  update_deferred()
  collectgarbage()
  update_deferred()
}
let s6 = gather_graph_stats()
println($"after cycles watched: {s6.watchedTotal}")
println($"after cycles computed: {s6.computedTotal}")

// Test 7: Values survive GC of unrelated nodes
println($"w1 survived: {w1.get()}")
println($"c1 survived: {c1.get()}")
println($"c2 survived: {c2.get()}")
