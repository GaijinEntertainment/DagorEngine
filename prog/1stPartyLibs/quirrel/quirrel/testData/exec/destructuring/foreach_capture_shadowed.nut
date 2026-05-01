// Inner function declares its own param/local with the same name as an
// outer foreach val. The capture scan over-approximates here (no shadow
// tracking), so this triggers the per-iter path. Either way, the visible
// behavior must be correct: closures returning the inner local must NOT
// see the outer per-iter value, and closures actually capturing the outer
// must see the per-iter value.

let outerCaps = []
let innerCaps = []
foreach (v in [1, 2, 3]) {
  // closure captures outer v
  outerCaps.append(@() v)
  // closure shadows v with its own param (no real capture of outer)
  innerCaps.append(@(v) v * 10)
}
foreach (f in outerCaps) println("o:", f())
foreach (f in innerCaps) println("i:", f(7))
