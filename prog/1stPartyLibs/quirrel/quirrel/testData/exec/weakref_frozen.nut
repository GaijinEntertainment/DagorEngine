// A weakref gives back the flags of the handle it was made from, in any order

let C = class {
  v = 0
  function gen() {
    yield this.is_frozen()
  }
}

let p = C()
let pw = p.weakref()
let pfw = freeze(p).weakref()
println($"Instance, plain first: {pw.ref().is_frozen()} {pfw.ref().is_frozen()}")

let q = C()
let qfw = freeze(q).weakref()
let qw = q.weakref()
println($"Instance, frozen first: {qw.ref().is_frozen()} {qfw.ref().is_frozen()}")

let t = { v = 0 }
let tfw = freeze(t).weakref()
let tw = t.weakref()
println($"Table, frozen first: {tw.ref().is_frozen()} {tfw.ref().is_frozen()}")

println($"Same handle, same weakref: {p.weakref() == pw} {freeze(p).weakref() == pfw}")
println($"Plain and frozen weakrefs equal: {pw == pfw}")


function setV() {
  this.v = 1
}

let e = { v = 0 }
let plainEnv = setV.bindenv(e)
let frozenEnv = setV.bindenv(freeze(e))
try {
  frozenEnv()
  println("bindenv frozen: written")
} catch (err) {
  println("bindenv frozen: rejected")
}
plainEnv()
println($"bindenv plain: {e.v}")


let g = C()
let _gw = g.weakref()
let gen = freeze(g).gen()
println($"Generator this: {resume gen}")


// Release the weakrefs in both orders, the other one must still work
let r = C()
local _rw = r.weakref()
let rfw = freeze(r).weakref()
_rw = null
println($"Release plain first: {rfw.ref().is_frozen()} {r.weakref().ref().is_frozen()}")

let s = C()
local _sfw = freeze(s).weakref()
let sw = s.weakref()
_sfw = null
println($"Release frozen first: {sw.ref().is_frozen()} {freeze(s).weakref().ref().is_frozen()}")


function deadRefs(cb) {
  local o = C()
  let ow = o.weakref()
  let ofw = freeze(o).weakref()
  o = null
  cb(ow, ofw)
}
deadRefs(@(ow, ofw) println($"Dead object: {ow.ref()} {ofw.ref()}"))
