local benchmark <const> = require("profile").benchmark

-- no result: the sum overflows
local function fibLoop(n)
  local last = 0
  local cur = 1
  for _ = 1, n - 1 do
    local next = last + cur
    last = cur
    cur = next
  end
end

benchmark("fibonacci loop", 20, function() fibLoop(6511134) end)
