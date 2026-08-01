// string building and formatting (localization/ui-text style)
let { clock } = require("datetime")
let { format } = require("string")

const N = 80000

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local total = 0
  local acc = ""
  for (local i = 0; i < N; i++) {
    local s = format("item %d: %s (%0.1f%%)", i, i % 2 == 0 ? "on" : "off", (i % 100) * 1.0)
    acc = "".concat(acc, s)
    if (i % 100 == 99) {
      total += acc.len()
      acc = ""
    }
  }
  println("BENCH strings", ((clock() - t0) * 1000), $"ms (r={total})")
}
