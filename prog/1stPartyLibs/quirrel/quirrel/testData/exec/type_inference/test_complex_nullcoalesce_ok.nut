// EXPECTED: no error
// null-coalescing: lhs is int|null, rhs is int
// result type: int (non-null part of lhs) | int (rhs) = int
function maybe_int(): int|null { return 42 }
local x: int = maybe_int() ?? 0
return null
