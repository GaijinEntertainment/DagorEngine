local tostring <const> = tostring
local benchmark <const> = require("profile").benchmark

local COUNT <const> = 10000
local PASSES <const> = 100

local floats <const> = table.create(COUNT)
for i = 0, COUNT - 1 do
  floats[i + 1] = (i + 1) * 1.0001
end

-- no result: float formats differ per language
local function totalLength(nums)
  local total = 0
  for _ = 1, PASSES do
    for i = 1, #nums do
      total = total + #tostring(nums[i])
    end
  end
end

benchmark("float2string", 20, function() totalLength(floats) end)
