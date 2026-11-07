// A pure call with constant arguments folds in any expression; a fold keeps only a scalar result.
// where() gives the line that the compiler folds it at, or "runtime".
from "debug" import get_const_eval_pos
from "math" import sqrt

const function [pure] where(_tag = null) {
  let pos = get_const_eval_pos()
  return pos == null ? "runtime" : $"{pos.line}"
}

const function [pure] box() {
  return { at = where(get_const_eval_pos()) }
}

const function notPure() {
  let pos = get_const_eval_pos()
  return pos == null ? "runtime" : $"{pos.line}"
}

const LIB = { where }

function inBody() {
  return where()
}

function withDefault(at = where()) {
  return at
}

function shadowed(where) {
  return where()
}

function shadowedInClosure(where) {
  let inner = @() @() where()
  return inner()()
}

let tag = null

println("body:", inBody())
println("default value:", withDefault())
println("argument:", where())
println("constant expression argument:", where(1 + 2))
println("member of a constant:", LIB.where())
println("slot of a constant:", LIB["where"]())
println("operand of ??:", tag ?? where())
println("scalar member of a container:", box().at)
let boxed = box()
println("container:", boxed.at)
boxed.at = "changed"
println("container is not frozen:", boxed.at)
println("not pure:", notPure())
println("shadowed:", shadowed(where))
println("shadowed in a closure:", shadowedInClosure(where))
println("nullable:", where?())
println("argument that is not a constant:", where(tag))

foreach (src in ["return sqrt(\"a\")", "function never() { return sqrt(\"a\") }", "return \"abc\".slice(\"1\")",
    "const IS_DEV = false\nif (IS_DEV) sqrt(\"a\")", "return false && sqrt(\"a\")",
    "const IS_DEV = false\nreturn IS_DEV ? sqrt(\"a\") : null"]) {
  try {
    compilestring(src, "failing", { sqrt })
    println("compiled")
  }
  catch (e) {
    println(e)
  }
}

// A call in the body of a pure function folds when that body compiles, at its own line
const function [pure] whereInPure() {
  return where()
}
println("call in a pure function:", whereInPure())

// A callee that is not found gives no compile error: the call stays a runtime call
try { LIB["missing"]() } catch (e) println("missing slot:", e)
try { "abc".nosuch() } catch (e) println("missing type method:", e)

// The compiler does not run the _get of a constant object to find a callee, only the runtime does
const function [pure] withGet() {
  let Dyn = class {
    function _get(key) {
      println($"_get({key})")
      throw null
    }
  }
  return Dyn()
}
const DYN = withGet()
try { DYN.dyn(tag) } catch (e) println("callee from _get:", e)
try { DYN.dyn(1) } catch (e) println("callee from _get:", e)

// A call through a slot or a field gets its table as this, as at runtime
const function [pure] getX() { return this.x }
const OBJ = { x = 7, getX }
println("this of a slot call:", OBJ["getX"]())
println("this of a field call:", OBJ.getX())

// The own name of a function that is also a constant is a compile error, also as a folded argument
const function [pure] typeName(x) { return typeof x }
try {
  compilestring("const own = 7\nlet named = function own() { return typeName(own) }", "own", { typeName })
  println("own name: compiled")
} catch (e) println("own name:", e)

// The compiler runs a call to get its result, also when it then keeps the call for runtime
const function [pure] noisyBox() {
  println("noisyBox runs")
  return {}
}
let noisy = compilestring("return noisyBox()", "noisy", { noisyBox })
println("compiled")
noisy()
