// Squirrel module under test: frp (registered by frp.cpp)

from "frp" import Watched, Computed, update_deferred, gather_graph_stats


// ============================================================
// Section 1: Watched constructor and get/set round-trip
// ============================================================
println("--- Section 1: Watched ctor + get/set ---")

let wInt = Watched(42)
assert(wInt.get() == 42, $"initial Watched int: {wInt.get()}")

wInt.set(100)
assert(wInt.get() == 100, $"Watched after set: {wInt.get()}")

let wDefault = Watched()
assert(wDefault.get() == null, "Watched() default value is null")

let wStr = Watched("hello")
assert(wStr.get() == "hello", "Watched string init")
wStr.set("world")
assert(wStr.get() == "world", "Watched string after set")

let wTbl = Watched({k = 1})
assert(wTbl.get().k == 1, "Watched table content")

println("Section 1 PASSED")


// ============================================================
// Section 2: Computed reacts to Watched updates
// ============================================================
println("--- Section 2: Computed reacts to Watched ---")

let a = Watched(5)
let doubled = Computed(@() a.get() * 2)
assert(doubled.get() == 10, $"doubled initial: {doubled.get()}")

a.set(7)
assert(doubled.get() == 14, $"doubled after a=7: {doubled.get()}")

let b = Watched(3)
let sum = Computed(@() a.get() + b.get())
assert(sum.get() == 10, $"sum initial: {sum.get()}")

b.set(8)
assert(sum.get() == 15, $"sum after b=8: {sum.get()}")

println("Section 2 PASSED")


// ============================================================
// Section 3: subscribe / trigger / unsubscribe
// ============================================================
println("--- Section 3: subscribe / trigger / unsubscribe ---")

let w = Watched(1)
local notifyCount = 0
local lastVal = null
let cb = function(v) {
  notifyCount++
  lastVal = v
}
w.subscribe(cb)

w.set(10)
update_deferred()
assert(notifyCount == 1, $"subscriber called once after set: {notifyCount}")
assert(lastVal == 10, $"subscriber received value: {lastVal}")

w.trigger()
update_deferred()
assert(notifyCount == 2, $"subscriber called after trigger: {notifyCount}")

w.unsubscribe(cb)
w.set(99)
assert(notifyCount == 2, $"unsubscribe stops further notifications: {notifyCount}")

println("Section 3 PASSED")


// ============================================================
// Section 4: gather_graph_stats reports counts
// ============================================================
println("--- Section 4: gather_graph_stats ---")

let stats = gather_graph_stats()
assert(typeof stats == "table", "gather_graph_stats returns table")
assert("watchedTotal" in stats, "stats has watchedTotal")
assert("computedTotal" in stats, "stats has computedTotal")
assert(stats.watchedTotal >= 5, $"watchedTotal counted: {stats.watchedTotal}")
assert(stats.computedTotal >= 2, $"computedTotal counted: {stats.computedTotal}")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

local threw = false
try {
  let c = Computed(@() a.get())
  c.set(1)
} catch (e) {
  threw = true
}
assert(threw, "Computed.set must throw")

threw = false
try {
  let c = Computed(@() a.get())
  c.trigger()
} catch (e) {
  threw = true
}
assert(threw, "Computed.trigger must throw")

threw = false
try {
  w.subscribe("not a function")
} catch (e) {
  threw = true
}
assert(threw, "subscribe with non-closure must throw")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
