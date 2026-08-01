// recursive call overhead
let { clock } = require("datetime")

function fib(n) {
  return n < 2 ? n : fib(n - 1) + fib(n - 2)
}

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local r = fib(30)
  println("BENCH fib", ((clock() - t0) * 1000), $"ms (r={r})")
}
