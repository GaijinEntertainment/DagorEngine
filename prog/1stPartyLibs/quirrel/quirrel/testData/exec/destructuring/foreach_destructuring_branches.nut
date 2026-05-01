// B01: empty table pattern (parser: '{' immediately followed by '}')
foreach ({} in [{a = 1}, {a = 2}]) println("B01")

// B02: empty array pattern (parser: '[' immediately followed by ']')
foreach ([] in [[1, 2], [3]]) println("B02")

// B03: destructure-only val, no idx (firstIsDestr branch)
foreach ({x} in [{x = 10}]) println("B03", x)

// B04: idx + destructured val (idx-then-destructure branch in parser)
foreach (i, {x} in [{x = 10}, {x = 20}]) println("B04", i, x)

// B05: idx + plain val (existing path, regression guard)
foreach (i, v in [11, 22]) println("B05", i, v)

// B06: plain val (existing path, regression guard)
foreach (v in [33, 44]) println("B06", v)

// B07: type mask without default (runtime EmitCheckType, initializer == null)
foreach ({n: int} in [{n = 1}, {n = 2}]) println("B07", n)

// B08: default without type mask (initializer present, typeMask == ~0u)
foreach ({m = -9} in [{m = 5}, {}]) println("B08", m)

// B09: default + type mask combined (initializer present, typeMask present)
foreach ({k: int = -1} in [{}, {k = 7}]) println("B09", k)

// B10: nullable union default
foreach ({s: string|null = null} in [{s = "ok"}, {}]) println("B10", s)

// B11: array destructure index-based (DT_ARRAY codegen branch with GetNumericConstant)
foreach ([a, b] in [[1, 2], [10, 20]]) println("B11", a, b)

// B12: array destructure with default for missing element
foreach ([a, b = 99] in [[1], [10, 20]]) println("B12", a, b)

// B13: multiple decls inside one destructure with mixed shapes
foreach ({a, b: int = 0, c = "?", d: int|null = null} in [
  {a = 1, c = "x"},
  {a = 2, b = 5, d = 42},
  {a = 3}
]) println("B13", a, b, c, d)

foreach ({arr} in [{arr = [1, 2]}, {arr = [3, 4, 5]}]) {
  local sum = 0
  foreach (n in arr) sum += n
  println("B14", sum)
}

let cap = 100
foreach ({v} in [{v = 1}, {v = 2}]) {
  let make = function() { return v + cap }
  println("B15", make())
}

foreach (val in [7, 8]) println("B16", val)

function defv() { return 77 }
foreach ({z = defv()} in [{z = 1}, {}, {z = 3}]) println("B17", z)

class Acc {
  data = null
  constructor() { this.data = [] }
  function ingest(rows) {
    foreach ({tag = "?"} in rows) this.data.append(tag)
  }
}
let acc = Acc()
acc.ingest([{tag = "a"}, {}, {tag = "c"}])
foreach (s in acc.data) println("B18", s)
