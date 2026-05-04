// f1: array destructure, no idx
let arr2d = [[1, 2, 3], [4, 5, 6]]
foreach ([a, b, c] in arr2d) {
  println("f1:", a, b, c)
}

// f2: array destructure with idx
foreach (i, [a, b, c] in arr2d) {
  println("f2:", i, a, b, c)
}

// f3: table destructure, no idx
let tbls = [{x = 1, y = 2}, {x = 3, y = 4}]
foreach ({x, y} in tbls) {
  println("f3:", x, y)
}

// f4: table destructure with idx
foreach (i, {x, y} in tbls) {
  println("f4:", i, x, y)
}

// f5: defaults on missing keys
let partial = [{x = 10}, {y = 20}, {}]
foreach ({x = -1, y = -2} in partial) {
  println("f5:", x, y)
}

// f6: typed destructure with default
foreach ({x: int = 0, y: int|null = null} in [{x = 5}, {y = 7}]) {
  println("f6:", x, y)
}

// f7: nested foreach destructures
let nested = [
  [{p = 1, q = 2}, {p = 3}],
  [{q = 9}]
]
foreach (i, row in nested) {
  foreach (j, {p = 0, q = 0} in row) {
    println("f7:", i, j, p, q)
  }
}

// f8: mixed -- outer destructured array, inner plain
foreach ([first, second] in [[10, 20], [30, 40]]) {
  println("f8:", first, second)
}

// f9: destructure value while reading idx
let m = {alpha = {n = 1}, beta = {n = 2}}
foreach (k, {n} in m) {
  println("f9:", k, n)
}

// f10: instance destructure
class Pt { x = 0; y = 0; constructor(x_, y_) { this.x = x_; this.y = y_ } }
foreach (k, {x, y} in {a = Pt(1, 2), b = Pt(3, 4)}) {
  println("f10:", k, x, y)
}

// f11: empty defaults flow when iterating same array twice
let runs = [{}, {x = 5}]
foreach ({x = 99} in runs) {
  println("f11:", x)
}
foreach ({x = 99} in runs) {
  println("f11b:", x)
}

// f12: capture destructured var in closure
let fns = []
foreach ({v} in [{v = 1}, {v = 2}, {v = 3}]) {
  fns.append(@() v)
}
foreach (f in fns) {
  println("f12:", f())
}
