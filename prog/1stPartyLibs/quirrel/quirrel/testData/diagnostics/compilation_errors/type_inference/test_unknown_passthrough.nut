// EXPECTED: no compile-time error (should fall back to runtime check)
// When the expression type is unknown (~0u), the compiler cannot check
// and must emit _OP_CHECK_TYPE for runtime. This test verifies no false positive.
function unknown_fn(x) {  // no return type annotation
    return x
}
local x: int = unknown_fn(42)   // unknown return type, runtime check only
return null
