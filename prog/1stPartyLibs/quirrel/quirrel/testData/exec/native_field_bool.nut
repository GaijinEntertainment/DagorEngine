from "test.native" import NativeVec

let v = NativeVec()
println($"default: {v.flag}")
v.flag = true
println($"set true: {v.flag} {type(v.flag)}")
let key = "flag"
println($"computed key: {v[key]}")
v.flag = 0
println($"set 0: {v.flag}")

class Holder {
  payload = null
}

function readAfterCounted() {
  let h = Holder()
  h.payload = {}
  let wr = h.payload.weakref()
  local p = h.payload
  let f = v.flag
  p = null
  h.payload = null
  return f == false && wr.ref() == null
}
println($"old value released: {readAfterCounted()}")

let frozenVec = freeze(NativeVec())

function readAfterFrozen() {
  let w = frozenVec.w
  let f = v.flag
  return w == 0 && getobjflags(f) == 0
}
println($"old flags cleared: {readAfterFrozen()}")
