from "math" import exp
from "string" import format
from "profile.nut" import benchmark

const N = 10000000

function expLoop(n) {
  local sum = 0.0
  for (local i = 0; i < n; i++)
    sum += exp(1.0 / (1.0 + i))
  return sum
}

benchmark("exp loop", 20, @() format("%.6f", expLoop(N)))
