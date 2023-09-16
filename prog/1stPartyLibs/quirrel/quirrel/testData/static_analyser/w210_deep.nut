//expect:w210
local y = { a = "b" }
local index = y?["a"]
local x = { b =  1 }

::f <- x[index]