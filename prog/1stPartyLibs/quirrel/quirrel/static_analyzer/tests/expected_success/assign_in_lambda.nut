local x
print(@() x = 3) // (function)
local y = @() x = 4
print(y()) // 4
print(x) // 4

