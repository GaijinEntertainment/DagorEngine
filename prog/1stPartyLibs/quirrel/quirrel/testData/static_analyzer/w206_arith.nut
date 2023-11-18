function foo() { return 20 }

local x = 10

x = foo();
x += foo();
x -= foo();
x *= foo();
x %= foo();
x /= foo();
x = foo();
::print(x)
x = foo()
::print("")
x = foo()