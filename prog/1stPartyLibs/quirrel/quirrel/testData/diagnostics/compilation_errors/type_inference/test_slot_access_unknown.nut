// EXPECTED: no compile-time error (should fall back to runtime check)
// Slot access (dynamic key) has unknown type at compile time
local t = { value = 42 }
local key = "value"
local x: int = t[key]   // type of t[key] is unknown, runtime check
return null
