// w341 conditional-arity-mismatch: a binding initialized by a ternary of two
// function literals may hold either arm at runtime. A call whose argument count
// crashes one arm is flagged even when the other arm accepts it.

let cond = true

// Ternary of a 3-param and a 2-param lambda; the call passes 3 args, which
// crashes the 2-param arm whenever it is selected.
let addIcon = cond
  ? function(name, icon, b) { return b ? icon : name }
  : @(name, _icon) name

let _a = addIcon("bob", "*", true)

// Both arms accept the call: no warning.
let pick = cond
  ? @(x, y) x + y
  : function(a, b) { return a - b }

let _b = pick(1, 2)

// Mixed arms: one is an opaque value (a parameter, not a function literal) and
// is left unchecked; only the literal arm is validated. The 1-arg call crashes
// the 2-param literal arm.
function outer(fn) {
  let mixed = cond ? fn : @(p, q) p + q
  return mixed(1)
}

// Default parameter widens the accepted range on both arms: a 1-arg call fits.
let withDef = cond
  ? @(x, y = 5) x + y
  : function(a, b = 1) { return a + b }

let _d = withDef(10)

return { _a, _b, outer, _d }
