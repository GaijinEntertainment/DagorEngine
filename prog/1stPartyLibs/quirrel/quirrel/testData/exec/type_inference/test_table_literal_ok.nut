// EXPECTED: no error - table literal assigned to table variable
local x: table = { a = 1, b = 2, c = "hello" }
local y: table|null = null
local z: table|null = { key = "value" }
return [x, y, z]
