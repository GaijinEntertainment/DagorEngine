//-file:expr-cannot-be-null

local y = { a = "b" }
local index = y?["a"]
local x = { b =  1 }

::f <- x[index]
