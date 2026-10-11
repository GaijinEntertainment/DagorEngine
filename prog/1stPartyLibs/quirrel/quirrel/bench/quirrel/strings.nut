// string building and formatting (localization/ui-text style)
from "string" import format
from "profile.nut" import vmBench

const N = 80000

function strings() {
  local total = 0
  local acc = ""
  for (local i = 0; i < N; i++) {
    let s = format("item %d: %s (%0.1f%%)", i, i % 2 == 0 ? "on" : "off", (i % 100) * 1.0)
    acc = "".concat(acc, s)
    if (i % 100 == 99) {
      total += acc.len()
      acc = ""
    }
  }
  return total
}

vmBench("strings", strings)
