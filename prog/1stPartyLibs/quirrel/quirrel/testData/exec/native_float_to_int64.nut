// A12: a 64-bit int parameter fed a float must keep full width; sqrat getAsInt
// must not route the float through a 32-bit int. Needs a native bound function.

from "test.native" import identity_i64

println($"i64 from float: {identity_i64(3000000000.0)}")
println($"i64 small: {identity_i64(100.0)}")

println("PASSED")
