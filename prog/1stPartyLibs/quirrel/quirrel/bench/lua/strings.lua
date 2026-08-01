-- string building and formatting (localization/ui-text style)
local format = string.format

local N = 80000

for rep = 1, 5 do
  local t0 = os.clock()
  local total = 0
  local acc = ""
  for i = 0, N - 1 do
    local s = format("item %d: %s (%0.1f%%)", i, (i % 2 == 0) and "on" or "off", (i % 100) * 1.0)
    acc = acc .. s
    if i % 100 == 99 then
      total = total + #acc
      acc = ""
    end
  end
  print("BENCH strings " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. total .. ")")
end
