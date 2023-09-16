//expect:w287

::a <- require("sq3_sa_test").a
::g_chat <- require("sq3_sa_test").b


local curVal = ::a
local x = ::g_chat.rooms.len()

return (curVal < 0 || curVal > x)
