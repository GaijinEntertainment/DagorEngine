// type() and classof() checks on mixed values (ui-code style argument checks)
from "types" import classof, Array
from "profile.nut" import vmBench

const N = 400000
let values = [1, 1.5, "s", [1], {a = 1}, null, true, @() null]

function typeChecks() {
  local hits = 0
  let n = values.len()
  for (local i = 0; i < N; i++) {
    let v = values[i % n]
    if (type(v) == "string" || type(v) == "integer")
      hits++
    if (classof(v) == Array)
      hits += 2
  }
  return hits
}

vmBench("type_checks", typeChecks)
