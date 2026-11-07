// `x - k` with an int literal k other than 0 and INT_MIN compiles to _OP_ADDI with -k.
// For a value that is not a number it must still act as `x - k`, not as `x + (-k)`.

let one = 1

function result(f) {
  try {
    return f()
  }
  catch (e) {
    return $"error: {e}"
  }
}

local s = "extra"
println(result(@() s - 1))
println(result(@() s - one))
println(result(@() s - -5))
println(result(@() s + 1))

local arr = []
println(result(@() arr - 1))

class WithSub {
  _sub = @(o) $"sub {o}"
  _add = @(o) $"add {o}"
}
let inst = WithSub()
println(result(@() inst - 1))
println(result(@() inst - one))
println(result(@() inst + 1))

class OnlySub {
  _sub = @(o) $"sub {o}"
}
let onlySub = OnlySub()
println(result(@() onlySub - 2))

local n = null
println(result(@() n - 1))
println(result(@() n - one))

local i = 7
local f = 2.5
println(i - 1, i - -3, i - 2147483647, f - 1, f - -1)

local nz = -0.0
let zero = 0
println(nz - 0, nz - zero)
