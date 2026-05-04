// =====================================================================
// 1. Three-level nested foreach with destructure at every level
// =====================================================================
let world = [
  {name = "north", regions = [
    {tag = "a", cells = [{x = 1, y = 2}, {x = 3, y = 4}]},
    {tag = "b", cells = [{x = 5, y = 6}]}
  ]},
  {name = "south", regions = [
    {tag = "c", cells = [{x = 7, y = 8}, {x = 9, y = 10}]}
  ]}
]
foreach (i, {name, regions} in world) {
  foreach (j, {tag, cells} in regions) {
    foreach (k, {x, y} in cells) {
      println("L1:", i, name, j, tag, k, x, y)
    }
  }
}


// =====================================================================
// 2. Destructured value used inside an inner function defined per iter
// =====================================================================
let factories = []
foreach (i, {label, basev} in [{label = "alpha", basev = 10}, {label = "beta", basev = 100}]) {
  let mult = i + 1
  function build(extra) {
    return label + ":" + (basev * mult + extra)
  }
  factories.append(build)
}
foreach (f in factories) {
  println("L2:", f(7))
}


// =====================================================================
// 3. Destructured value flows through nested functions and a class method
// =====================================================================
class Counter {
  total = 0
  function add(v) { this.total += v; return this.total }
}
let counter = Counter()
let pipeline = [{op = "x", n = 3}, {op = "y", n = 5}, {op = "z", n = 2}]
foreach (i, {op, n} in pipeline) {
  let local_op = op
  function step() {
    function inner() {
      return counter.add(n * (i + 1))
    }
    return local_op + "->" + inner()
  }
  println("L3:", step())
}
println("L3 total:", counter.total)


// =====================================================================
// 4. Defaults + type masks combined with nested foreach
// =====================================================================
let groups = [
  {gid = 1, members = [{n = "a", age = 10}, {n = "b"}, {age = 99}]},
  {gid = 2, members = []},
  {gid = 3, members = [{n = "c", age = 7}]}
]
foreach ({gid, members} in groups) {
  foreach ({n = "?", age: int|null = null} in members) {
    println("L4:", gid, n, age)
  }
}


// =====================================================================
// 5. Capturing destructured value per iteration via inner let-destructure
// =====================================================================
let getters = []
foreach (item in [{key = "a", val = 1}, {key = "b", val = 2}, {key = "c", val = 3}]) {
  let {key, val} = item
  getters.append(@() key + "=" + val)
}
foreach (g in getters) {
  println("L5:", g())
}


// =====================================================================
// 6. Modifying the source table through destructure idx alias
// =====================================================================
let mutable_rows = [{count = 1}, {count = 2}, {count = 3}]
foreach (i, {count} in mutable_rows) {
  // count is a copy of the field; mutate via index, not via destructured local
  mutable_rows[i].count = count * 10
}
foreach ({count} in mutable_rows) {
  println("L6:", count)
}


// =====================================================================
// 7. break / continue / return inside nested destructuring loops
// =====================================================================
function findFirstHigh(rows) {
  foreach (i, {label, scores} in rows) {
    foreach (j, {v} in scores) {
      if (v < 50) continue
      if (v > 200) return [label, i, j, v, "stop"]
      if (v > 100) return [label, i, j, v, "ok"]
    }
  }
  return null
}
let r = findFirstHigh([
  {label = "first",  scores = [{v = 10}, {v = 30}]},
  {label = "second", scores = [{v = 70}, {v = 130}, {v = 9000}]},
  {label = "third",  scores = [{v = 1}]}
])
if (r != null) {
  let [a, b, c, d, e] = r
  println("L7:", a, b, c, d, e)
}


// =====================================================================
// 8. Destructuring over class instances with inherited fields
// =====================================================================
class Base {
  x = 0
  y = 0
  constructor(x_, y_) { this.x = x_; this.y = y_ }
}
class Derived (Base) {
  z = 0
  constructor(x_, y_, z_) { base.constructor(x_, y_); this.z = z_ }
}
let pts = [Base(1, 2), Derived(3, 4, 5), Base(6, 7), Derived(8, 9, 10)]
foreach (i, {x, y, z = -1} in pts) {
  println("L8:", i, x, y, z)
}


// =====================================================================
// 9. Generator/coroutine + destructure from yielded values
// =====================================================================
let gen = function() {
  yield {tag = "a", n = 1}
  yield {tag = "b", n = 2}
  yield {tag = "c", n = 3}
}
let g = gen()
foreach (i, {tag, n} in g) {
  println("L9:", i, tag, n)
}


// =====================================================================
// 10. Destructured value as constructor argument and method call result
// =====================================================================
class Box {
  v = 0
  constructor(v_) { this.v = v_ }
  function bump(d) { this.v += d; return this }
  function get() { return this.v }
}
let configs = [{init = 100, deltas = [1, 2, 3]}, {init = 1000, deltas = [10, 20]}]
let boxes = []
foreach ({init, deltas} in configs) {
  let b = Box(init)
  foreach (d in deltas) b.bump(d)
  boxes.append(b)
}
foreach (i, b in boxes) {
  println("L10:", i, b.get())
}


// =====================================================================
// 11. Heterogeneous default types (int default vs string default vs null)
// =====================================================================
let mixed = [
  {n = 5, s = "hello", b = true},
  {n = 7},
  {s = "world"},
  {},
  {b = false, s = "x", n = 9}
]
foreach (i, {n: int = -1, s: string = "?", b: bool|null = null} in mixed) {
  println("L11:", i, n, s, b)
}


// =====================================================================
// 12. Destructure with idx and value, then re-iterate inside body
// =====================================================================
let matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
foreach (i, [a, b, c] in matrix) {
  let row_sum = a + b + c
  // inner foreach over the same row by index access
  let parts = []
  foreach (cell in [a, b, c]) parts.append(cell * (i + 1))
  println("L12:", i, row_sum, parts[0], parts[1], parts[2])
}


// =====================================================================
// 13. Function returning destructure-friendly tables, chained
// =====================================================================
function mkPair(a, b) { return {first = a, second = b} }
let pairs = [mkPair(1, "one"), mkPair(2, "two"), mkPair(3, "three")]
let folded = (function() {
  let parts = []
  foreach ({first, second} in pairs) {
    parts.append(second + "=" + first)
  }
  return parts
})()
foreach (s in folded) {
  println("L13:", s)
}


// =====================================================================
// 14. Closure capture of idx + destructured field; called after loop
// =====================================================================
let bound = []
foreach (i, {tag, weight} in [{tag = "p", weight = 3}, {tag = "q", weight = 7}]) {
  let captured_tag = tag
  let captured_weight = weight
  bound.append(function() {
    return captured_tag + "@" + i + "=" + captured_weight
  })
}
foreach (f in bound) {
  println("L14:", f())
}


// =====================================================================
// 15. Destructure in foreach inside a class method
// =====================================================================
class Aggregator {
  data = null
  constructor() { this.data = [] }
  function ingest(rows) {
    foreach ({src, val = 0} in rows) {
      this.data.append(src + ":" + val)
    }
  }
}
let agg = Aggregator()
agg.ingest([{src = "a", val = 1}, {src = "b"}, {src = "c", val = 9}])
foreach (s in agg.data) {
  println("L15:", s)
}
