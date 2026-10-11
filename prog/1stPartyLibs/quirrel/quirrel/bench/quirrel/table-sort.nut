// Public domain, ported from Lua
from "profile.nut" import benchmark

const COUNT = 100000

// lagged Fibonacci, same numbers in every port
function createRng(seed) {
  let ring = array(17, 0)
  for (local i = 16; i >= 0; i--) {
    seed = (seed * 9069) % 0x80000000
    ring[i] = seed
  }
  local j = 0
  return function() {
    local h = j - 5
    if (h < 0)
      h += 17
    local k = ring[h] - ring[j]
    if (k < 0)
      k += 2147483647
    ring[j] = k
    j = j < 16 ? j + 1 : 0
    return k
  }
}

let rand = createRng(12345)
let data = array(COUNT)
for (local i = 0; i < COUNT; i++)
  data[i] = rand()

function sortCopy() {
  let arr = clone data
  arr.sort(@(a, b) b <=> a)
  return arr[0]
}

benchmark("sort", 10, sortCopy)
