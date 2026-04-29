// EXPECTED: no error - all types match
local a: int = 42
local b: float = 3.14
local c: string = "hello"
local d: bool = true
local e: int|null = null
local f: int|string = 42
local g: int|string = "world"
local h: null = null
return [a, b, c, d, e, f, g, h]
