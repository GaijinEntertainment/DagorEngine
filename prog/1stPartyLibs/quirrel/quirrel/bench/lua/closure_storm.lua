-- closure creation and invocation with captured locals, the shape of daRg
-- @() builder components
local create <const> = table.create
local vmBench <const> = require("profile").vmBench

local N <const> = 1000000
local KEEP_EVERY <const> = 1000

local function storm()
  local sum = 0
  local kept = create(N // KEEP_EVERY)
  local keptCount = 0
  for i = 0, N - 1 do
    local a = i
    local b = i * 2
    local f = function() return a + b end
    sum = sum + f()
    if i % KEEP_EVERY == 0 then
      keptCount = keptCount + 1
      kept[keptCount] = f
    end
  end
  for i = 1, keptCount do
    sum = sum + kept[i]()
  end
  return sum
end

vmBench("closure_storm", storm)
