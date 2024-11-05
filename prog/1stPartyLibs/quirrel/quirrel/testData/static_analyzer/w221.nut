//expect:w221
local z
function foo(y){ //-declared-never-used
    ++z
    z--
    ::x == y
  }

//-file:undefined-global