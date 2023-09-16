
function foo() {return 10}

let y = 5000
local x
do {
  x = foo()
} while (x >= y)

return 0