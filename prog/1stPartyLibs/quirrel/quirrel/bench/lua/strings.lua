-- string building and formatting (localization/ui-text style)
local format <const> = string.format
local vmBench <const> = require("profile").vmBench

local N <const> = 80000

local function strings()
  local total = 0
  local acc = ""
  for i = 0, N - 1 do
    local s = format("item %d: %s (%0.1f%%)", i, i % 2 == 0 and "on" or "off", (i % 100) * 1.0)
    acc = acc .. s
    if i % 100 == 99 then
      total = total + #acc
      acc = ""
    end
  end
  return total
end

vmBench("strings", strings)
