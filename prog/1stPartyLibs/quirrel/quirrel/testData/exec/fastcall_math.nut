// Coverage for every fastcall-marked math stdlib function: one or two calls
// each, plus the clamp error branch and a mixed-type variadic min/max.

from "math" import sqrt, fabs, sin, cos, asin, acos, log, log10, tan, atan,
  atan2, pow, floor, ceil, round, exp, abs, min, max, clamp

// Its result is not a constant, so a call with it runs as _OP_FASTCALL and is not folded
let runtime = @(v) v

println(sqrt(runtime(16.0)))
println(fabs(runtime(-3.5)))
println(floor(runtime(3.7)))
println(ceil(runtime(3.2)))
println(round(runtime(3.5)))
println(pow(runtime(2.0), 10.0))
println(log10(runtime(1000.0)))
println(abs(runtime(-8)))

// trig / exp: just check they run and return floats
println(typeof sin(runtime(0.0)))
println(sin(runtime(0.0)))
println(cos(runtime(0.0)))
println(typeof tan(runtime(0.0)))
println(typeof atan(runtime(1.0)))
println(typeof atan2(runtime(1.0), 1.0))
println(asin(runtime(0.0)))
println(acos(runtime(1.0)))
println(typeof asin(runtime(0.5)))
println(asin(runtime(2.0)))   // input clamped to 1.0 internally
println(acos(runtime(-3.0)))  // input clamped to -1.0 internally
println(typeof log(runtime(1.0)))
println(exp(runtime(0.0)))

// variadic min/max, mixed int/float
println(min(runtime(1), 2.5, -3, 4))
println(max(runtime(1), 2.5, -3, 4))
println(min(runtime(5), 2))
println(max(runtime(5), 2))

// clamp: in range, below, above
println(clamp(runtime(5), 0, 10))
println(clamp(runtime(-1), 0, 10))
println(clamp(runtime(99), 0, 10))

// clamp error branch: min > max
try {
  clamp(runtime(5), 10, 0)
} catch (e) {
  println($"clamp err: {e}")
}

println("OK")
