local x = -0x7FFFFFFF_FFFFFFFF - 1
local y = @(a) -a
print( x % y(1) )
