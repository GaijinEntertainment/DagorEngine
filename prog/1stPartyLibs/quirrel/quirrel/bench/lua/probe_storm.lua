-- property-probe storm: repeated string-keyed reads over many small tables,
-- the shape of Element::setup re-reading ~60 properties per element
local vmBench <const> = require("profile").vmBench

local TABLES <const> = 2000
local ROUNDS <const> = 300

local tables <const> = table.create(TABLES)
for i = 0, TABLES - 1 do
  tables[i + 1] = {
    rendObj = 1, size = i, color = 2, text = "x", padding = 3,
    margin = 4, flow = 5, halign = 6, valign = 7, gap = 8,
    opacity = 9, zOrder = 10, clipChildren = 11, behavior = 12, hotkeys = 13,
    transform = 14, animations = 15, transitions = 16, watch = 17, onClick = 18,
    onHover = 19, sound = 20, group = 21, xmbNode = 22, scrollHandler = 23,
  }
end

local function probe()
  local sum = 0
  for _ = 1, ROUNDS do
    for i = 1, TABLES do
      local t = tables[i]
      sum = sum + t.rendObj + t.size + t.color + t.padding + t.margin
          + t.flow + t.halign + t.valign + t.gap + t.opacity
          + t.zOrder + t.clipChildren + t.behavior + t.hotkeys + t.transform
          + t.animations + t.transitions + t.watch + t.onClick + t.onHover
    end
  end
  return sum
end

vmBench("probe_storm", probe)
