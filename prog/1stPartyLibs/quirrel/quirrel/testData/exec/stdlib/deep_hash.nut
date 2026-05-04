from "math" import *
from "string" import *

class A {
  w = 333
}

let inst1 = A()
let inst2 = A()

assert(deep_hash({}) == deep_hash({}))
assert(deep_hash({x = 3}) == deep_hash({x = 3}))
assert(deep_hash({x = {y = 444}}, 2) == deep_hash({x = {y = 111}}, 2))
assert(deep_hash({xxx = {y = 444}}, 2) != deep_hash({qqq = {y = 444}}, 2))
assert(deep_hash([{x = 3}, null, 0.75]) == deep_hash([{x = 3}, null, 0.75]))
assert(deep_hash([{x = 3}, null, 0.75]) != deep_hash([{x = 4}, null, 0.75]))

assert(deep_hash([1, 2, 3]) == deep_hash([1, 2, 3]))
assert(deep_hash([1, 2, 3]) != deep_hash([3, 2, 1]))

assert(deep_hash(inst1) == deep_hash(inst2))
inst2.w = 999
assert(deep_hash(inst1) != deep_hash(inst2))

assert(hash("a123") == deep_hash("a123"))
assert(hash("a123") != deep_hash("XXX"))

assert(hash("a123") == hash("a123"))
assert(hash("a123") != hash("XXX"))

assert(hash(0) != hash(false))
assert(hash(0) != hash(0.0))
assert(hash(-0.0) != hash(0.0))
assert(hash("") != hash(0))
assert(hash("") != hash([]))
assert(hash({}) != hash([]))

assert(hash(0) > 100000)
assert(hash(null) > 100000)
assert(hash(0.0) > 100000)
assert(hash("") > 100000)
assert(hash({}) > 100000)
assert(hash([]) > 100000)
assert(hash(class {}) > 100000)

local arr = []
local cnt = 1000
while (cnt--) {
  arr.append(cnt)
  assert(deep_hash(arr) > 0)
}

println("ok")
