
function foo() {}


local z = foo()

local { x = 20 } = foo()

let a = {}

while (foo()) {
    if (z) {
        x = 100
    }
}


a.x <- x

