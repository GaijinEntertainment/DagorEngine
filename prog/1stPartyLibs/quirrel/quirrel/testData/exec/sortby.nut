// array.sortby gives the order of a stable sort by the keys, calls the key function once per element and in order,
// and leaves the array as it was after an error.
// The output must not depend on how the key function is called: each case runs again under a debug hook, which takes
// the old call, and prints the second result only when it differs.
from "debug" import setdebughook, collectgarbage

local rngState = 12345
function rnd(n) {
  rngState = (rngState * 1103515245 + 12345) & 0x7fffffff
  return rngState % n
}

function run(f, hooked) {
  if (hooked)
    setdebughook(@(_event, _src, _line, _fname) null)
  local text
  try {
    let r = f()
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

foreach (n in [0, 1, 2, 31, 32, 33, 100, 1000]) {
  let src = array(n).map(@(_v, i) { k = rnd(n / 3 + 1), i })
  show($"as a stable sort, {n}", function() {
    let byKey = clone src
    byKey.sortby(@(v) v.k)
    let byCmp = clone src
    byCmp.sort(@(a, b) a.k <=> b.k)
    return byKey.len() == n && byKey.reduce(@(ok, v, i) ok && v == byCmp[i], true)
  })
}

show("strings", @() ["pear", "fig", "banana", "kiwi"].sortby(@(w) w.len()))
show("floats and integers", @() [3, 1.5, 2, 0.5, 2.0].sortby(@(v) -v))
show("index param", @() [5, 3, 9].sortby(@(_v, i) -i))
show("array param", @() [5, 3, 9].sortby(@(v, _i, arr) arr.len() - v))
show("key calls in order", function() {
  let seen = []
  let a = [5, 3, 9, 1]
  a.sortby(function(v) { seen.append(v); return v })
  return seen
})
show("no calls for one element", function() {
  local calls = 0
  let one = [7]
  let none = []
  one.sortby(function(v) { calls++; return v })
  none.sortby(function(v) { calls++; return v })
  return calls
})
show("key throws, array unchanged", function() {
  let a = [5, 3, 9, 1]
  try {
    a.sortby(function(v) { if (v == 9) throw "nine"; return v })
  } catch (e)
    return $"{e}: {",".join(a.map(@(v) $"{v}"))}"
  return "no error"
})
show("incomparable keys, array unchanged", function() {
  let a = [5, 3, 9, 1]
  try {
    a.sortby(@(v) v == 9 ? "s" : v)
  } catch (e)
    return $"{e.indexof("comparison between") != null}: {",".join(a.map(@(v) $"{v}"))}"
  return "no error"
})
show("key resizes the array", function() {
  let a = [5, 3, 9, 1]
  a.sortby(function(v) { if (v == 9) a.append(0); return v })
  return a
})
show("keys with _cmp", function() {
  let Rev = class {
    v = 0
    constructor(x) { this.v = x }
    function _cmp(other) { return other.v <=> this.v }
  }
  return [5, 3, 9, 1].sortby(@(v) Rev(v))
})
show("native key function", @() [1, "a", 2, "b", 3.5].sortby(type))
show("wrong param count", @() [5, 3].sortby(@(_a, _b, _c, _d) 0))
show("immutable array", @() freeze([3, 1, 2]).sortby(@(v) v))
show("key _cmp resizes the array", function() {
  let a = [5, 3, 9, 1]
  let Key = class {
    v = 0
    constructor(x) { this.v = x }
    function _cmp(other) { if (a.len() < 6) a.append(0); return this.v <=> other.v }
  }
  a.sortby(@(v) Key(v))
  return a
})
show("deep key, three params", function() {
  function depth(n) { return n == 0 ? 0 : 1 + depth(n - 1) }
  return [5, 3, 9, 1].sortby(@(v, _i, arr) depth(500) + arr.len() - v)
})
show("under a debug hook", function() {
  local calls = 0
  setdebughook(function(event, _src, _line, fname) { if (event == 'c' && fname == "keyOf") calls++ })
  let r = [5, 3, 9, 1].sortby(function keyOf(v) { return v })
  setdebughook(null)
  println($"  hook saw the calls: {calls == 4}")
  return r
})

show("weakref elements keep weak ownership", function() {
  let owners = array(40).map(@(_v, i) { k = 39 - i })
  let refs = owners.map(@(v) v.weakref())
  refs.sortby(@(v) v.k)
  let sorted = refs.reduce(@(ok, v, i) ok && v.k == i, true)
  owners.clear()
  return sorted && refs.reduce(@(ok, v) ok && v == null, true)
})
show("weakref identity keys", function() {
  let Key = class {
    v = 0
    constructor(x) { this.v = x }
    function _cmp(other) { return this.v <=> other.v }
  }
  let first = Key(2)
  let second = Key(1)
  let refs = [first.weakref(), second.weakref()]
  refs.sortby(@(v) v)
  return refs.map(@(v) v.v)
})
show("expired weakref elements", function() {
  local owner = { k = 2 }
  let expired = owner.weakref()
  owner = null
  let live = { k = 3 }
  let refs = [live.weakref(), expired]
  refs.sortby(@(v) v?.k ?? -1)
  return refs.map(@(v) v?.k ?? -1)
})

show("sortby in a _cmp metamethod", function() {
  let Item = class {
    v = 0
    constructor(x) { this.v = x }
    function _cmp(other) {
      let both = [this, other].sortby(@(p) p.v)
      return both[0] == this ? this.v <=> other.v : 1
    }
  }
  return [5, 3, 9, 1, 7].map(@(x) Item(x)).sort().map(@(p) p.v)
})
show("sortby in a key function", @() [[3, 1], [2, 9], [5, 0]].sortby(@(p) (clone p).sortby(@(x) -x)[0]).map(@(p) $"{p[0]}{p[1]}"))
show("collect during key calls", function() {
  let Key = class {
    k = 0
    constructor(x) { this.k = x }
    function _cmp(other) { return this.k <=> other.k }
  }
  let src = array(40).map(@(_v, i) { k = (i * 7) % 40 })
  let small = src.slice(0, 5).reverse()
  let want = [small, src].map(@(a) a.map(@(v) v.k).sort())
  let r = [small, src].map(@(a) (clone a).sortby(function(v) { collectgarbage(); return Key(v.k) }))
  return r.map(@(a, j) a.reduce(@(ok, v, i) ok && v.k == want[j][i], true))
})
