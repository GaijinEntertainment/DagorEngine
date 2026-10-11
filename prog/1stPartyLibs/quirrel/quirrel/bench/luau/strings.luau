--!strict
-- string building and formatting (localization/ui-text style)
local profile = require("./profile")

local N = 80000

local function strings(): number
  local total = 0
  local acc = ""
  for i = 0, N - 1 do
    local s = string.format("item %d: %s (%0.1f%%)", i, if i % 2 == 0 then "on" else "off", (i % 100) * 1.0)
    acc ..= s
    if i % 100 == 99 then
      total += #acc
      acc = ""
    end
  end
  return total
end

profile.vmBench("strings", strings)
