// get_const_eval_pos gives the constant the compiler evaluates, and null at runtime
from "debug" import get_const_eval_pos

const function [pure] where(_tag = null) {
  let pos = get_const_eval_pos()
  return pos == null ? "runtime" : $"{pos.func}:{pos.line}"
}

const function [pure] box() {
  let Box = class {
    function _add(other) {
      return where(other)
    }
  }
  return Box()
}

const AT_CALL = where()
const PER_LINE = [
  where(1),
  where(2)
]
const BY_METAMETHOD = box() + box()

const function [pure] nested() {
  let inner = compilestring("const X = where()\n\nconst Y = where()\nreturn [X, Y]", "inner", { where })()
  return [inner[0], inner[1], where(inner)]
}
const NESTED = nested()

const function [pure] throwing() {
  throw "boom"
}

function f() {
  return const where()
}

// A call with an argument that is not a constant runs at runtime
let tag = null

println(AT_CALL)
println(PER_LINE[0], PER_LINE[1])
println(BY_METAMETHOD)
println(NESTED[0], NESTED[1], NESTED[2])
println(f())
println(where(tag))

try {
  compilestring("const E = throwing()", "failing", { throwing })
}
catch (e) {
  println(e)
}
println(where(tag))

let pos = compilestring("const P = get_pos()\nreturn P", "fields", { get_pos = @[pure]() get_const_eval_pos() })()
println(pos.func, pos.src, pos.line)
