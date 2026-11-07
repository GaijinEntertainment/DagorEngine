// `k + x` with an int literal k must act as `k + x`, not as `x + k`:
// for a value that is not a number the order of the operands matters.

let one = 1

function result(f) {
  try {
    return f()
  }
  catch (e) {
    return $"error: {e}"
  }
}

local s = "s"
println(result(@() 1 + s))
println(result(@() one + s))
println(result(@() -7 + s))
println(result(@() s + 1))

class WithAdd {
  _add = @(o) $"add {o}"
}
let inst = WithAdd()
println(result(@() 1 + inst))
println(result(@() one + inst))
println(result(@() inst + 1))

local arr = []
println(result(@() 1 + arr))
println(result(@() one + arr))

local n = null
println(result(@() 1 + n))
println(result(@() one + n))

local i = 7
local f = 2.5
println(1 + i, -3 + i, 2147483647 + i, 1 + f, -1 + f)
