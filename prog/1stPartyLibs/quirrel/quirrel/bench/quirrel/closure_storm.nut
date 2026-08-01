// closure creation and invocation with captured locals, the shape of daRg
// @() builder components
let { clock } = require("datetime")

const N = 1000000

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local sum = 0
  local kept = []
  for (local i = 0; i < N; i++) {
    local a = i
    local b = i * 2
    let f = @() a + b
    sum += f()
    if (i % 1000 == 0)
      kept.append(f)
  }
  foreach (f in kept)
    sum += f()
  println("BENCH closure_storm", ((clock() - t0) * 1000), $"ms (r={sum })")
}
