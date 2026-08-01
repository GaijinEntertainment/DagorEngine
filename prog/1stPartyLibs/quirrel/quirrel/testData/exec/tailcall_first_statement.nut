// Tail calls must fire when `return f(...)` is the very first statement of a
// function. Covers the _OP_CALL -> _OP_TAILCALL fold for both a plain call
// and a zero-argument method call.

let dbg = require("debug")

const N = 100000

function reportDepth(tag) {
  local depth = 0
  while (dbg.getstackinfos(depth) != null)
    depth++
  println($"{tag}: flat stack = {depth < 10}")
  return 0
}

local rec = null
function deepCall(n) {
  return rec(n)
}
rec = function(n) {
  if (n <= 0)
    return reportDepth("plain call")
  return deepCall(n - 1)
}

local counter = N
local obj = null
function deepMethod() {
  return obj.tick()
}
obj = {
  function tick() {
    if (counter <= 0)
      return reportDepth("method call")
    counter = counter - 1
    return deepMethod()
  }
}

deepCall(N)
deepMethod()
println("OK")
