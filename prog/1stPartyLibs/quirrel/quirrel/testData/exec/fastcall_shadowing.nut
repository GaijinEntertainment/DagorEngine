// A parameter named `abs` shadows the imported constant, so the compiler
// resolves it as a local first and emits a plain _OP_CALL, not a fastcall.
// (Shadowing an import with a `local` in an inner block is a compile error in
// Quirrel, so a function parameter is the way to express the shadowing case.)

from "math" import abs

function useParam(abs) {
  return abs(5)          // param shadows the import -> plain call
}
println(useParam(@(x) x + 100))  // -> 105

// the import is still the fastcall native at file scope
println(abs(-5))         // -> 5

println("OK")
