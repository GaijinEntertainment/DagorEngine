//expect:w212
local x = 1

function foo() { return 10 }

if (x == 1)
  ::print("1")
else if (x == 2)
  ::print("2")
else if (x == 1) // OK
  ::print("3")
else {
    x = foo()
    ::print("4");
    if (x == 1) // WRONG
        ::print("5");
}


if (x == 1)
  ::print("1")
else if (x == 2)
  ::print("2")
else {
    ;
    ;
    if (x == 1) // OK
        ::print("3");
}


if (x == 1)
  ::print("1")
else if (x == 2)
  ::print("2")
else if (x == 1) // OK
    ::print("3");;;