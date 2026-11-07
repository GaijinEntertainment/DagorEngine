// The callback shapes of the array map, apply, filter, reduce and sortby, and of the table map, filter and reduce:
// parameter counts, defaults, varargs, types, a bound env, errors, a native closure, nesting, deep recursion, a
// callback that changes the container.
// each, findindex and findvalue: parameter counts, a table, errors, a callback that changes the array.
// The string join filter: parameter counts, 'this', a bound env, errors, a native closure, a callback that grows the array.
// The output must not depend on how the callback is called: each case runs again under a debug hook, which takes the
// old call, and prints the second result only when it differs.
from "debug" import setdebughook, seterrorhandler

function run(f, hooked) {
  if (hooked)
    setdebughook(@(_event, _src, _line, _fname) null)
  local text
  try {
    local r = f()
    if (typeof r == "table") {
      let keys = r.keys()
      keys.sort()
      r = keys.map(@(k) $"{k}={r[k]}")
    }
    text = $"ok {typeof r == "array" ? ",".join(r.map(@(v) $"{v}")) : r}"
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

let a = [5, 3, 9, 1, 7]
let env = { k = 3 }
show("map 1", @() a.map(@(v) v * 2))
show("map 2", @() a.map(@(v, i) v + i * 100))
show("map 3", @() a.map(@(v, _i, arr) v + arr.len()))
show("map 0 params", @() a.map(@() 42))
show("map 5 params", @() a.map(@(v, _i, _arr, _x, _y) v))
show("map default param", @() a.map(function(v, k = 7) { return v + k }))
show("map vararg", @() a.map(function(...) { return vargv.len() }))
show("map generator", @() a.map(function(v) { yield v }).map(@(g) typeof g))
show("map typed", @() a.map(function(v: int) { return v + 1 }))
show("map typed wrong", @() ["s", 1].map(function(v: int) { return v + 1 }))
show("map bound env", @() a.map(function(v) { return v * this.k }.bindenv(env)))
show("map throws", @() a.map(function(v) { if (v == 9) throw "nine"; return v }))
show("map throws null skips", @() a.map(function(v) { if (v == 9) throw null; return v }))
show("map native", @() a.map(getroottable()["type"]))
show("map nested", @() a.map(@(v) [v, v].map(@(x) x + 1).len()))
show("apply", function() { let b = clone a; b.apply(@(v) v - 1); return b })
show("filter 1", @() a.filter(@(v) v > 4))
show("filter 2", @() a.filter(@(_v, i) i % 2 == 0))
show("filter throws", @() a.filter(function(_v) { throw "f" }))
show("reduce 2", @() a.reduce(@(x, y) x + y))
show("reduce init", @() a.reduce(@(x, y) x + y, 100))
show("reduce 4", @() a.reduce(@(x, y, i, arr) x + y * i + arr.len(), 0))
show("reduce 1 param", @() a.reduce(@(x) x))
show("reduce empty", @() [].reduce(@(x, y) x + y))
show("reduce throws", @() a.reduce(function(_x, _y) { throw "r" }))
show("sortby 1", @() (clone a).sortby(@(v) -v))
show("sortby 2", @() (clone a).sortby(@(_v, i) -i))
show("sortby 3", @() (clone a).sortby(@(v, _i, arr) v % arr.len()))
show("sortby typed wrong", @() ["s", 1].sortby(function(v: int) { return v }))
show("sortby throws", @() (clone a).sortby(function(_v) { throw "s" }))
show("sortby_reversed", @() (clone a).sortby_reversed(@(v) v))
show("deep callback", @() a.map(function(v) { function d(n) { return n == 0 ? 0 : 1 + d(n - 1) }; return d(300) + v }))
show("callback grows the array", function() { let b = [1, 2, 3]; return b.map(function(v) { b.append(v); return v }) })
show("callback shrinks the array", function() { let b = [1, 2, 3, 4]; return b.map(function(v) { if (b.len() > 2) b.pop(); return v }) })
show("callback shrinks during reduce", function() { let b = [1, 2, 3, 4]; return b.reduce(function(x, y) { if (b.len() > 2) b.pop(); return x + y }) })
show("callback shrinks during filter", function() { let b = [1, 2, 3, 4]; return b.filter(function(_v) { if (b.len() > 2) b.pop(); return true }) })
show("callback error reaches the error handler", function() {
  local seen = null
  seterrorhandler(function(err) { seen = err })
  let co = newthread(@() a.map(function(v) { if (v == 9) throw "nine"; return v }))
  local caught = null
  try {
    co.call()
  } catch (e)
    caught = e
  seterrorhandler(null)
  return $"handler {seen}, caller {caught}"
})
let hookedMap = run(function() {
  local calls = 0
  setdebughook(function(event, _src, _line, fname) { if (event == 'c' && fname == "twice") calls++ })
  let r = a.map(function twice(v) { return v * 2 })
  setdebughook(null)
  println($"  hook saw each call: {calls == a.len()}")
  return r
}, false)
println($"under a debug hook: {hookedMap}")

show("each 1", function() { let acc = []; a.each(@(v) acc.append(v)); return acc })
show("each 2", function() { let acc = []; a.each(@(v, i) acc.append(v + i)); return acc })
show("each table", function() { let acc = []; let t = { x = 1, y = 2 }; t.each(@(v, k) acc.append($"{k}{v}")); acc.sort(); return acc })
show("each throws", @() a.each(function(_v) { throw "e" }))
show("findindex hit", @() a.findindex(@(v) v == 9))
show("findindex miss", @() a.findindex(@(v) v == 10))
show("findindex 2", @() a.findindex(@(_v, i) i == 3))
show("findindex table", function() { let t = { x = 1, y = 2 }; return t.findindex(@(v) v == 2) })
show("findindex throws", @() a.findindex(function(_v) { throw "fi" }))
show("each sees an appended element", function() { let b = [1, 2]; local n = 0; b.each(function(_v) { if (b.len() < 4) b.append(9); n++ }); return n })
show("findindex sees an appended element", function() { let b = [1, 2]; return b.findindex(function(v) { if (b.len() < 4) b.append(9); return v == 9 }) })
show("callback resizes during each", function() { let b = [1, 2, 3]; local n = 0; b.each(function(_v) { n++; if (n == 1) b.resize(1) }); return n })
let t = { a = 5, b = 3, c = 9, d = 1 }
show("findvalue array hit", @() [5, 3, 9].findvalue(@(v) v > 4))
show("findvalue array miss", @() [5, 3, 9].findvalue(@(v) v > 40))
show("findvalue array default", @() [5, 3, 9].findvalue(@(v) v > 40, -1))
show("findvalue array 2", @() [5, 3, 9].findvalue(@(_v, i) i == 2))
show("findvalue array 3", @() [5, 3, 9].findvalue(@(v, _i, arr) v == arr.len() + 6))
show("findvalue table", @() t.findvalue(@(v) v == 9))
show("findvalue table 3", @() t.findvalue(@(v, _k, tbl) v == tbl.len() + 5))
show("findvalue throws", @() t.findvalue(function(_v) { throw "fv" }))

show("table map 1", @() t.map(@(v) v * 2))
show("table map 2", @() t.map(@(v, k) $"{k}{v}"))
show("table map 3", @() t.map(@(v, _k, tbl) v + tbl.len()))
show("table map 0 params", @() t.map(@() 42))
show("table map 5 params", @() t.map(@(v, _k, _tbl, _x, _y) v))
show("table map default param", @() t.map(function(v, k = 7) { return $"{v}{k}" }))
show("table map vararg", @() t.map(function(...) { return vargv.len() }))
show("table map generator", @() t.map(function(v) { yield v }).map(@(g) typeof g))
show("table map typed", @() t.map(function(v: int) { return v + 1 }))
show("table map typed wrong", @() { s = "s" }.map(function(v: int) { return v + 1 }))
show("table map bound env", @() t.map(function(v) { return v * this.k }.bindenv(env)))
show("table map throws", @() t.map(function(v) { if (v == 9) throw "nine"; return v }))
show("table map throws null skips", @() t.map(function(v) { if (v == 9) throw null; return v }))
show("table map native", @() t.map(getroottable()["type"]))
show("table filter 1", @() t.filter(@(v) v > 4))
show("table filter 2", @() t.filter(@(_v, k) k != "a"))
show("table filter throws", @() t.filter(function(_v) { throw "f" }))
show("table reduce 2", @() t.reduce(@(x, y) x + y))
show("table reduce init", @() t.reduce(@(x, y) x + y, 100))
show("table reduce 4", @() t.reduce(@(x, y, k, tbl) x + y + k.len() + tbl.len(), 0))
show("table reduce 1 param", @() t.reduce(@(x) x, 0))
show("table reduce empty", @() {}.reduce(@(x, y) x + y))
show("table reduce throws", @() t.reduce(function(_x, _y) { throw "r" }))
show("callback grows the table", function() { let u = { x = 1 }; u.map(function(v) { u.y <- 2; return v }); return u })
let hookedFilter = run(function() {
  local calls = 0
  setdebughook(function(event, _src, _line, fname) { if (event == 'c' && fname == "pick") calls++ })
  let r = t.filter(function pick(v) { return v > 4 })
  setdebughook(null)
  println($"  hook saw each call: {calls == t.len()}")
  return r
}, false)
println($"table filter under a debug hook: {hookedFilter}")

show("join filter", @() ",".join(["a", "", "b", null, "c"], @(s) s != "" && s != null))
show("join filter this", @() ",".join(["a", "b"], function(s) { return this == "," && s == "a" }))
show("join filter native", @() ",".join(["a", "b"], getroottable()["type"]))
show("join filter 2 params", @() ",".join(["a", "b"], @(_s, i) i))
show("join filter 0 params", @() ",".join(["a", "b"], @() true))
show("join filter bound env", @() ",".join(["a", "b"], function(s) { return this.k == 3 && s == "a" }.bindenv(env)))
show("join filter throws", @() ",".join(["a", "b"], function(_s) { throw "j" }))
show("join filter grows the array", function() { let b = [1, 2]; return ",".join(b, function(_s) { if (b.len() < 6) b.append(3); return true }) })
