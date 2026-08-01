-- property-probe storm: repeated string-keyed reads over many small tables,
-- the shape of Element::setup re-reading ~60 properties per element
local TABLES = 2000
local ROUNDS = 300

local tables = {}
for i = 1, TABLES do
  tables[i] = {
    rendObj = 1, size = i, color = 2, text = "x", padding = 3,
    margin = 4, flow = 5, halign = 6, valign = 7, gap = 8,
    opacity = 9, zOrder = 10, clipChildren = 11, behavior = 12, hotkeys = 13,
    transform = 14, animations = 15, transitions = 16, watch = 17, onClick = 18,
    onHover = 19, sound = 20, group = 21, xmbNode = 22, scrollHandler = 23,
  }
end

for rep = 1, 5 do
  local t0 = os.clock()
  local sum = 0
  for r = 1, ROUNDS do
    for i = 1, TABLES do
      local t = tables[i]
      sum = sum + t.rendObj + t.size + t.color + t.padding + t.margin
          + t.flow + t.halign + t.valign + t.gap + t.opacity
          + t.zOrder + t.clipChildren + t.behavior + t.hotkeys + t.transform
          + t.animations + t.transitions + t.watch + t.onClick + t.onHover
    end
  end
  print("BENCH probe_storm " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. sum .. ")")
end
