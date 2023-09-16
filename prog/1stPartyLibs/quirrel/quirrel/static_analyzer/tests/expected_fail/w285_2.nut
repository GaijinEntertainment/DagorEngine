//expect:w285

::unlock <- require("sq3_sa_test").unlock

local regions = ::unlock?.meta.regions ?? [::unlock?.meta.region] ?? []
return regions
