local benchmark = require("profile").benchmark
local exp = math.exp
local format = string.format

local N = 10000000

local function expLoop(n)
  local sum = 0.0
  for i = 0, n - 1 do
    sum = sum + exp(1.0 / (1.0 + i))
  end
  return sum
end

benchmark("exp loop", 20, function() return format("%.6f", expLoop(N)) end)
