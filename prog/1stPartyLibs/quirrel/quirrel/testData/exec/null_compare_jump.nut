// == null and != null in each jump kind give the same result as in a value
#allow-switch-statement
class ThrowCmp {
  function _cmp(_o) { throw "cmp called" }
}
let values = [null, 0, 0.0, false, true, 1, -1.5, "", "a", [], {}, ThrowCmp(), ThrowCmp, @() null]

function bit(c) { return c ? "1" : "0" }

function jumps(x) {
  local r = ""
  if (x == null) r = $"{r}1"; else r = $"{r}0"
  if (x != null) r = $"{r}1"; else r = $"{r}0"
  if (!(x == null)) r = $"{r}1"; else r = $"{r}0"
  if (null == x) r = $"{r}1"; else r = $"{r}0"
  r = $"{r}{x == null ? "t" : "f"}{x != null ? "t" : "f"}"
  local n = 0
  while (x == null) { if (++n >= 2) break }
  r = $"{r}{n}"
  n = 0
  do { if (++n >= 2) break } while (x != null)
  r = $"{r}{n}"
  n = 0
  for (; x == null;) { if (++n >= 3) break }
  r = $"{r}{n}"
  n = 0
  for (; x != null;) { if (++n >= 3) break }
  r = $"{r}{n}"
  switch (x) {
    case null: r = $"{r}s"; break
    default: r = $"{r}d"
  }
  let t = { f = x }
  if (t.f == null) r = $"{r}1"; else r = $"{r}0"
  let eq = x == null
  let ne = x != null
  return $"{r} {bit(eq)}{bit(ne)}"
}

foreach (i, x in values) {
  try {
    println($"{i}: {jumps(x)}")
  } catch (e) {
    println($"{i}: error {e}")
  }
}

// A jump farther than a signed byte
function far(x) {
  local s = 0
  if (x == null) {
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
    s += 1; s += 2; s += 3; s += 4; s += 5; s += 6; s += 7; s += 8; s += 9; s += 10
  }
  return s
}
println($"far: {far(null)} {far(0)}")
