// Coverage for every fastcall-marked math stdlib function: one or two calls
// each, plus the clamp error branch and a mixed-type variadic min/max.

from "math" import sqrt, fabs, sin, cos, asin, acos, log, log10, tan, atan,
  atan2, pow, floor, ceil, round, exp, abs, min, max, clamp

println(sqrt(16.0))
println(fabs(-3.5))
println(floor(3.7))
println(ceil(3.2))
println(round(3.5))
println(pow(2.0, 10.0))
println(log10(1000.0))
println(abs(-8))

// trig / exp: just check they run and return floats
println(typeof sin(0.0))
println(sin(0.0))
println(cos(0.0))
println(typeof tan(0.0))
println(typeof atan(1.0))
println(typeof atan2(1.0, 1.0))
println(asin(0.0))
println(acos(1.0))
println(typeof asin(0.5))
println(asin(2.0))   // input clamped to 1.0 internally
println(acos(-3.0))  // input clamped to -1.0 internally
println(typeof log(1.0))
println(exp(0.0))

// variadic min/max, mixed int/float
println(min(1, 2.5, -3, 4))
println(max(1, 2.5, -3, 4))
println(min(5, 2))
println(max(5, 2))

// clamp: in range, below, above
println(clamp(5, 0, 10))
println(clamp(-1, 0, 10))
println(clamp(99, 0, 10))

// clamp error branch: min > max
try {
  clamp(5, 10, 0)
} catch (e) {
  println($"clamp err: {e}")
}

println("OK")
