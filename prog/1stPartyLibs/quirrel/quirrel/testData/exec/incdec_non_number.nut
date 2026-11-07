// `--x` and `x--` must act as `x -= 1` for a value that is not a number, and `++x` as `x += 1`:
// a string does not get "-1" appended, and an instance gets _sub(1), not _add(-1).

function result(f) {
  try {
    return f()
  }
  catch (e) {
    return $"error: {e}"
  }
}

println(result(function() { local s = "a"; s--; return s }))
println(result(function() { local s = "a"; return --s }))
println(result(function() { local s = "a"; return s-- }))
println(result(function() { local s = "a"; s -= 1; return s }))
println(result(function() { local s = "a"; s++; return s }))

println(result(function() { let t = { s = "a" }; t.s--; return t.s }))
println(result(function() { let t = { s = "a" }; return --t.s }))
println(result(function() { let arr = ["a"]; arr[0]--; return arr[0] }))
println(result(function() { let t = { s = "a" }; t.s++; return t.s }))
println(result(function() { let arr = ["a"]; ++arr[0]; return arr[0] }))

local outer = "a"
println(result(function() { outer--; return outer }))

class Counter {
  _sub = @(o) $"sub {o}"
  _add = @(o) $"add {o}"
}
println(result(function() { local c = Counter(); c--; return c }))
println(result(function() { local c = Counter(); return --c }))
println(result(function() { local c = Counter(); c++; return c }))
println(result(function() { let t = { c = Counter() }; t.c--; return t.c }))
println(result(function() { let t = { c = Counter() }; t.c++; return t.c }))

class OnlyAdd {
  _add = @(o) $"add {o}"
}
println(result(function() { local c = OnlyAdd(); c--; return c }))

class Holder {
  s = "a"
}
println(result(function() { let h = Holder(); h.s--; return h.s }))

local i = 5
local f = 2.5
let t = { i = 5, f = 2.5 }
let post = i--
let fpost = f--
--t.i
t.f--
println(i, post, f, fpost, t.i, t.f, --i, ++f, --f)
