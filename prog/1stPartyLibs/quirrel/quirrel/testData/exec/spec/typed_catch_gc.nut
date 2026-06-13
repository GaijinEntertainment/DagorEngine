// The catch type class is captured into the trap at try entry, then its only
// other reference is dropped and a GC is forced mid-try. The trap must keep the
// class reachable (GC mark of _etraps); otherwise the IsInstanceOf check on the
// propagating value would touch a collected class.
let dbg = require("debug")

function f() {
  local CatchType = class {}
  local other = (class {})()
  local result = "?"
  try {
    CatchType = null
    dbg.collectgarbage()
    throw other
  }
  catch (CatchType e) { result = "BUG matched" }
  catch (e) { result = "propagated" }
  return result
}
println(f())
