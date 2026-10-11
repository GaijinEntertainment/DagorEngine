local benchmark = require("profile").benchmark
local new_tab = require("table.new")
local format = string.format
local tonumber = tonumber

local COUNT = 10000
local PASSES = 100

local numberStrings = new_tab(COUNT, 0)
for i = 1, COUNT do
  numberStrings[i] = format("%.4f", i * 1.0001)
end

local function total(strs)
  local n = #strs
  local sum = 0.0
  for _ = 1, PASSES do
    for k = 1, n do
      sum = sum + tonumber(strs[k])
    end
  end
  return format("%.4f", sum)
end

benchmark("string2float", 20, function() return total(numberStrings) end)
