from "frp" import Watched, Computed, update_deferred

// Test that immediate updates happen synchronously during .set(),
// not deferred to update_deferred().

// --- Test 1: Subscriber fires during set, not at update_deferred ---
let log = []
let w = Watched(1)
w.setDeferred(false)

update_deferred()
w.subscribe(@(v) log.append($"sub:{v}"))

log.append("before-set")
w.set(2)
log.append("after-set")
update_deferred()
log.append("after-update")

println($"timing: {" ".join(log)}")
// If immediate: before-set sub:2 after-set after-update
// If deferred:  before-set after-set sub:2 after-update

// --- Test 2: Computed value available immediately after set ---
let w2 = Watched(10)
w2.setDeferred(false)
let c2 = Computed(@() w2.get() * 2)
c2.setDeferred(false)
update_deferred()

let snapshots = []
snapshots.append(c2.get())  // 20
w2.set(20)
snapshots.append(c2.get())  // 40 immediately, no update_deferred needed
w2.set(30)
snapshots.append(c2.get())  // 60 immediately
println($"snapshots: {" ".join(snapshots)}")

// --- Test 3: Immediate subscriber sees intermediate values ---
let history = []
let w3 = Watched(0)
w3.setDeferred(false)
update_deferred()
w3.subscribe(@(v) history.append(v))

w3.set(1)
w3.set(2)
w3.set(3)
println($"history: {" ".join(history)}")
// Immediate: subscriber called 3 times with 1, 2, 3

// --- Test 4: Deferred subscriber does NOT see intermediate values ---
let dHistory = []
let dw = Watched(0)
// dw is deferred by default
update_deferred()
dw.subscribe(@(v) dHistory.append(v))

dw.set(1)
dw.set(2)
dw.set(3)
println($"deferred before update: {" ".join(dHistory)}")
update_deferred()
println($"deferred after update: {" ".join(dHistory)}")
// Deferred: subscriber called once with final value 3

// --- Test 5: Immediate computed in chain, read between sets ---
let src = Watched("a")
src.setDeferred(false)
let upper = Computed(@() src.get() + src.get())
upper.setDeferred(false)
let triple = Computed(@() upper.get() + src.get())
triple.setDeferred(false)
update_deferred()

println($"chain: {triple.get()}")  // aaa
src.set("b")
println($"chain: {triple.get()}")  // bbb (immediate, no update_deferred)
src.set("c")
println($"chain: {triple.get()}")  // ccc
