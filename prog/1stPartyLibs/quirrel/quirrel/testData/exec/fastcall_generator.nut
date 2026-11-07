// abs (a fastcall) invoked inside a generator across yield/resume points.
// The mini-frame must not disturb the generator's saved stack.

from "math" import abs

// Its result is not a constant, so a call with it runs as _OP_FASTCALL and is not folded
let runtime = @(v) v

function gen(start) {
  local acc = abs(start)
  yield acc
  acc += abs(runtime(-3))
  yield acc
  acc += abs(start - 10)
  return acc
}

local g = gen(-5)
println(resume g)   // abs(-5) = 5
println(resume g)   // + abs(-3) = 8
println(resume g)   // + abs(-15) = 23

println("OK")
