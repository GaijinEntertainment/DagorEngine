// EXPECTED: compile-time error
// null-coalescing: lhs is int|null, rhs is string
// result type: int (non-null part of lhs) | string (rhs) = int|string
// assigned to float => mismatch
function maybe_int(): int|null { return 42 }
local x: float = maybe_int() ?? "default"
return x
