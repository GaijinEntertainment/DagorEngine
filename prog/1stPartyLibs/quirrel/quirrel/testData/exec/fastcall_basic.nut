// _OP_FASTCALL basic behavior: abs on int/float/negative/zero, result types.

from "math" import abs

// Its result is not a constant, so a call with it runs as _OP_FASTCALL and is not folded
let runtime = @(v) v

println(abs(runtime(5)))
println(abs(runtime(-5)))
println(abs(runtime(0)))
println(abs(runtime(-2.5)))
println(abs(runtime(2.5)))
println(abs(runtime(-0.0)))

// result type is preserved (int stays int, float stays float)
println(typeof abs(runtime(-5)))
println(typeof abs(runtime(-2.5)))

// nested / used as a subexpression
println(abs(runtime(-3)) + abs(runtime(-4)))
local x = -7
println(abs(x) * 2)

println("OK")
