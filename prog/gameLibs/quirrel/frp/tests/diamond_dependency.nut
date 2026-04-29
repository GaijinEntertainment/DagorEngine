from "frp" import Watched, Computed, update_deferred

//     A (Watched)
//    / \
//   B   C (Computed)
//    \ /
//     D (Computed)

let a = Watched(1)
let b = Computed(@() a.get() * 2)
let c = Computed(@() a.get() + 10)
let d = Computed(@() b.get() + c.get())
update_deferred()

println($"a={a.get()}, b={b.get()}, c={c.get()}, d={d.get()}")

// Test 1: D sees consistent values after A changes
a.set(5)
update_deferred()
println($"a={a.get()}, b={b.get()}, c={c.get()}, d={d.get()}")

// Test 2: D subscriber called once, not twice
local dSubCount = 0
local dSubVal = null
d.subscribe(function(v) {
  dSubCount++
  dSubVal = v
})
a.set(10)
update_deferred()
println($"d sub count: {dSubCount}")
println($"d sub val: {dSubVal}")
println($"d={d.get()}")

// Test 3: Wider diamond -- multiple intermediate nodes
//     W
//   / | \
//  X  Y  Z
//   \ | /
//     R
let w = Watched(2)
let x = Computed(@() w.get() + 1)
let y = Computed(@() w.get() * 2)
let z = Computed(@() w.get() * 3)
let r = Computed(@() x.get() + y.get() + z.get())
update_deferred()
println($"wide diamond: r={r.get()}")

w.set(3)
update_deferred()
println($"wide diamond after w=3: r={r.get()}")
