//expect:w259

::a <- require("sq3_sa_test").a
::b <- require("sq3_sa_test").b
::c <- require("sq3_sa_test").c
::d <- require("sq3_sa_test").d
::x <- require("sq3_sa_test").x


local numAnimations = ::a + ::b + ::c + ::d - (::a + ::b + ::c + ::d) * ::x + 123
local numTextAnimations = ::a + ::b + ::c + ::d - (::a + ::b + ::c + ::d) * ::x + 124

return [numAnimations, numTextAnimations]
