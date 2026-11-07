// array.sort_reversed and array.sortby_reversed give the order of a stable descending sort: the result of sort
// with the two arguments of each compare swapped, made by the same comparator and _cmp calls.
// Each case runs again under a debug hook, which takes the old call, and prints the second result only when it
// differs.
from "debug" import setdebughook

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

let same = @(x, y) x.len() == y.len() && x.reduce(@(ok, v, i) ok && typeof v == typeof y[i] && v == y[i], true)
let names = @(arr) arr.map(@(v) v.n)

foreach (n in [0, 1, 2, 3, 31, 32, 33, 64, 65, 100, 1000]) {
  // sorted thirds take the run and merge shortcuts
  let src = array(n).map(@(_v, i) { k = 3 * i < n ? rnd(n / 3 + 1) : 3 * i < 2 * n ? i : 2 * n - i, i })
  let nums = src.map(@(v, i) i % 2 ? v.k : v.k.tofloat())
  show($"as a swapped stable sort, {n}", function() {
    let ref = (clone src).sort(@(a, b) b.k <=> a.k)
    let stable = ref.reduce(@(ok, v, i) ok && (i == 0 || ref[i - 1].k > v.k || ref[i - 1].i < v.i), true)
    let byCmp = (clone src).sort_reversed(@(a, b) a.k <=> b.k)
    let byKey = (clone src).sortby_reversed(@(v) v.k)
    let plain = (clone nums).sort_reversed()
    return stable && same(ref, byCmp) && same(ref, byKey) && same((clone nums).sort(@(a, b) b <=> a), plain)
  })
}

show("ties keep their order, unlike reverse", function() {
  let a = [{ k = 1, n = "a" }, { k = 2, n = "b" }, { k = 1, n = "c" }, { k = 2, n = "d" }]
  let reversed = (clone a).sort(@(x, y) x.k <=> y.k).reverse()
  return $"{",".join(names((clone a).sortby_reversed(@(v) v.k)))}; reverse: {",".join(names(reversed))}"
})
show("strings", @() ["b", "a", "c", "a"].sort_reversed())
show("string keys", @() ["pear", "fig", "banana", "kiwi"].sortby_reversed(@(w) w.len()))
show("bool keys", @() names([{ n = "a", ok = false }, { n = "b", ok = true }, { n = "c", ok = false },
  { n = "d", ok = true }].sortby_reversed(@(v) v.ok)))
show("nullable keys", @() names([{ n = "a", x = 2 }, { n = "b" }, { n = "c", x = 5 }, { n = "d" }, { n = "e", x = 2 }]
  .sortby_reversed(@(v) v?.x)))
show("index param", @() [5, 3, 9].sortby_reversed(@(_v, i) i))
show("array param", @() [5, 3, 9].sortby_reversed(@(v, _i, arr) arr.len() - v))
show("comparator ignored with two arguments", @() [3, 1, 2].sort_reversed(@(a, b) b <=> a, "ignored"))

show("comparator calls", function() {
  let src = array(100).map(@(_v) rnd(10))
  let seen = [[], []]
  let r0 = (clone src).sort_reversed(function(a, b) { seen[0].append($"{a}:{b}"); return a <=> b })
  let r1 = (clone src).sort(function(a, b) { seen[1].append($"{b}:{a}"); return b <=> a })
  return same(r0, r1) && seen[0].len() > 0 && same(seen[0], seen[1])
})
show("_cmp calls", function() {
  let log = [[], [], []]
  local cur = 0
  let Item = class {
    v = 0
    constructor(x) { this.v = x }
    function _cmp(other) { log[cur].append($"{this.v}:{other.v}"); return this.v <=> other.v }
  }
  let src = array(100).map(@(_v) Item(rnd(10)))
  let r0 = (clone src).sort_reversed()
  cur = 1
  let r1 = (clone src).sortby_reversed(@(v) v)
  cur = 2
  let r2 = (clone src).sort(@(a, b) b <=> a)
  return same(r0, r2) && same(r1, r2) && log[0].len() > 0 && same(log[0], log[2]) && same(log[1], log[2])
})

show("no calls for one element or none", function() {
  local calls = 0
  foreach (a in [[], [7]]) {
    a.sort_reversed(function(x, y) { calls++; return x <=> y })
    a.sortby_reversed(function(v) { calls++; return v })
  }
  return calls
})
show("two elements", function() {
  local calls = 0
  let up = [1, 2].sort_reversed(function(x, y) { calls++; return x <=> y })
  let down = [2, 1].sort_reversed()
  let tie = [{ k = 1, n = "a" }, { k = 1, n = "b" }]
  let byCmp = names((clone tie).sort_reversed(function(x, y) { calls++; return x.k <=> y.k }))
  let byKey = names((clone tie).sortby_reversed(@(v) v.k))
  return $"{up[0]},{up[1]} {down[0]},{down[1]} {",".join(byCmp)} {",".join(byKey)} calls {calls}"
})

show("comparator throws", @() [3, 1, 2].sort_reversed(function(_a, _b) { throw "boom" }))
show("non-numeric comparator result", @() [3, 1, 2].sort_reversed(@(_a, _b) "abc"))
show("incomparable elements", function() {
  local swapped = null
  try {
    [1, "a"].sort(@(a, b) b <=> a)
  } catch (e) swapped = e
  try {
    [1, "a"].sort_reversed()
  } catch (e) return $"{e}; same as the swapped comparator: {e == swapped}"
  return "no error"
})
show("key throws, array unchanged", function() {
  let a = [5, 3, 9, 1]
  try {
    a.sortby_reversed(function(v) { if (v == 9) throw "nine"; return v })
  } catch (e)
    return $"{e}: {",".join(a.map(@(v) $"{v}"))}"
  return "no error"
})
show("incomparable keys, array unchanged", function() {
  let a = [5, 3, 9, 1]
  try {
    a.sortby_reversed(@(v) v == 9 ? "s" : v)
  } catch (e)
    return $"{e.indexof("comparison between") != null}: {",".join(a.map(@(v) $"{v}"))}"
  return "no error"
})
show("wrong param count", @() [5, 3].sortby_reversed(@(_a, _b, _c, _d) 0))
show("immutable array", @() freeze([3, 1, 2]).sort_reversed())
show("immutable array, key", @() freeze([3, 1, 2]).sortby_reversed(@(v) v))

show("comparator resizes the array", function() {
  let a = [5, 3, 9, 1]
  a.sort_reversed(function(x, y) { if (a.len() < 6) a.append(0); return x <=> y })
  return a
})
show("key resizes the array", function() {
  let a = [5, 3, 9, 1]
  a.sortby_reversed(function(v) { if (v == 9) a.append(0); return v })
  return a
})
show("key _cmp resizes the array", function() {
  let a = [5, 3, 9, 1]
  let Key = class {
    v = 0
    constructor(x) { this.v = x }
    function _cmp(other) { if (a.len() < 6) a.append(0); return this.v <=> other.v }
  }
  a.sortby_reversed(@(v) Key(v))
  return a
})
foreach (action in ["append", "pop"]) {
  show($"{action} during a merge", function() {
    let a = array(1000).map(@(_v) rnd(1000))
    local calls = 0
    try {
      a.sort_reversed(function(x, y) {
        if (++calls == 6000) {
          if (action == "append")
            a.append(1)
          else
            a.pop()
        }
        return x <=> y
      })
    } catch (e)
      return $"{e} len {a.len()} nulls {a.filter(@(v) v == null).len()}"
    return "no error"
  })
}
