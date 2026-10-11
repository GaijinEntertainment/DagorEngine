local exp <const> = math.exp
local format <const> = string.format
local benchmark <const> = require("profile").benchmark

local N <const> = 10000000

local function expLoop(n)
  local sum = 0.0
  for i = 0, n - 1 do
    sum = sum + exp(1.0 / (1.0 + i))
  end
  return sum
end

benchmark("exp loop", 20, function() return format("%.6f", expLoop(N)) end)
