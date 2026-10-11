local tonumber <const> = tonumber
local format <const> = string.format
local benchmark <const> = require("profile").benchmark

local COUNT <const> = 10000
local PASSES <const> = 100

local numberStrings <const> = table.create(COUNT)
for i = 0, COUNT - 1 do
  numberStrings[i + 1] = format("%.4f", (i + 1) * 1.0001)
end

local function total(strs)
  local sum = 0.0
  for _ = 1, PASSES do
    for i = 1, #strs do
      sum = sum + tonumber(strs[i])
    end
  end
  return format("%.4f", sum)
end

benchmark("string2float", 20, function() return total(numberStrings) end)
