-- nullable property probe: guarded reads over prop tables where some
-- receivers are absent (safe-navigation equivalent of the Quirrel version)
local clock = os.clock

local TABLES = 2000
local ROUNDS = 300

local tables = {}
for i = 1, TABLES do
  if i % 4 == 0 then
    tables[i] = false
  else
    tables[i] = {
      rendObj = 1, size = i, color = 2, padding = 3, margin = 4,
      flow = 5, halign = 6, valign = 7, gap = 8, opacity = 9
    }
  end
end

for rep = 1, 5 do
  local t0 = clock()
  local sum = 0
  for r = 1, ROUNDS do
    for i = 1, TABLES do
      local t = tables[i]
      if t then
        sum = sum + t.rendObj + t.size + t.color + t.padding + t.margin
                  + t.flow + t.halign + t.valign + t.gap + t.opacity
      end
    end
  end
  print("BENCH nullable_probe " .. ((clock() - t0) * 1000) .. " ms (r=" .. sum .. ")")
end
