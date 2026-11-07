// Each compare operator in a jump and as a value, for each pair of operand types
class C {
  v = 0
  constructor(v_) { this.v = v_ }
  function _cmp(o) { return this.v <=> o.v }
}
class BadCmp {
  v = 0
  function _cmp(_o) { return "x" }
}
class ThrowCmp {
  v = 0
  function _cmp(_o) { throw "cmp failed" }
}

let maxInt = 9223372036854775807
let minInt = -9223372036854775807 - 1
let big = 1e30
let inf = big * big
let nan = inf - inf
let values = [0, 1, -1, 2, 3, 100, -100, maxInt, minInt, maxInt - 1, minInt + 1, 4294967296, 0.0, -0.0, 1.0, 1.5,
  2.0, -2.5, 1e30, -1e30, inf, -inf, nan, null, true, false, "", "a", "b", [1], { a = 1 }, C(1), C(2), BadCmp(),
  ThrowCmp()]

// The message of a compare error prints a NaN differently on each platform
function errCode(e) {
  let s = $"{e}"
  if (s.indexof("comparison between ") == 0)
    return "Ec"
  if (s == "_cmp must return an integer")
    return "Em"
  return $"E:{s}"
}

function bit(c) { return c ? "1" : "0" }

function regs(a, b) {
  local r = ""
  if (a < b) r = $"{r}1"; else r = $"{r}0"
  if (a <= b) r = $"{r}1"; else r = $"{r}0"
  if (a > b) r = $"{r}1"; else r = $"{r}0"
  if (a >= b) r = $"{r}1"; else r = $"{r}0"
  if (!(a < b)) r = $"{r}1"; else r = $"{r}0"
  if (!(a <= b)) r = $"{r}1"; else r = $"{r}0"
  if (!(a > b)) r = $"{r}1"; else r = $"{r}0"
  if (!(a >= b)) r = $"{r}1"; else r = $"{r}0"
  r = $"{r}{(a < b) ? "t" : "f"}{(a >= b) ? "t" : "f"}"
  local n = 0
  while (a < b && n < 2) n++
  r = $"{r}{n}"
  n = 0
  do { n++ } while (a > b && n < 2)
  r = $"{r}{n}"
  if (a <=> b) r = $"{r}x"; else r = $"{r}y"
  if (!(a <=> b)) r = $"{r}x"; else r = $"{r}y"
  let lt = a < b
  let ge = a >= b
  return $"{r}{bit(lt)}{bit(ge)}{(a <=> b) <=> 0}"
}

function cell(f, a, b) {
  try {
    return f(a, b)
  } catch (e) {
    return errCode(e)
  }
}

foreach (i, a in values) {
  local row = ""
  foreach (b in values)
    row = $"{row} {cell(regs, a, b)}"
  println($"regs {i}:{row}")
}

function consts(a, _) {
  local r = ""
  r = $"{r}{bit(a < 2)}{bit(a <= 2)}{bit(a > 2)}{bit(a >= 2)}"
  r = $"{r}{bit(a < 0)}{bit(a <= 0)}{bit(a > 0)}{bit(a >= 0)}"
  r = $"{r}{bit(a < -1)}{bit(a >= -1)}"
  r = $"{r}{bit(a < 1.5)}{bit(a <= 1.5)}{bit(a > 1.5)}{bit(a >= 1.5)}"
  r = $"{r}{bit(a < 0.0)}{bit(a >= 0.0)}{bit(a < 2.0)}{bit(a <= 2.0)}"
  r = $"{r}{bit(a < 4294967296)}{bit(a <= 4294967296)}{bit(a > 4294967296)}{bit(a >= 4294967296)}"
  r = $"{r}{bit(a < 9223372036854775807)}{bit(a >= 9223372036854775807)}"
  r = $"{r}{bit(2 < a)}{bit(2 >= a)}{bit(1.5 < a)}{bit(1.5 >= a)}"
  local n = 0
  if (a < 2) n += 1
  if (a <= 2) n += 2
  if (a > 2) n += 4
  if (a >= 2) n += 8
  if (a < 1.5) n += 16
  if (a >= 1.5) n += 32
  if (a < 4294967296) n += 64
  if (a >= 4294967296) n += 128
  if (!(a < 2)) n += 256
  if (!(a > 1.5)) n += 512
  if (a <=> 2) n += 1024
  if (a <= -100000) n += 2048
  if (a > 100000.5) n += 4096
  return $"{r} {n} {a <=> 2} {a <=> 1.5} {a <=> 4294967296}"
}

foreach (i, a in values)
  println($"consts {i}: {cell(consts, a, null)}")

function loops() {
  local n = 0
  for (local i = 0; i < 5; i++) n++
  for (local i = 0; i <= 5; i++) n++
  for (local i = 5; i > 0; i--) n++
  for (local i = 5; i >= 0; i--) n++
  for (local i = 0; 5 > i; i++) n++
  for (local f = 0.0; f < 2.5; f += 0.5) n++
  for (local f = 2.5; f >= 0.5; f -= 0.5) n++
  for (local i = maxInt - 3; i < maxInt; i++) n++
  for (local i = minInt + 3; i > minInt; i--) n++
  let lim = 4
  for (local i = 0; i < lim; i++) n++
  let inexactInFloat32 = 20000001
  if (inexactInFloat32 > 20000000) n++
  if (inexactInFloat32 < 20000002) n++
  local i = 0
  while (i < 10) i += 3
  let r = $"{n} {i}"
  i = 10
  while (i > 0.5) i -= 3
  return $"{r} {i}"
}
println($"loops: {loops()}")

function fib(n) { return n < 2 ? n : fib(n - 1) + fib(n - 2) }
function clamp(v, lo, hi) { return v < lo ? lo : v > hi ? hi : v }
println($"fib: {fib(15)} {clamp(5, 1, 3)} {clamp(-5, 1, 3)} {clamp(2, 1, 3)} {clamp(2.5, 1, 3)} {clamp(0.5, 1, 3)}")
