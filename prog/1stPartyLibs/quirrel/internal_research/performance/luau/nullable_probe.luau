--!strict
-- nullable property probe: guarded reads over prop tables where some
-- receivers are absent (safe-navigation equivalent of the Quirrel version)
local profile = require("./profile")

type Props = {
  rendObj: number, size: number, color: number, padding: number, margin: number,
  flow: number, halign: number, valign: number, gap: number, opacity: number,
}

local TABLES = 2000
local ROUNDS = 300

local tables: { Props? } = table.create(TABLES)
for i = 0, TABLES - 1 do
  tables[i + 1] = if i % 4 == 3 then nil else {
    rendObj = 1, size = i, color = 2, padding = 3, margin = 4,
    flow = 5, halign = 6, valign = 7, gap = 8, opacity = 9,
  }
end

local function probe(): number
  local sum = 0
  for _ = 1, ROUNDS do
    -- numeric loop: generalized iteration skips nil receivers
    for i = 1, TABLES do
      local t = tables[i]
      if t then
        sum += t.rendObj + t.size + t.color + t.padding + t.margin
          + t.flow + t.halign + t.valign + t.gap + t.opacity
      end
    end
  end
  return sum
end

profile.vmBench("nullable_probe", probe)
