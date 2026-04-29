from "%sq/std/frp.nut" import WatchedImmediate, ComputedImmediate, isWatched, isComputed, isObservable, Watched, Computed
from "frp" import update_deferred

// Test 1: WatchedImmediate creates non-deferred Watched
let wi = WatchedImmediate(10)
println($"wi initial: {wi.get()}")
println($"wi deferred: {wi.deferred}")

// Test 2: ComputedImmediate creates non-deferred Computed
let ci = ComputedImmediate(@() wi.get() * 2)
println($"ci initial: {ci.get()}")
println($"ci deferred: {ci.deferred}")

// Immediate updates
wi.set(5)
println($"ci after immediate set: {ci.get()}")

// Test 3: Type checks
let w = Watched(1)
let c = Computed(@() w.get())
update_deferred()

println($"isWatched(w): {isWatched(w)}")
println($"isWatched(c): {isWatched(c)}")
println($"isComputed(w): {isComputed(w)}")
println($"isComputed(c): {isComputed(c)}")
println($"isObservable(w): {isObservable(w)}")
println($"isObservable(c): {isObservable(c)}")
println($"isObservable(42): {isObservable(42)}")
println($"isObservable(null): {isObservable(null)}")
