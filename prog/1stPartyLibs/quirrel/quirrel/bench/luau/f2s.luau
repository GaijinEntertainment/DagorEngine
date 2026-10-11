--!strict
local profile = require("./profile")

local COUNT = 10000
local PASSES = 100

local floats: { number } = table.create(COUNT)
for i = 0, COUNT - 1 do
  floats[i + 1] = (i + 1) * 1.0001
end

-- no result: each language formats floats differently
local function totalLength(nums: { number })
  local total = 0
  for _ = 1, PASSES do
    for _, x in nums do
      total += #tostring(x)
    end
  end
end

profile.benchmark("float2string", 20, function()
  totalLength(floats)
end)
