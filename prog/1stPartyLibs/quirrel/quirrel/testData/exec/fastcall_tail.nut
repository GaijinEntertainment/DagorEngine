// `return abs(x)` as the last statement. The return-after-call tailcall fold
// only matches _OP_CALL, so a fastcall stays _OP_FASTCALL and returns normally.

from "math" import abs

function tailAbs(x) {
  return abs(x)
}

println(tailAbs(-9))
println(tailAbs(4))
println(tailAbs(-1.5))

// still works when tailAbs itself is the tail of another function
function wrap(x) {
  return tailAbs(x)
}
println(wrap(-42))

println("OK")
