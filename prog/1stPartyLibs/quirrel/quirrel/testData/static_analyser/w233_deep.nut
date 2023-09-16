//expect:w233

::flags <- 10
local mask = -0x40
::aspect_ratio <- (::flags && !mask)