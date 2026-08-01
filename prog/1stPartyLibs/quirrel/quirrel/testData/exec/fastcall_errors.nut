// _OP_FASTCALL error path: typemask error is raised, catchable, and leaves the
// caller's locals and stack intact (exercised under --check-stack).

from "math" import abs

local before = 111
try {
  abs("str")
  println("NOT REACHED")
} catch (e) {
  println($"caught: {e}")
}
local after = 222
println($"locals intact: {before} {after}")

// a second failing call after the first catch still behaves
try {
  println(abs({}))
} catch (e) {
  println($"caught2: {e}")
}

println("OK")
