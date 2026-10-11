dofile("profile.nut")

const N = 10000000

local exp = ::exp

local function expLoop(n) {
  local sum = 0.0
  for (local i = 0; i < n; i++)
    sum += exp(1.0 / (1.0 + i))
  return sum
}

benchmark("exp loop", 20, @() format("%.6f", expLoop(N)))
