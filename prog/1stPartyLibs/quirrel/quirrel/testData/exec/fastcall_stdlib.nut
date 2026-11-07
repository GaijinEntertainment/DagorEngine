// Leaf stdlib natives from the blob (iostream), string and datetime modules
// marked `fastcall`: results through the _OP_FASTCALL mini-frame must match the
// ordinary native-call path.

from "iostream" import casti2f, castf2i, swap2, swap4, swapfloat
from "string" import strip, lstrip, rstrip, startswith, endswith, escape
from "datetime" import clock

// Its result is not a constant, so a call with it is not folded and stays a run-time call
let runtime = @(v) v

// blob bit reinterpret / byteswap
println(castf2i(casti2f(runtime(0x3F800000))))   // round-trips the bit pattern
println(swap2(runtime(0x1234)))
println(swap4(runtime(0x11223344)))
println(castf2i(swapfloat(swapfloat(casti2f(runtime(0x40490FDB))))))

// string leaves
println($"[{strip(runtime("  hi  "))}]")
println($"[{lstrip(runtime("  hi  "))}]")
println($"[{rstrip(runtime("  hi  "))}]")
println(startswith(runtime("hello"), "he"))
println(endswith(runtime("hello"), "lo"))
println(startswith(runtime("hi"), "hello"))
println(escape(runtime("a\tb")))

// zero-arg leaf
println(clock() >= 0.0)
