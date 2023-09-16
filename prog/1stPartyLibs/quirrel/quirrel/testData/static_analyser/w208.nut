//expect:w208
local x = { z = {y = 3}}
x.z?.y <- 6
::print(x.z.y)