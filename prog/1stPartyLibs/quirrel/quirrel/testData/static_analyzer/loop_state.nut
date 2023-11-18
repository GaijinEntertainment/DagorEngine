function foo() {}

local found = false

let diap = foo()


foreach (_seg in diap) {
//   if (seg[1] - seg[0] < size)
//     continue

  if (found)
    continue

  found = true
}

return 0