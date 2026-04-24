from "frp" import Watched, Computed, update_deferred, gather_graph_stats

// Create a known set of observables
let w1 = Watched(1)
let w2 = Watched(2)
let w3 = Watched(3)
let c1 = Computed(@() w1.get() + w2.get())
let c2 = Computed(@() w2.get() + w3.get())
let c3 = Computed(@() c1.get() + c2.get())
update_deferred()

let stats = gather_graph_stats()
println($"watchedTotal: {stats.watchedTotal}")
println($"computedTotal: {stats.computedTotal}")

// Test 2: Subscribe to some -- check unsubscribed counts
local dummy = null //-declared-never-used
c3.subscribe(@(v) dummy = v)
update_deferred()

let stats2 = gather_graph_stats()
println($"computedTotal2: {stats2.computedTotal}")

// Verify values are correct
println($"c3 = {c3.get()}")
