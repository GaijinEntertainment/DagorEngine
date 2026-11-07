// A field hint of a freed class must not match a new class at its address.

function setX(o, v) { o.x = v }
function getX(o) { return o.x }
function addX(o, v) { o.x += v }

function useOldClass() {
  let A = class { y = 0; x = 0 }
  let a = A()
  setX(a, 1)
  getX(a)
  addX(a, 1)
  return A.tostring()
}

let oldAddr = useOldClass()
let live = []
local b = null
for (local i = 0; i < 1000 && b == null; i++) {
  let B = class { x = 0; y = 0 }
  live.append(B())
  if (B.tostring() == oldAddr)
    b = live.top()
}
assert(b != null, "no new class at the address of the freed class")

setX(b, 7)
println($"after set: x={b.x} y={b.y}")
println($"get: {getX(b)}")
addX(b, 2)
println($"after add: x={b.x} y={b.y}")
println("OK")
