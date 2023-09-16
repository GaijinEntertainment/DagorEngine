//expect:w247

::a <- require("sq3_sa_test").a

local result = ::a.b.c.indexof("x")
return result + 6;