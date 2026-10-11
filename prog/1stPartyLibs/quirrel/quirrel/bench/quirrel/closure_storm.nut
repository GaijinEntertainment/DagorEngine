// closure creation and invocation with captured locals, the shape of daRg
// @() builder components
from "profile.nut" import vmBench

const N = 1000000

function storm() {
  local sum = 0
  let kept = []
  for (local i = 0; i < N; i++) {
    let a = i
    let b = i * 2
    let f = @() a + b
    sum += f()
    if (i % 1000 == 0)
      kept.append(f)
  }
  foreach (f in kept)
    sum += f()
  return sum
}

vmBench("closure_storm", storm)
