--!strict
local profile = require("./profile")

-- no result: the sum overflows
local function fibLoop(n: number)
  local last = 0
  local cur = 1
  for _ = 1, n - 1 do
    local next = last + cur
    last = cur
    cur = next
  end
end

profile.benchmark("fibonacci loop", 20, function()
  fibLoop(6511134)
end)
