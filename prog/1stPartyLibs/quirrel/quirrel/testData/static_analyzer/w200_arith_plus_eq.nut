//-file:expr-cannot-be-null

local x = {y = 2, z = 1}
local a = x?.z
a -= 10
print(a)
