// Wrong-arity calls do not match the fastcall arity rule at compile time, so
// they fall back to _OP_CALL and produce the same runtime errors as master.
// srand is not pure, so its calls with constant arguments are not folded.

from "math" import srand

try {
  srand()
} catch (e) {
  println($"caught0: {e}")
}

try {
  srand(1, 2)
} catch (e) {
  println($"caught2: {e}")
}

println("OK")
