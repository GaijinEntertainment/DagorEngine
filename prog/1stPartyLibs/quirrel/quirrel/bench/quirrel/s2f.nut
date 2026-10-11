from "string" import format
from "profile.nut" import benchmark

const COUNT = 10000
const PASSES = 100

let numberStrings = array(COUNT)
for (local i = 0; i < COUNT; i++)
  numberStrings[i] = format("%.4f", (i + 1) * 1.0001)

function total(strs) {
  local sum = 0.0
  for (local pass = 0; pass < PASSES; pass++)
    foreach (s in strs)
      sum += s.tofloat()
  return format("%.4f", sum)
}

benchmark("string2float", 20, @() total(numberStrings))
