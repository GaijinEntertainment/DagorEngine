// EXPECTED: no error - freeze() preserves type of its argument
local a: table = freeze({x = 1})
local b: array = freeze([1, 2, 3])
return [a, b]
