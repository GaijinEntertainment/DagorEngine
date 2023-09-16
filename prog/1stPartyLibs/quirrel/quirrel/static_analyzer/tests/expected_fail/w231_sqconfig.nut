//expect:w231

::TEST_F <- require("sq3_sa_test").TEST_F
::x <- require("sq3_sa_test").x

return ::TEST_F("%d%%", 1, ::x)

