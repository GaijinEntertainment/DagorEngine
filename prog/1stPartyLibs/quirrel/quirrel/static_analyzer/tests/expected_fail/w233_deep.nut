//expect:w233

::flags <- require("sq3_sa_test").flags

local mask = -0x40
::aspect_ratio <- (::flags && !mask)