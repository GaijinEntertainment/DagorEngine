// Var args are not conflicting

function foo(a, ...) {
    return function (b, ...) { return a + b; }
}


foo("...")