from "profile.nut" import benchmark

// no result: the sum overflows
function fibLoop(n) {
  local last = 0
  local cur = 1
  for (local i = 1; i < n; i++) {
    let next = last + cur
    last = cur
    cur = next
  }
}

benchmark("fibonacci loop", 20, @() fibLoop(6511134))
