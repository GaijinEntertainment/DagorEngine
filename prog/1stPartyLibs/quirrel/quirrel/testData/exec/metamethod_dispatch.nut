// Coverage: VM metamethod / fallback paths in sqvm.cpp
// (ArithMetaMethod, object CMP_OP, FallBackGet/FallBackSet, DeleteSlot, NewSlot,
//  CallMetaMethod, _typeof/_tostring/_cmp dispatch).
#allow-delete-operator

// Metamethods return scalars; the goal is to *invoke* each arithmetic/compare
// metamethod so the VM's ArithMetaMethod / object CMP_OP paths run.
class Vec {
  x = 0
  constructor(v) { this.x = v }
  function _add(o)   { return this.x + o.x }
  function _sub(o)   { return this.x - o.x }
  function _mul(o)   { return this.x * o.x }
  function _div(o)   { return this.x / o.x }
  function _modulo(o){ return this.x % o.x }
  function _unm()    { return -this.x }
  function _cmp(o)   { return this.x <=> o.x }
  function _tostring(){ return "Vec(" + this.x + ")" }
  function _typeof() { return "Vec" }
}

local a = Vec(10)
local b = Vec(3)
println(a + b)            // 13  -> ArithMetaMethod (+)
println(a - b)            // 7   -> ArithMetaMethod (-)
println(a * b)            // 30  -> ArithMetaMethod (*)
println(a / b)            // 3   -> ArithMetaMethod (/)
println(a % b)            // 1   -> ArithMetaMethod (%)
println(-a)               // -10 -> NEG metamethod
println(a > b)            // true   -> object CMP_OP via _cmp
println(a < b)            // false
println(a >= b)           // true
println(a <= b)           // false
println(a.tostring())     // Vec(10) -> _tostring
println(typeof a)         // Vec     -> _typeof

// Instance _get / _set / _newslot / _delslot metamethods drive FallBackGet,
// FallBackSet and DeleteSlot fallback paths (table setdelegate is C-API only).
class Proxy {
  store = null
  constructor() { this.store = {} }
  function _get(k)      { return ("v_" + k) in this.store ? this.store["v_" + k] : null }
  function _set(k, v)   { this.store["v_" + k] <- v }
  function _newslot(k, v) { this.store["v_" + k] <- v }
  function _delslot(k)  { delete this.store["v_" + k] }
}

local proxy = Proxy()
proxy.alpha <- 1          // _newslot  -> FallBackSet path
proxy.alpha = 2           // _set      -> FallBackSet path
println(proxy.alpha)      // 2         -> _get / FallBackGet
println(proxy.missing)    // (null)    -> _get returns null
delete proxy.alpha        // _delslot  -> DeleteSlot fallback
println(proxy.alpha)      // (null)

// NewSlot on a plain table via '<-' and delete
local plain = {}
plain.one <- 1            // NEWSLOT
plain["two"] <- 2         // NEWSLOTK
plain.three <- 3
delete plain.two          // DeleteSlot
println(plain.one + plain.three)  // 4
println("two" in plain)   // false

// _call metamethod
class Callable {
  function _call(ctx, n) { return n * n }
}
local c = Callable()
println(c(7))             // 49 -> CallMetaMethod (_call)
