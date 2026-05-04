// EXPECTED: no error - typeof always returns string
local x: string = typeof 42
local y: string = typeof "hello"
local z: string = typeof null
return [x, y, z]
