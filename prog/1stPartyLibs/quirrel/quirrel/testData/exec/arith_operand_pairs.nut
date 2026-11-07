from "iostream" import castf2i

function noptr(s) {
  local i = s.indexof(" : 0x")
  while (i != null) {
    let j = s.indexof(")", i)
    s = $"{s.slice(0, i)}{s.slice(j)}"
    i = s.indexof(" : 0x")
  }
  return s
}

function show(v) {
  let t = type(v)
  if (t == "float")
    return $"f:{v}:{castf2i(v)}"
  if (t == "string")
    return $"s:{noptr(v)}"
  if (t == "array" || t == "table" || t == "instance")
    return t
  return $"{t}:{v}"
}

class Num {
  v = 0
  constructor(v_) { this.v = v_ }
  function _add(o) { return $"add({this.v},{show(o)})" }
  function _sub(o) { return $"sub({this.v},{show(o)})" }
  function _mul(o) { return $"mul({this.v},{show(o)})" }
  function _div(o) { return $"div({this.v},{show(o)})" }
  function _modulo(o) { return $"mod({this.v},{show(o)})" }
  function _tostring() { return $"Num({this.v})" }
}

class Plain {}

function run(f) {
  try
    return show(f())
  catch (e)
    return $"E:{e}"
}

let values = [0, 1, -1, 7, 9223372036854775807, -9223372036854775807 - 1, 0.0, -0.0, 0.5, -2.5, 1e30,
  null, true, "s", [1], { a = 1 }, Num(3), Plain()]

let ops = [
  ["+", @(a, b) a + b],
  ["-", @(a, b) a - b],
  ["*", @(a, b) a * b],
  ["/", @(a, b) a / b],
  ["%", @(a, b) a % b],
]

foreach ([name, f] in ops) {
  foreach (a in values) {
    local row = $"{show(a)} {name}"
    foreach (b in values)
      row = $"{row} | {run(@() f(a, b))}"
    println(row)
  }
}

let literals = [
  ["+1", @(a) a + 1], ["-1", @(a) a - 1], ["1-", @(a) 1 - a], ["1.5/", @(a) 1.5 / a],
]
foreach (a in values) {
  local row = $"literal {show(a)}"
  foreach ([name, f] in literals)
    row = $"{row} | {name} {run(@() f(a))}"
  println(row)
}

function compound(x, d) {
  let t = { v = x }
  let a = [x]
  let n = Num(x)
  local l = x
  local r = ""
  foreach (step in ["+", "-", "*", "/", "%"]) {
    try {
      if (step == "+") { t.v += d; a[0] += d; n.v += d; l += d }
      if (step == "-") { t.v -= d; a[0] -= d; n.v -= d; l -= d }
      if (step == "*") { t.v *= d; a[0] *= d; n.v *= d; l *= d }
      if (step == "/") { t.v /= d; a[0] /= d; n.v /= d; l /= d }
      if (step == "%") { t.v %= d; a[0] %= d; n.v %= d; l %= d }
      let res = [show(t.v), show(a[0]), show(n.v), show(l)]
      r = res.filter(@(s) s == res[0]).len() == res.len() ? $"{r} {res[0]}" : $"{r} {",".join(res)}"
    } catch (e) {
      r = $"{r} E:{e}"
    }
  }
  return r
}
foreach (x in [1, 2.5, -3, 0.0, null, "s"])
  foreach (d in [2, 0.5, -1, 0, null, "t"])
    println($"compound {show(x)} {show(d)}:{compound(x, d)}")

function increments(x) {
  let t = { v = x }
  local l = x
  local r = ""
  foreach (step in ["pre", "post", "dec"]) {
    try {
      if (step == "pre") { r = $"{r} {show(++t.v)},{show(++l)}" }
      if (step == "post") { r = $"{r} {show(t.v++)},{show(l++)}" }
      if (step == "dec") { r = $"{r} {show(--t.v)},{show(l--)},{show(--l)}" }
      r = $"{r}={show(t.v)},{show(l)}"
    } catch (e) {
      r = $"{r} E:{e}"
    }
  }
  return r
}
foreach (x in [1, 2.5, 9223372036854775807, null, "s", true])
  println($"inc {show(x)}:{increments(x)}")

function chain(a, b, c) { return a * b + c - a / (b + 1) }
foreach (a in [1, 2.5])
  foreach (b in [3, 0.5])
    foreach (c in [4, -1.25])
      println($"chain {a} {b} {c}: {show(chain(a, b, c))} {show(-a)} {show(-(a * b))}")
