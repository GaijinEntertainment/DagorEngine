// property-probe storm: repeated string-keyed reads over many small tables,
// the shape of Element::setup re-reading ~60 properties per element
let { clock } = require("datetime")

const TABLES = 2000
const ROUNDS = 300

let tables = []
for (local i = 0; i < TABLES; i++) {
  tables.append({
    rendObj = 1, size = i, color = 2, text = "x", padding = 3,
    margin = 4, flow = 5, halign = 6, valign = 7, gap = 8,
    opacity = 9, zOrder = 10, clipChildren = 11, behavior = 12, hotkeys = 13,
    transform = 14, animations = 15, transitions = 16, watch = 17, onClick = 18,
    onHover = 19, sound = 20, group = 21, xmbNode = 22, scrollHandler = 23
  })
}

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local sum = 0
  for (local r = 0; r < ROUNDS; r++) {
    for (local i = 0; i < TABLES; i++) {
      let t = tables[i]
      sum += t.rendObj + t.size + t.color + t.padding + t.margin
           + t.flow + t.halign + t.valign + t.gap + t.opacity
           + t.zOrder + t.clipChildren + t.behavior + t.hotkeys + t.transform
           + t.animations + t.transitions + t.watch + t.onClick + t.onHover
    }
  }
  println("BENCH probe_storm", ((clock() - t0) * 1000), $"ms (r={sum})")
}
