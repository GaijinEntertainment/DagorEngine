// abs (a fastcall) invoked inside a generator across yield/resume points.
// The mini-frame must not disturb the generator's saved stack.

from "math" import abs

function gen(start) {
  local acc = abs(start)
  yield acc
  acc += abs(-3)
  yield acc
  acc += abs(start - 10)
  return acc
}

local g = gen(-5)
println(resume g)   // abs(-5) = 5
println(resume g)   // + abs(-3) = 8
println(resume g)   // + abs(-15) = 23

println("OK")
