local benchmark = require("profile").benchmark
local new_tab = require("table.new")
local tostring = tostring

local COUNT = 10000
local PASSES = 100

local floats = new_tab(COUNT, 0)
for i = 1, COUNT do
  floats[i] = i * 1.0001
end

-- no result: each language formats floats differently
local function totalLength(nums)
  local n = #nums
  local total = 0
  for _ = 1, PASSES do
    for k = 1, n do
      total = total + #tostring(nums[k])
    end
  end
end

benchmark("float2string", 20, function() totalLength(floats) end)
