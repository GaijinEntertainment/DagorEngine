// idx + val captured: both rebound per iteration.
let fns = []
foreach (i, v in ["a", "b", "c"]) {
  fns.append(@() i + ":" + v)
}
foreach (f in fns) println("a1:", f())

// Only idx captured (val unused inside closure).
let gs = []
foreach (i, _v in [100, 200, 300]) {
  gs.append(@() i * 2)
}
foreach (g in gs) println("a2:", g())

// Closure captures idx via inner named function (not just lambda).
let hs = []
foreach (i, v in [7, 8, 9]) {
  function bind() { return i * 10 + v }
  hs.append(bind)
}
foreach (h in hs) println("a3:", h())
