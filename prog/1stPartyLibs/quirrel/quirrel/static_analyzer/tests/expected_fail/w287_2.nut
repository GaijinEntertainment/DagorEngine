//expect:w287

::a <- require("sq3_sa_test").a
::listObj <- require("sq3_sa_test").listObj

local value = ::a
local cnt = ::listObj.childrenCount()

return (value >= 0) && (value <= cnt)
