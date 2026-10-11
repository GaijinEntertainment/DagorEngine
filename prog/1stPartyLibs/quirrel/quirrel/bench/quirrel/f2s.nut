from "profile.nut" import benchmark

const COUNT = 10000
const PASSES = 100

let floats = array(COUNT)
for (local i = 0; i < COUNT; i++)
  floats[i] = (i + 1) * 1.0001

// no result: each language formats floats differently
function totalLength(nums) {
  local total = 0
  for (local pass = 0; pass < PASSES; pass++)
    foreach (x in nums)
      total += x.tostring().len()
}

benchmark("float2string", 20, @() totalLength(floats))
