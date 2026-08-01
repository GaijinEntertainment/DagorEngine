// _OP_FASTCALL basic behavior: abs on int/float/negative/zero, result types.

from "math" import abs

println(abs(5))
println(abs(-5))
println(abs(0))
println(abs(-2.5))
println(abs(2.5))
println(abs(-0.0))

// result type is preserved (int stays int, float stays float)
println(typeof abs(-5))
println(typeof abs(-2.5))

// nested / used as a subexpression
println(abs(-3) + abs(-4))
local x = -7
println(abs(x) * 2)

println("OK")
