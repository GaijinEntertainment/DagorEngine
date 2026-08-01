// Leaf stdlib natives from the blob (iostream), string and datetime modules
// marked `fastcall`: results through the _OP_FASTCALL mini-frame must match the
// ordinary native-call path.

from "iostream" import casti2f, castf2i, swap2, swap4, swapfloat
from "string" import strip, lstrip, rstrip, startswith, endswith, escape
from "datetime" import clock

// blob bit reinterpret / byteswap
println(castf2i(casti2f(0x3F800000)))   // round-trips the bit pattern
println(swap2(0x1234))
println(swap4(0x11223344))
println(castf2i(swapfloat(swapfloat(casti2f(0x40490FDB)))))

// string leaves
println("[" + strip("  hi  ") + "]")
println("[" + lstrip("  hi  ") + "]")
println("[" + rstrip("  hi  ") + "]")
println(startswith("hello", "he"))
println(endswith("hello", "lo"))
println(startswith("hi", "hello"))
println(escape("a\tb"))

// zero-arg leaf
println(clock() >= 0.0)
