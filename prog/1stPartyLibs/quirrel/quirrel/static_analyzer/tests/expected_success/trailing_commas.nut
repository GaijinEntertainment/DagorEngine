local a = [1, 2,]
local b = {x=1, y=2,}
local function fn(_a, _b,) {}
fn(1, 2,)
local {x, y,} = b
local [z, w,] = a

return {a, b, fn, x, y, z, w,}