// A tail call to a fastcall native gives the arity errors of _OP_CALL.

let math = require("math") // not import: the calls stay tail calls

function tailSqrt0() { return math.sqrt() }
function tailSqrt2() { return math.sqrt(4, 9) }
function tailMin1() { return math.min(1) }
function tailSqrt(x) { return math.sqrt(x) }
function tailMin(x, y) { return math.min(x, y) }

foreach (f in [tailSqrt0, tailSqrt2, tailMin1]) {
  try {
    println(f())
  } catch (e) {
    println(e)
  }
}

println(tailSqrt(16))
println(tailSqrt(2.25))
println(tailMin(5, -2))
println("OK")
