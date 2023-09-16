//expect:w285

::x <- require("sq3_sa_test").x

local regions = ::x ? 2 : 4
return regions != null
