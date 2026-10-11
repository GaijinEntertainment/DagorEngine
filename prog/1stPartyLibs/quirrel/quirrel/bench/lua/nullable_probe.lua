-- nullable property probe: guarded reads over prop tables where some
-- receivers are absent (safe-navigation equivalent of the Quirrel version)
local vmBench <const> = require("profile").vmBench

local TABLES <const> = 2000
local ROUNDS <const> = 300

local tables <const> = table.create(TABLES)
for i = 0, TABLES - 1 do
  if i % 4 ~= 3 then
    tables[i + 1] = {
      rendObj = 1, size = i, color = 2, padding = 3, margin = 4,
      flow = 5, halign = 6, valign = 7, gap = 8, opacity = 9
    }
  end
end

local function probe()
  local sum = 0
  for _ = 1, ROUNDS do
    for i = 1, TABLES do
      local t = tables[i]
      if t then
        sum = sum + t.rendObj + t.size + t.color + t.padding + t.margin
            + t.flow + t.halign + t.valign + t.gap + t.opacity
      end
    end
  end
  return sum
end

vmBench("nullable_probe", probe)
