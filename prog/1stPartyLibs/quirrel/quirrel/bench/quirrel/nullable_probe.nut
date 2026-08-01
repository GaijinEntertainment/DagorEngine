// nullable property probe: ?. reads over daRg-like prop tables where some
// receivers are null, the shape of component builders probing optional props
let { clock } = require("datetime")

const TABLES = 2000
const ROUNDS = 300

let tables = []
for (local i = 0; i < TABLES; i++) {
  tables.append((i % 4 == 3) ? null : {
    rendObj = 1, size = i, color = 2, padding = 3, margin = 4,
    flow = 5, halign = 6, valign = 7, gap = 8, opacity = 9
  })
}

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local sum = 0
  for (local r = 0; r < ROUNDS; r++) {
    for (local i = 0; i < TABLES; i++) {
      let t = tables[i]
      sum += (t?.rendObj ?? 0) + (t?.size ?? 0) + (t?.color ?? 0) + (t?.padding ?? 0) + (t?.margin ?? 0)
           + (t?.flow ?? 0) + (t?.halign ?? 0) + (t?.valign ?? 0) + (t?.gap ?? 0) + (t?.opacity ?? 0)
    }
  }
  println("BENCH nullable_probe", ((clock() - t0) * 1000), $"ms (r={sum})")
}
