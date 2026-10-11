--!strict
local profile = require("./profile")

local COUNT = 10000
local PASSES = 100

local numberStrings: { string } = table.create(COUNT)
for i = 0, COUNT - 1 do
  numberStrings[i + 1] = string.format("%.4f", (i + 1) * 1.0001)
end

local function total(strs: { string }): string
  local sum = 0.0
  for _ = 1, PASSES do
    for _, s in strs do
      sum += tonumber(s) :: number
    end
  end
  return string.format("%.4f", sum)
end

profile.benchmark("string2float", 20, function()
  return total(numberStrings)
end)
