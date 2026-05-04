// EXPECTED: no compile-time error (should fall back to runtime check)
// Field access on tables/objects has unknown type at compile time
local t = { value = 42 }
local x: int = t.value   // type of t.value is unknown, runtime check
return null
