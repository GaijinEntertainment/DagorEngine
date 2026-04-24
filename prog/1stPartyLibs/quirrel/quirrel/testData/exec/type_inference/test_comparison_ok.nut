// EXPECTED: no error - comparisons produce bool
local a: int = 10
local b: int = 20
local c: bool = a > b
local d: bool = a < b
local e: bool = a == b
local f: bool = a != b
local g: bool = a >= b
local h: bool = a <= b
local MyClass = class { x = 0 }
local obj = MyClass()
local i: bool = obj instanceof MyClass
local j: bool = !c
return [c, d, e, f, g, h, i, j]
