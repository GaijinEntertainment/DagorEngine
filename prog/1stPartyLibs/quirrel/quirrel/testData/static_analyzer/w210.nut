//-file:expr-cannot-be-null

local x = { b =  1 }
local y = { a = "b" }

::f <- x[y?["a"]]
