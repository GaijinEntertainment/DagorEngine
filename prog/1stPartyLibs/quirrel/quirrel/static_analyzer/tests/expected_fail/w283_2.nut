//expect:w283

::y <- require("sq3_sa_test").y


local s = null
local x = ::y ?? s
return x
