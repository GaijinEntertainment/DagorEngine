// Demonstrates SQVM::FallBackGet <-> SQVM::Get recursion (1.3)
//
// Note: The pure delegation chain path (table -> delegate -> table)
// requires setdelegate() which is a C API only, not exposed to script.
// But the SAME FallBackGet function also handles OT_INSTANCE via _get
// metamethod (sqvm.cpp:2172-2188), which re-enters Get().
//
// Call chain for `a.missing_key` where a is instance of class with _get:
//   GetImpl(a, "missing_key")           // sqvm.cpp:2053
//     -> key not found in instance slots
//     -> FallBackGet(a, "missing_key")  // sqvm.cpp:2159
//       -> case OT_INSTANCE:            // sqvm.cpp:2172
//       -> GetMetaMethod(MT_GET) -> found _get
//       -> Call(_get, [a, "missing_key"]) // sqvm.cpp:2178
//         -> Execute() runs _get body
//           -> body accesses b.missing_key
//           -> GetImpl(b, "missing_key")
//             -> FallBackGet(b, "missing_key")
//               -> Call(_get, [b, "missing_key"])
//                 -> body accesses a.missing_key  <-- infinite loop!

local depth = 0
local a = null
local b = null

class A {
  other = null
  function _get(key) {
    depth++
    if (depth <= 5)
      println($"  A._get('{key}') depth={depth}, forwarding to B...")
    // Accessing .missing on the other instance triggers
    // Get -> FallBackGet -> _get on that instance
    return this.other.missing
  }
}

class B {
  other = null
  function _get(key) {
    depth++
    if (depth <= 5)
      println($"  B._get('{key}') depth={depth}, forwarding to A...")
    return this.other.missing
  }
}

a = A()
b = B()
a.other = b
b.other = a

println("Accessing a.x ...")
try {
  // This triggers: Get(a,"x") -> FallBackGet -> _get -> Get(b,"missing")
  //   -> FallBackGet -> _get -> Get(a,"missing") -> ... infinite
  local val = a.x
  println($"val = {val}")
} catch(e) {
  println($"  ... (skipping depths 6..{depth})")
  println($"Caught after depth={depth}: {e}")
}
