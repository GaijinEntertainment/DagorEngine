// each, findindex and findvalue of arrays and tables: what the callback gets, which elements it visits when it
// changes the container, weak references, frozen containers, a collection during the callback, errors.
from "debug" import collectgarbage

function show(name, f) {
  local text
  try {
    let r = f()
    text = $"ok {typeof r == "array" ? ",".join(r.map(@(v) $"{v}")) : r}"
  } catch (e) {
    text = $"error: {e}"
  }
  println($"{name}: {text}")
}

let a = [5, 3, 9, 1, 7]
let t = { x = 1, y = 2 }
local acc
show("each 0 params", function() { local n = 0; a.each(function() { n++ }); return n })
show("each 1", function() { acc = []; a.each(@(v) acc.append(v)); return acc })
show("each 2", function() { acc = []; a.each(@(v, i) acc.append(v + i)); return acc })
show("each 3", function() { acc = []; a.each(@(v, _i, arr) acc.append(v + arr.len())); return acc })
show("each table", function() { acc = []; t.each(@(v, k) acc.append($"{k}{v}")); acc.sort(); return acc })
show("each throws", @() a.each(function(_v) { throw "e" }))
show("findindex hit", @() a.findindex(@(v) v == 9))
show("findindex miss", @() a.findindex(@(v) v == 10))
show("findindex 2", @() a.findindex(@(_v, i) i == 3))
show("findindex table", @() t.findindex(@(v) v == 2))
show("findindex throws", @() a.findindex(function(_v) { throw "fi" }))
show("findvalue hit", @() a.findvalue(@(v) v > 6))
show("findvalue miss", @() a.findvalue(@(v) v > 50))
show("findvalue default", @() a.findvalue(@(v) v > 50, "none"))
show("findvalue table", @() t.findvalue(@(v) v == 2))
show("findvalue too many args", @() a.findvalue(@(v) v > 50, 1, 2))
show("each sees an appended element", function() { let b = [1, 2]; local n = 0; b.each(function(_v) { if (b.len() < 4) b.append(9); n++ }); return n })
show("findindex sees an appended element", function() { let b = [1, 2]; return b.findindex(function(v) { if (b.len() < 4) b.append(9); return v == 9 }) })
show("findvalue sees an appended element", function() { let b = [1, 2]; return b.findvalue(function(v) { if (b.len() < 4) b.append(9); return v == 9 }) })
show("each stops at a new end", function() { let b = [1, 2, 3]; local n = 0; b.each(function(_v) { n++; if (n == 1) b.resize(1) }); return n })
show("findvalue gives the value it passed", function() { let b = [1, 2, 3]; return b.findvalue(function(v) { b[0] = 99; return v == 1 }) })
show("each passes the object of a weakref", function() { let t = { x = 1 }; local s = ""; [t.weakref()].each(@(v) s = typeof v); return s })
show("findvalue of a weakref", function() { let t = { x = 1 }; return [t.weakref()].findvalue(@(v) v.x == 1).x })
show("findindex of a weakref", function() { let t = { x = 1 }; return [5, t.weakref()].findindex(@(v) typeof v == "table") })
show("findindex of a weakref whose object the callback drops", function() {
  let b = (function() { let t = {}; return [t, t.weakref()] })()
  return b.findindex(function(v, i) { if (i == 0) b[0] = null; return v == null })
})
let frozenArr = freeze([{ x = 1 }, [2]])
show("each of a frozen array", function() { acc = []; frozenArr.each(@(v) acc.append(v.is_frozen())); return acc })
show("each index of a frozen array", function() { acc = []; frozenArr.each(@(_v, i) acc.append((getobjflags(i) & SQOBJ_FLAG_IMMUTABLE) != 0)); return acc })
show("each index of an array that is not frozen", function() { acc = []; a.each(@(_v, i) acc.append((getobjflags(i) & SQOBJ_FLAG_IMMUTABLE) != 0)); return acc })
show("findindex result of a frozen array", @() (getobjflags(frozenArr.findindex(@(v) typeof v == "array")) & SQOBJ_FLAG_IMMUTABLE) != 0)
show("findindex of a frozen array", @() frozenArr.findindex(@(v) v.is_frozen() && typeof v == "array"))
show("findvalue of a frozen array", @() frozenArr.findvalue(@(v) typeof v == "table").is_frozen())
show("findvalue of an array that is not frozen", @() [{}].findvalue(@(_v) true).is_frozen())
show("findvalue holds the value during a collection", function() {
  let b = [{}]
  b[0].self <- b[0]
  let r = b.findvalue(function(v) { b.clear(); v = null; collectgarbage(); return true })
  return r.len()
})
show("each holds the value during a collection", function() {
  let b = [{}]
  b[0].self <- b[0]
  local n = -1
  b.each(function(v) { b.clear(); collectgarbage(); n = v.len() })
  return n
})
