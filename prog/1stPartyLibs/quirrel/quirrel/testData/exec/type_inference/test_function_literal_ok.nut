// EXPECTED: no error - lambda assigned to function variable
local x: function = @() 42
local y: function|null = null
local z: function|null = @(a, b) a + b
return [x, y, z]
