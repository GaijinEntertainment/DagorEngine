// Some cases that might logically seem like they should warn
// are not flagged due to analyzer limitations in tracking complex flows.

if (__name__ == "__analysis__")
  return

function foo(_p = null) {}
function bar(_x = null) {}

//=============================================================================
// Nested ternary expressions with nullable values
//=============================================================================
{
  let a = foo()?.x
  let b = foo()?.y
  // Nested ternary where both branches could be null
  // TRUE NEGATIVE: a and b are checked before access in their respective branches
  let _result = a != null
    ? (b != null ? a.val + b.val : a.val)
    : (b != null ? b.val : 0)

  // KNOWN LIMITATION: a.missing is in else branch where a wasn't checked,
  // but analyzer doesn't track ternary else-branch nullable state deeply
  let c = foo()?.z
  let _bad = c != null ? c.val : a.missing
}

//=============================================================================
// Multiple reassignments in complex control flow
//=============================================================================
{
  local x = foo()?.data
  if (x != null) {
    x = foo()?.other  // reassign to new nullable
  }
  // KNOWN LIMITATION: After if-block, analyzer loses precise tracking
  // of reassigned variables. x could be null but no warning issued.
  foo(x.field)
}

{
  // TRUE NEGATIVE: y is guaranteed non-null after null-coalescing assignment
  local y = foo()?.val
  y = y ?? { default = 1 }
  foo(y.default)
}

//=============================================================================
// Closure captures of nullable variables
//=============================================================================
{
  // EXPECT WARNING: closure captures nullable without outer null-check
  let outer = foo()?.captured
  let _closure = @() outer.accessed
}

{
  // TRUE NEGATIVE: closure defined inside null-check block inherits the checked state
  let checked = foo()?.val
  if (checked != null) {
    let _safe_closure = @() checked.safe
  }
}

//=============================================================================
// Loop-carried nullable dependencies
//=============================================================================
{
  // TRUE NEGATIVE inside loop: prev is explicitly checked before access
  local prev = null
  foreach (item in [1, 2, 3]) {
    if (prev != null) {
      foo(prev.data)
    }
    prev = foo(item)?.result
  }
  // KNOWN LIMITATION: After loop, prev could be null but analyzer
  // doesn't track loop exit state precisely
  foo(prev.final)
}

//=============================================================================
// Switch/case with nullable checks (using if-else chain pattern)
//=============================================================================
{
  let val = foo()?.type
  let obj = foo()?.obj

  // EXPECT WARNING: obj not checked, only val is tested
  if (val == "string") {
    foo(obj.asString)
  } else if (val == "number") {
    foo(obj.asNumber)
  } else if (obj != null) {
    // TRUE NEGATIVE: obj explicitly checked in this branch
    foo(obj.other)
  }
}

//=============================================================================
// Safe access chain followed by unsafe access
//=============================================================================
{
  // TRUE NEGATIVE: if deep is non-null, the entire chain must have succeeded,
  // meaning chain is non-null. Analyzer correctly infers this via extractReceiver.
  let chain = foo()
  let deep = chain?.level1?.level2?.level3
  if (deep != null) {
    foo(chain.direct)
  }
}

//=============================================================================
// Nullable in array/table literal (edge case)
//=============================================================================
{
  // EXPECT WARNING: accessing nullable in collection literal
  let nullable = foo()?.item
  let _arr = [nullable.prop]
  let _tbl = { key = nullable.prop2 }
}

//=============================================================================
// Double negation pattern
//=============================================================================
{
  // TRUE NEGATIVE: !! coerces to boolean, so if (!!val) means val is truthy (non-null)
  let val = foo()?.x
  if (!!val) {
    foo(val.data)
  }
}

//=============================================================================
// Nullable used as function argument then accessed
//=============================================================================
{
  // EXPECT WARNING: passing nullable to function is OK, but accessing member is not
  let arg = foo()?.param
  bar(arg)
  foo(arg.field)
}

//=============================================================================
// Try-catch with nullable
//=============================================================================
{
  // EXPECT WARNING in both try and catch: nullable not checked in either block
  let val = foo()?.risky
  try {
    foo(val.attempt)
  } catch (_e) {
    foo(val.fallback)
  }
}

//=============================================================================
// Assignment before null guard - alias tracking
//=============================================================================
{
  // TRUE NEGATIVE: y holds the same value as x. The guard on x proves
  // that value is non-null, and the analyzer tracks this aliasing relationship.
  local x = foo()?.val
  let y = x              // y is an alias for x's value
  if (!x) return         // guard proves the value (held by both x and y) is non-null
  foo(y.field)           // no warning - y is non-null via alias tracking
}

{
  // EXPECT WARNING: x was reassigned before the guard, so y might hold old null value
  local x = foo()?.val
  let y = x              // y holds x's current (possibly null) value
  x = foo()?.other       // x reassigned to different value
  if (!x) return         // guard proves NEW x is non-null, but y has OLD value
  foo(y.field)           // warning - y might be null (holds old x value)
}
