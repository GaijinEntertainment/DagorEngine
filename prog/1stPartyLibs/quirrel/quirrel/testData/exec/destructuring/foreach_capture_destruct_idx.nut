// foreach with idx + destructured val: idx is in the foreach scope (shared
// across iters by default), x/y are body-block locals (per-iter via parser
// desugaring). When idx is captured by an inner closure, idx must also be
// rebound per-iter.

let fns = []
foreach (i, {x, y} in [{x = 1, y = 2}, {x = 10, y = 20}, {x = 100, y = 200}]) {
  fns.append(@() i * 1000 + x * 10 + y)
}
foreach (f in fns) println("d1:", f())

// Same shape but idx not captured: only destructured fields captured. The
// parser's body-Block scope already gives x/y per-iter; idx scan reports
// no capture, so foreach uses the shared-slot path for idx.
let gs = []
foreach (_i, {x, y} in [{x = 7, y = 8}, {x = 70, y = 80}]) {
  gs.append(@() x + y)
}
foreach (g in gs) println("d2:", g())

// Destructured field default that *is* a closure (it captures an outer
// var, but is evaluated per-iter so closures see fresh state each time).
let cap = 100
let hs = []
foreach ({n = (function() { return cap })()} in [{n = 1}, {}, {n = 3}]) {
  hs.append(@() n)
}
foreach (h in hs) println("d3:", h())
