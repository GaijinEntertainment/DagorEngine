import "math" as math

// Its result is not a constant, so a call with it runs as _OP_FASTCALL and is not folded
let runtime = @(v) v

println(math.abs(runtime(-5)))
println(math.min(3, 1, runtime(2)))
println(math.clamp(runtime(5), 0, 3))

try {
  math.srand()
} catch (e) {
  println($"caught0: {e}")
}

try {
  math.srand(1, 2)
} catch (e) {
  println($"caught2: {e}")
}

function useParam(math) {
  return math.abs(-5)
}
println(useParam({ abs = @(x) x + 100 }))

require("math").abs = require("math").clamp
println(math.abs(runtime(-5)))

println("OK")
