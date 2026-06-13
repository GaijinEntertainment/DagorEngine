// f1: array destructure, no idx
let result = []
let arr2d = [[1, 2, 3], [4, 5, 6]]
foreach ([a, b, c] in arr2d) {
  result.append($"f01: {a} {b} {c}")
}

// f2: array destructure with idx
foreach (i, [a, b, c] in arr2d) {
  result.append($"f02: {i} {a} {b} {c}")
}

// f3: table destructure, no idx
let tbls = [{x = 1, y = 2}, {x = 3, y = 4}]
foreach ({x, y} in tbls) {
  result.append($"f03: {x} {y}")
}

// f4: table destructure with idx
foreach (i, {x, y} in tbls) {
  result.append($"f04: {i} {x} {y}")
}

// f5: defaults on missing keys
let partial = [{x = 10}, {y = 20}, {}]
foreach ({x = -1, y = -2} in partial) {
  result.append($"f05: {x} {y}")
}

// f6: typed destructure with default
foreach ({x: int = 0, y: int|null = null} in [{x = 5}, {y = 7}]) {
  result.append($"f06: {x} {y}")
}

// f7: nested foreach destructures
let nested = [
  [{p = 1, q = 2}, {p = 3}],
  [{q = 9}]
]
foreach (i, row in nested) {
  foreach (j, {p = 0, q = 0} in row) {
    result.append($"f07: {i} {j} {p} {q}")
  }
}

// f8: mixed -- outer destructured array, inner plain
foreach ([first, second] in [[10, 20], [30, 40]]) {
  result.append($"f08: {first} {second}")
}

// f9: destructure value while reading idx
let m = {alpha = {n = 1}, beta = {n = 2}}
foreach (k, {n} in m) {
  result.append($"f09: {k} {n}")
}

// f10: instance destructure
class Pt { x = 0; y = 0; constructor(x_, y_) { this.x = x_; this.y = y_ } }
foreach (k, {x, y} in {a = Pt(1, 2), b = Pt(3, 4)}) {
  result.append($"f10: {k} {x} {y}")
}

// f11: empty defaults flow when iterating same array twice
let runs = [{}, {x = 5}]
foreach ({x = 99} in runs) {
  result.append($"f11: {x}")
}
foreach ({x = 99} in runs) {
  result.append($"f11b: {x}")
}

// f12: capture destructured var in closure
let fns = []
foreach ({v} in [{v = 1}, {v = 2}, {v = 3}]) {
  fns.append(@() v)
}
foreach (f in fns) {
  result.append($"f12: {f()}")
}

result.sort()
foreach (line in result) {
  print(line + "\n")
}
