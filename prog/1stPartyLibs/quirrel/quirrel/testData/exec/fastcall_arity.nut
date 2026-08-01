// Wrong-arity calls do not match the fastcall arity rule at compile time, so
// they fall back to _OP_CALL and produce the same runtime errors as master.

from "math" import abs

try {
  abs()
} catch (e) {
  println($"caught0: {e}")
}

try {
  abs(1, 2)
} catch (e) {
  println($"caught2: {e}")
}

println("OK")
