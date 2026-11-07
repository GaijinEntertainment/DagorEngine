// The comparator shapes of array.sort and sort_reversed: typed and bound closures, wrong parameter counts, varargs, a generator,
// a native closure, errors thrown or returned, a nested sort, deep recursion, a resize, a debug hook.
// The output must not depend on how the comparator is called: each case runs again under a debug hook, which takes the
// old call, and prints the second result only when it differs.
from "debug" import setdebughook
from "math" import max

function run(f, hooked) {
  let a = [5, 3, 9, 1, 7, 3, 8, 2, 6, 4, 0, 11, 10]
  if (hooked)
    setdebughook(@(_event, _src, _line, _fname) null)
  local text
  try {
    f(a)
    text = $"ok {",".join(a.map(@(v) v.tostring()))}"
  } catch (e) {
    text = $"error: {e}"
  }
  setdebughook(null)
  return text
}

function show(name, f) {
  let text = run(f, false)
  let hooked = run(f, true)
  println(text == hooked ? $"{name}: {text}" : $"{name}: {text}; under a debug hook: {hooked}")
}

show("plain", @(a) a.sort(@(x, y) x <=> y))
show("typed params", @(a) a.sort(function(x: int, y: int) { return x <=> y }))
show("typed params, wrong type", function(a) { a[2] = "s"; a.sort(function(x: int, y: int) { return x <=> y }) })
show("typed params, wrong second", function(a) { a[0] = "s"; a.sort(function(x: int, y: int) { return x <=> y }) })
show("bound env", function(a) {
  let obj = { dir = -1 }
  a.sort(function(x, y) { return this.dir * (x <=> y) }.bindenv(obj))
})
show("default param", @(a) a.sort(function(x, y, sign = -1) { return sign * (x <=> y) }))
show("default param, last", @(a) a.sort(function(x, y = 0) { return x <=> y }))
show("one param", @(a) a.sort(@(_x) 0))
show("three params", @(a) a.sort(@(x, y, _z) x <=> y))
show("vararg", @(a) a.sort(function(...) { return vargv[0] <=> vargv[1] }))
show("vararg with a named param", @(a) a.sort(function(x, ...) { return x <=> vargv[0] }))
show("generator", @(a) a.sort(function(_x, _y) { yield 1; return 0 }))
show("throws", @(a) a.sort(function(_x, _y) { throw "boom" }))
show("throws null", @(a) a.sort(function(_x, _y) { throw null }))
show("returns string", @(a) a.sort(@(_x, _y) "no"))
show("returns float", @(a) a.sort(@(x, y) (x <=> y) * 1.5))
show("nested sort", @(a) a.sort(function(x, y) { let b = [3, 1, 2]; b.sort(@(p, q) p <=> q); return x <=> y }))
show("deep recursion in comparator", function(a) {
  function depth(n) { return n == 0 ? 0 : 1 + depth(n - 1) }
  a.sort(@(x, y) (x <=> y) + 0 * depth(200))
})
show("resize", @(a) a.sort(function(x, y) { a.resize(20); return x <=> y }))
show("closure with outers", function(a) {
  let sign = 1
  local calls = 0
  a.sort(function(x, y) { calls++; return sign * (x <=> y) })
  println($"  called: {calls > 0}")
})
show("native closure, sorted again", function(a) { a.sort(max); a.sort() })
show("not callable", @(a) a.sort(5))
show("under a debug hook", function(a) {
  local calls = 0
  setdebughook(function(event, _src, _line, fname) { if (event == 'c' && fname == "cmp") calls++ })
  a.sort(function cmp(x, y) { return x <=> y })
  setdebughook(null)
  println($"  hook saw the calls: {calls > 0}")
})
show("comparator sets a debug hook", function(a) {
  local calls = 0
  let hook = function(event, _src, _line, fname) { if (event == 'c' && fname == "cmp") calls++ }
  a.sort(function cmp(x, y) { setdebughook(hook); return x <=> y })
  setdebughook(null)
  println($"  hook saw the later calls: {calls > 0}")
})
show("reversed", @(a) a.sort_reversed(@(x, y) x <=> y))
show("reversed, typed params, wrong type", function(a) { a[2] = "s"; a.sort_reversed(function(x: int, y: int) { return x <=> y }) })
show("reversed, one param", @(a) a.sort_reversed(@(_x) 0))
show("reversed, throws", @(a) a.sort_reversed(function(_x, _y) { throw "boom" }))

local nest = null
local deepest = 0
nest = function(n) {
  [2, 1].sort(function(x, y) {
    deepest = 200 - n
    if (n > 0)
      nest(n - 1)
    return x <=> y
  })
}
try {
  nest(200)
} catch (e)
  println($"nested sorts: {e} at depth {deepest}")
