//expect:w289
let function fn(aaa_x, bbb, ...) {return aaa_x + bbb}

local aaaX = 1;
local b = -1;
return fn(b, aaaX);