/*

Proposed optimization: replace t.__merge({small literal table}) with
clone + newslot sequence, avoiding temporary table allocation.

Pattern:
  local r = t.__merge({y = 5, f = 5.3})

Can be optimized to:
  local r = clone t
  r.y <- 5
  r.f <- 5.3

This is faster because __merge allocates a temporary table literal
on every call, while clone + newslot avoids that overhead.

*/

let {clock} = require("datetime")

function profile_it(cnt, f) {
  local res = 0
  for (local i = 0; i < cnt; ++i) {
    let start = clock()
    f()
    let measured = clock() - start
    if (i == 0 || measured < res)
      res = measured
  }
  return res / 1.0
}

let t = { x = 4, z = 5, u = 123 }

function test_clone_newslot() {
  local cnt = 1000000
  local r = null
  while (--cnt) {
    r = clone t
    r.y <- 5
    r.f <- 5.3
  }
  return r
}

function test_merge() {
  local cnt = 1000000
  local r = null
  while (--cnt) {
    r = t.__merge({y = 5, f = 5.3})
  }
  return r
}

let a = test_clone_newslot()
let b = test_merge()
assert(a.x == b.x && a.z == b.z && a.u == b.u && a.y == b.y && a.f == b.f)

const numTests = 10
let profile = @(name, func) println("".concat(name, ", ", profile_it(numTests, func), $", {numTests}"))

profile("\"clone_newslot\"", test_clone_newslot)
profile("\"merge\"", test_merge)
