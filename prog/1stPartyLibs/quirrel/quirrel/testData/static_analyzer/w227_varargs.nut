

function foo(a, ...) {
    return function (b, ...) { return a + b; }
}


foo("...")