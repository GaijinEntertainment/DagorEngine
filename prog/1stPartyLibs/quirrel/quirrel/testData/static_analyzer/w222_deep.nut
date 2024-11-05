//expect:w222
local x, y;


function b() {}

function foo(a) { //-declared-never-used
    local index = x < y
    b()
    ::print(a[index])
  }