//expect:w240

::x <- require("sq3_sa_test").x

local a = ::x; // can be null
local b = 1;

print(a ?? b != 1) // expected boolean
