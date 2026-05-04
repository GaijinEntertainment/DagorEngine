// EXPECTED: no error
// (1 + 2) * 3 => int, typeof "x" => string => ternary is int|string
function get_flag(): bool { return true }
local n: int = 10
local x: int|string = get_flag() ? ((1 + 2) * 3 + n) : typeof "x"
return null
