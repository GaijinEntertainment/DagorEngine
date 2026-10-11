--!strict
local profile = require("./profile")

local N = 10000000

local function expLoop(n: number): number
  local sum = 0.0
  for i = 0, n - 1 do
    sum += math.exp(1.0 / (1.0 + i))
  end
  return sum
end

profile.benchmark("exp loop", 20, function()
  return string.format("%.6f", expLoop(N))
end)
