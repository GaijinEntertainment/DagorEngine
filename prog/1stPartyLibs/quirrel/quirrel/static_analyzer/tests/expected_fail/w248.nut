//expect:w248

::x <- require("sq3_sa_test").x

local a = ::x?.b
return a()
