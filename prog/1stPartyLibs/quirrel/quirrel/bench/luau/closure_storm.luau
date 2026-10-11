--!strict
-- closure creation and invocation with captured locals, the shape of daRg
-- @() builder components
local profile = require("./profile")

local N = 1000000

local function storm(): number
  local sum = 0
  local kept: { () -> number } = {}
  for i = 0, N - 1 do
    local a = i
    local b = i * 2
    local function f(): number
      return a + b
    end
    sum += f()
    if i % 1000 == 0 then
      table.insert(kept, f)
    end
  end
  for _, f in kept do
    sum += f()
  end
  return sum
end

profile.vmBench("closure_storm", storm)
