local benchmark = require("profile").benchmark

-- no result: the sum overflows
local function fibLoop(n)
  local last, cur = 0, 1
  for _ = 1, n - 1 do
    local sum = last + cur
    last = cur
    cur = sum
  end
end

benchmark("fibonacci loop", 20, function() fibLoop(6511134) end)
