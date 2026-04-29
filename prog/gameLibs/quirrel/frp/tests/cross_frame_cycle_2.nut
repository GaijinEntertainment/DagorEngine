from "frp" import Watched, update_deferred

// One-directional subscriber chain: external sets A every frame, A's subscriber
// sets B. This is NOT a cycle and must NOT trigger the cross-frame cycle warning.

let a = Watched(0)
let b = Watched(0)
a.subscribe(@(_v) b.set(a.get() + 1))

for (local i = 0; i < 25; i++) {
  a.set(i)
  update_deferred()
}

println($"b={b.get()}")
println("done")
