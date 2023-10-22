//expect:w240

local a = ::x; // can be null
local b = 1;

::print(a ?? b != 1) // expected boolean

//-file:undefined-global