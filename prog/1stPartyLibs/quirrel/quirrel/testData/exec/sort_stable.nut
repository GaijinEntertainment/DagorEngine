// array.sort gives a permutation of the input in the order of the comparator, keeps the input order of
// elements that compare equal (the reference order below is computed with an index tie-break), and leaves a
// permutation when the comparator fails.

local rngState = 12345
function rnd(n) {
  rngState = (rngState * 1103515245 + 12345) & 0x7fffffff
  return rngState % n
}

function isPermutation(orig, res) {
  if (orig.len() != res.len())
    return false
  let cnt = {}
  foreach (v in orig)
    cnt[v] <- (cnt?[v] ?? 0) + 1
  foreach (v in res) {
    if (!(v in cnt))
      return false
    cnt[v]--
    if (cnt[v] < 0)
      return false
  }
  return true
}

function isOrdered(res, cmp) {
  for (local i = 1; i < res.len(); i++)
    if (cmp(res[i - 1], res[i]) > 0)
      return false
  return true
}

local fails = 0
local checks = 0
function check(name, ok) {
  checks++
  if (!ok) {
    fails++
    println($"FAIL {name}")
  }
}

let asc = @(a, b) a <=> b
let desc = @(a, b) b <=> a

function inputsOf(n) {
  let r = array(n), s = array(n), v = array(n), e = array(n), f = array(n), fl = array(n), st = array(n)
  for (local i = 0; i < n; i++) {
    r[i] = rnd(1000000)
    s[i] = i * 3
    v[i] = (n - i) * 3
    e[i] = 7
    f[i] = rnd(3)
    fl[i] = rnd(1000) / 7.0
    st[i] = $"s{rnd(100)}"
  }
  return [["random", r], ["sorted", s], ["reversed", v], ["equal", e], ["few", f], ["floats", fl], ["strings", st]]
}

foreach (n in [0, 1, 2, 3, 5, 8, 16, 31, 32, 33, 63, 64, 65, 100, 127, 128, 129, 1000, 5000]) {
  foreach (pair in inputsOf(n)) {
    let kind = pair[0]
    let input = pair[1]
    let a = clone input
    a.sort(asc)
    check($"asc {kind} {n}", isPermutation(input, a) && isOrdered(a, asc))
    let d = clone input
    d.sort(desc)
    check($"desc {kind} {n}", isPermutation(input, d) && isOrdered(d, desc))
    let p = clone input
    p.sort()
    check($"plain {kind} {n}", isPermutation(input, p) && isOrdered(p, asc))
    if (kind == "sorted" || kind == "reversed") {
      local calls = 0
      let c = clone input
      c.sort(function(x, y) { calls++; return x <=> y })
      let expected = n < 2 ? 0 : kind == "sorted" ? n - 1 : n - 2 + (n + 31) / 32
      check($"compares {kind} {n}", calls == expected)
    }
  }
}

foreach (n in [2, 10, 33, 64, 65, 1000, 5000]) {
  let pairs = array(n)
  for (local i = 0; i < n; i++)
    pairs[i] = [rnd(n / 4 + 1), i]
  let byKey = clone pairs
  byKey.sort(@(a, b) a[0] <=> b[0])
  let reference = clone pairs
  reference.sort(@(a, b) a[0] != b[0] ? a[0] <=> b[0] : a[1] <=> b[1])
  local same = true
  for (local i = 0; i < n; i++)
    if (byKey[i][1] != reference[i][1])
      same = false
  println($"stable {n} {same}")
}

foreach (k in [1, 2, 5, 50, 500, 5000]) {
  let orig = array(1000)
  for (local i = 0; i < 1000; i++)
    orig[i] = rnd(500)
  let a = clone orig
  local count = 0
  local caught = null
  try {
    a.sort(function(x, y) {
      if (++count == k)
        throw "boom"
      return x <=> y
    })
  } catch (e) caught = e
  check($"throw at {k} caught", caught == "boom")
  check($"throw at {k} permutation", isPermutation(orig, a))
}

{
  let a = [5, 3, 1, 4, 2, 9, 8, 7, 6, 0]
  local caught = null
  try {
    a.sort(function(x, y) {
      a.resize(17)
      return x <=> y
    })
  } catch (e) caught = e
  println($"resize: {caught}")
}

{
  let a = [3, 1, 2]
  local caught = null
  try {
    a.sort(@(_x, _y) "abc")
  } catch (e) caught = e
  println($"non-numeric: {caught}")
}

{
  let orig = array(5000)
  for (local i = 0; i < 5000; i++)
    orig[i] = rnd(100)
  let a = clone orig
  local caught = null
  try {
    a.sort(@(_x, _y) rnd(3) - 1)
  } catch (e) caught = e
  check("inconsistent comparator raises no error", caught == null)
  check("inconsistent comparator leaves a permutation", isPermutation(orig, a))
}

{
  let orig = array(3000)
  for (local i = 0; i < 3000; i++)
    orig[i] = rnd(1000)
  let a = clone orig
  let gc = require("debug")?.collectgarbage
  local n = 0
  a.sort(function(x, y) {
    if (++n % 500 == 0) {
      let inner = [3, 1, 2, { k = 1 }, [1, 2]]
      inner.sort(@(p, q) (typeof p == "integer" ? p : 100) <=> (typeof q == "integer" ? q : 100))
      if (gc != null)
        gc()
    }
    return x <=> y
  })
  check("nested sort and collect", isPermutation(orig, a) && isOrdered(a, asc))
}

{
  let orig = array(2000)
  for (local i = 0; i < 2000; i++)
    orig[i] = { v = rnd(300), i = i }
  let a = clone orig
  a.sort(@(x, y) x.v <=> y.v)
  local ok = true
  for (local i = 1; i < 2000; i++)
    if (a[i - 1].v > a[i].v || (a[i - 1].v == a[i].v && a[i - 1].i > a[i].i))
      ok = false
  check("tables by a field, stable", ok)
}

{
  let a = [1, "a", 2]
  local caught = null
  try {
    a.sort()
  } catch (e) caught = e
  println($"mixed types: {caught?.indexof("comparison between") != null}")
  let b = [3, 1.5, 2, 0.5]
  b.sort()
  println($"int and float: {", ".join(b.map(@(v) v.tostring()))}")
}

{
  let n = 300
  let a = array(n)
  for (local i = 0; i < n; i++)
    a[i] = { v = rnd(1000) }
  local calls = 0
  a.sort(function(x, y) {
    if (++calls % 7 == 0)
      a[rnd(n)] = { v = rnd(1000) }
    return x.v <=> y.v
  })
  local ok = a.len() == n
  foreach (e in a)
    if (typeof e != "table" || !("v" in e))
      ok = false
  println($"writes during the sort: {ok}")
}

foreach (action in ["append", "pop"]) {
  let n = 1000
  let a = array(n)
  for (local i = 0; i < n; i++)
    a[i] = rnd(1000)
  local calls = 0
  local caught = null
  try {
    a.sort(function(x, y) {
      if (++calls == 6000) {
        if (action == "append")
          a.append(1)
        else
          a.pop()
      }
      return x <=> y
    })
  } catch (e) caught = e
  println($"{action} during a merge: {caught} len {a.len()} nulls {a.filter(@(e) e == null).len()}")
}

println($"checks {checks} fails {fails}")
