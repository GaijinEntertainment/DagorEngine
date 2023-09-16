//expect:w232
local i = {x = true}
::print(i.x && !i.x)
::print(!i.x || i.x)
