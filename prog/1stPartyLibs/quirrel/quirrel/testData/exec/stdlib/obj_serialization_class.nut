// Coverage: class (de)serialization paths in sqstdserialization.cpp -
// getCountOfParams (__setstate arity), isClassConstructorValid, and the
// deserialize error branches (missing class, missing __setstate, invalid
// constructor) that the existing serialization tests do not reach.

let { blob } = require("iostream")

// --- valid round-trip: __setstate with 1 param vs 2 params (availableClasses)
class Holder {
  v = null
  constructor(v = null) { this.v = v }
  function __getstate(_av) { return this.v }
  function __setstate(v) { this.v = v }
}
class Linked {
  data = null
  constructor(d = null) { this.data = d }
  function __getstate() { return this.data }
  function __setstate(d, _av) { this.data = d }   // 3 effective params -> passes availableClasses
}

let registry = { Holder = Holder, Linked = Linked }

let src = [Holder(42), Linked("chain"), Holder(7)]
let b = blob()
b.writeobject(src, registry)
b.seek(0)
let out = b.readobject(registry)
println($"roundtrip: {out[0].v} {out[1].data} {out[2].v}")   // 42 chain 7

// --- error: instance encountered but no available classes table ----------
function expectErr(label, fn) {
  try { fn(); println($"FAIL: {label}") }
  catch (e) { println($"err: {label}") }
}

let bi = blob()
bi.writeobject([Holder(1)], registry)
bi.seek(0)
expectErr("no-registry", function() { bi.readobject() })   // availableClasses not set

// --- error: class missing from the registry ------------------------------
let b2 = blob()
b2.writeobject(Holder(5), registry)
b2.seek(0)
expectErr("class-not-found", function() { b2.readobject({}) })

// --- error: class without __setstate -------------------------------------
class NoSetState {
  x = null
  constructor(x = null) { this.x = x }
  function __getstate(_av) { return this.x }
}
let b3 = blob()
b3.writeobject(NoSetState(9), { NoSetState = NoSetState })
b3.seek(0)
expectErr("missing-setstate", function() { b3.readobject({ NoSetState = NoSetState }) })

// --- error: constructor that cannot be called with one arg ---------------
class BadCtor {
  a = null; bb = null
  constructor(a, bb) { this.a = a; this.bb = bb }   // 2 required params, no defaults
  function __getstate(_av) { return this.a }
  function __setstate(v) { this.a = v }
}
let b4 = blob()
b4.writeobject(BadCtor(1, 2), { BadCtor = BadCtor })
b4.seek(0)
expectErr("invalid-ctor", function() { b4.readobject({ BadCtor = BadCtor }) })

// --- scalar / container type coverage round-trip -------------------------
let mixed = { i = -123, f = 3.5, t = true, n = null, s = "str", arr = [1, 2, [3]], tbl = { k = "v" } }
let bm = blob()
bm.writeobject(mixed)
bm.seek(0)
let rm = bm.readobject({})
println($"scalars: {rm.i} {rm.f} {rm.t} {rm.s} {rm.arr[2][0]} {rm.tbl.k}")  // -123 3.5 true str 3 v

println("serialization class: OK")
