//expect:w285

::x <- require("sq3_sa_test").x


local regions = ::x ? [] : {}
return regions ?? 123
