
function foo() {}
local x = foo()?.x


if (foo() || !x)
    return
let r = x
x = null

if (r.s)
    foo()