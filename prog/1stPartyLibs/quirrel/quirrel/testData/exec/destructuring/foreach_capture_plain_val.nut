// Plain `foreach (v in arr)`: closure captures `v`. Each closure must see
// its own per-iteration value (not the final shared-slot value).
let fns = []
foreach (v in [10, 20, 30]) {
  fns.append(@() v)
}
foreach (f in fns) println("v1:", f())

// One-liner body form, same expectation.
let gs = []
foreach (s in ["a", "b", "c"]) gs.append(@() s)
foreach (g in gs) println("v2:", g())

// Nested foreach: inner closure captures both outer and inner val.
let pairs = []
foreach (a in [1, 2]) {
  foreach (b in [10, 20]) {
    pairs.append(@() a * 100 + b)
  }
}
foreach (f in pairs) println("v3:", f())
