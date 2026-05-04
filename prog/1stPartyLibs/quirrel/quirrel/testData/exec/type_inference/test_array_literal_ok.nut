// EXPECTED: no error - array literal assigned to array variable
local x: array = [1, 2, 3]
local y: array|null = null
local z: array|null = ["a", "b"]
return [x, y, z]
