//expect:w222
local x, y;


let function b() {}

let function foo(a) { //-declared-never-used
    local index = x < y
    b()
    ::print(a[index])
  }