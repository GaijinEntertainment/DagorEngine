// == null and != null branches: list walks and optional-value checks
from "profile.nut" import vmBench

const N = 400000
let values = [1, null, "s", {a = 1}, null, 0, [1], null]

local list = null
for (local i = 0; i < 64; i++)
  list = { v = i % 3 == 0 ? null : i, next = list }

function nullChecks() {
  local hits = 0
  let n = values.len()
  for (local i = 0; i < N; i++) {
    let v = values[i % n]
    if (v == null)
      hits++
    if (v != null)
      hits += 2
  }
  for (local r = 0; r < N / 64; r++) {
    local node = list
    while (node != null) {
      if (node.v != null)
        hits += 3
      node = node.next
    }
  }
  return hits
}

vmBench("null_checks", nullChecks)
