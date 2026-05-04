// EXPECTED: no error - string + anything produces string
local s: string = "hello"
local n: int = 42
local result: string = s + n
local result2: string = s + " world"
return [result, result2]
