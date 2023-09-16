local x = 1
local y = 2
x = x //-assigned-to-itself
y = y // suppress warning -assigned-to-itself
return {x, y}